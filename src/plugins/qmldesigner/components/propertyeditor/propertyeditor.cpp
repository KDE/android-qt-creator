/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "propertyeditor.h"

#include <nodemetainfo.h>
#include <metainfo.h>

#include <invalididexception.h>
#include <rewritingexception.h>
#include <invalidnodestateexception.h>
#include <variantproperty.h>

#include <bindingproperty.h>

#include <nodeabstractproperty.h>
#include <rewriterview.h>

#include "propertyeditorvalue.h"
#include "basiclayouts.h"
#include "basicwidgets.h"
#include "resetwidget.h"
#include "qlayoutobject.h"
#include <qmleditorwidgets/colorwidgets.h>
#include "gradientlineqmladaptor.h"
#include "behaviordialog.h"
#include "qproxylayoutitem.h"
#include "fontwidget.h"
#include "siblingcombobox.h"
#include "propertyeditortransaction.h"
#include "originwidget.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtDeclarative/QDeclarativeView>
#include <QtDeclarative/QDeclarativeContext>
#include <QtGui/QVBoxLayout>
#include <QtGui/QShortcut>
#include <QtGui/QStackedWidget>
#include <QDeclarativeEngine>
#include <private/qdeclarativemetatype_p.h>
#include <QMessageBox>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QToolBar>

enum {
    debug = false
};

const int collapseButtonOffset = 114;

namespace QmlDesigner {

PropertyEditor::NodeType::NodeType(PropertyEditor *propertyEditor) :
        m_view(new DeclarativeWidgetView), m_propertyEditorTransaction(new PropertyEditorTransaction(propertyEditor)), m_dummyPropertyEditorValue(new PropertyEditorValue()),
        m_contextObject(new PropertyEditorContextObject())
{
    Q_ASSERT(QFileInfo(":/images/button_normal.png").exists());

    QDeclarativeContext *ctxt = m_view->rootContext();
    m_view->engine()->setOutputWarningsToStandardError(debug);
    m_dummyPropertyEditorValue->setValue("#000000");
    ctxt->setContextProperty("dummyBackendValue", m_dummyPropertyEditorValue.data());
    m_contextObject->setBackendValues(&m_backendValuesPropertyMap);
    ctxt->setContextObject(m_contextObject.data());

    connect(&m_backendValuesPropertyMap, SIGNAL(valueChanged(const QString&, const QVariant&)), propertyEditor, SLOT(changeValue(const QString&)));
}

PropertyEditor::NodeType::~NodeType()
{
}

void setupPropertyEditorValue(const QString &name, QDeclarativePropertyMap *propertyMap, PropertyEditor *propertyEditor, const QString &type)
{
    QString propertyName(name);
    propertyName.replace(QLatin1Char('.'), QLatin1Char('_'));
    PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(propertyMap->value(propertyName)));
    if (!valueObject) {
        valueObject = new PropertyEditorValue(propertyMap);
        QObject::connect(valueObject, SIGNAL(valueChanged(QString, const QVariant&)), propertyMap, SIGNAL(valueChanged(QString, const QVariant&)));
        QObject::connect(valueObject, SIGNAL(expressionChanged(QString)), propertyEditor, SLOT(changeExpression(QString)));
        propertyMap->insert(propertyName, QVariant::fromValue(valueObject));
    }
    valueObject->setName(propertyName);
    if (type == "QColor")
        valueObject->setValue(QVariant("#000000"));
    else
        valueObject->setValue(QVariant(1));

}

void createPropertyEditorValue(const QmlObjectNode &fxObjectNode, const QString &name, const QVariant &value, QDeclarativePropertyMap *propertyMap, PropertyEditor *propertyEditor)
{
    QString propertyName(name);
    propertyName.replace(QLatin1Char('.'), QLatin1Char('_'));
    PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(propertyMap->value(propertyName)));
    if (!valueObject) {
        valueObject = new PropertyEditorValue(propertyMap);
        QObject::connect(valueObject, SIGNAL(valueChanged(QString, const QVariant&)), propertyMap, SIGNAL(valueChanged(QString, const QVariant&)));
        QObject::connect(valueObject, SIGNAL(expressionChanged(QString)), propertyEditor, SLOT(changeExpression(QString)));
        propertyMap->insert(propertyName, QVariant::fromValue(valueObject));
    }
    valueObject->setName(name);
    valueObject->setModelNode(fxObjectNode);

    if (fxObjectNode.propertyAffectedByCurrentState(name) && !(fxObjectNode.modelNode().property(name).isBindingProperty())) {
        valueObject->setValue(fxObjectNode.modelValue(name));

    } else {
        valueObject->setValue(value);
    }

    if (propertyName != QLatin1String("id") &&
        fxObjectNode.currentState().isBaseState() &&
        fxObjectNode.modelNode().property(propertyName).isBindingProperty()) {
        valueObject->setExpression(fxObjectNode.modelNode().bindingProperty(propertyName).expression());
    } else {
        valueObject->setExpression(fxObjectNode.instanceValue(name).toString());
    }
}

void PropertyEditor::NodeType::setValue(const QmlObjectNode & fxObjectNode, const QString &name, const QVariant &value)
{
    QString propertyName = name;
    propertyName.replace(QLatin1Char('.'), QLatin1Char('_'));
    PropertyEditorValue *propertyValue = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_backendValuesPropertyMap.value(propertyName)));
    if (propertyValue) {
        propertyValue->setValue(value);
        if (!fxObjectNode.hasBindingProperty(name))
            propertyValue->setExpression(value.toString());
        else
            propertyValue->setExpression(fxObjectNode.expression(name));
    }
}

void PropertyEditor::NodeType::setup(const QmlObjectNode &fxObjectNode, const QString &stateName, const QUrl &qmlSpecificsFile, PropertyEditor *propertyEditor)
{
    if (!fxObjectNode.isValid()) {
        return;
    }

    QDeclarativeContext *ctxt = m_view->rootContext();

    if (fxObjectNode.isValid()) {
        foreach (const QString &propertyName, fxObjectNode.modelNode().metaInfo().propertyNames())
            createPropertyEditorValue(fxObjectNode, propertyName, fxObjectNode.instanceValue(propertyName), &m_backendValuesPropertyMap, propertyEditor);

        // className
        PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_backendValuesPropertyMap.value("className")));
        if (!valueObject)
            valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
        valueObject->setName("className");
        valueObject->setModelNode(fxObjectNode.modelNode());
        valueObject->setValue(fxObjectNode.modelNode().simplifiedTypeName());
        QObject::connect(valueObject, SIGNAL(valueChanged(QString, const QVariant&)), &m_backendValuesPropertyMap, SIGNAL(valueChanged(QString, const QVariant&)));
        m_backendValuesPropertyMap.insert("className", QVariant::fromValue(valueObject));

        // id
        valueObject = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_backendValuesPropertyMap.value("id")));
        if (!valueObject)
            valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
        valueObject->setName("id");
        valueObject->setValue(fxObjectNode.id());
        QObject::connect(valueObject, SIGNAL(valueChanged(QString, const QVariant&)), &m_backendValuesPropertyMap, SIGNAL(valueChanged(QString, const QVariant&)));
        m_backendValuesPropertyMap.insert("id", QVariant::fromValue(valueObject));

        // anchors
        m_backendAnchorBinding.setup(QmlItemNode(fxObjectNode.modelNode()));

        ctxt->setContextProperty("anchorBackend", &m_backendAnchorBinding);
        
        ctxt->setContextProperty("transaction", m_propertyEditorTransaction.data());
        
        m_contextObject->setSpecificsUrl(qmlSpecificsFile);
        
        m_contextObject->setStateName(stateName);
        QApplication::processEvents();
        if (!fxObjectNode.isValid())
            return;
        ctxt->setContextProperty("propertyCount", QVariant(fxObjectNode.modelNode().properties().count()));

        m_contextObject->setIsBaseState(fxObjectNode.isInBaseState());
        m_contextObject->setSelectionChanged(false);

        m_contextObject->setSelectionChanged(false);
    } else {
        qWarning() << "PropertyEditor: invalid node for setup";
    }
}

void PropertyEditor::NodeType::initialSetup(const QString &typeName, const QUrl &qmlSpecificsFile, PropertyEditor *propertyEditor)
{
    QDeclarativeContext *ctxt = m_view->rootContext();

    NodeMetaInfo metaInfo = propertyEditor->model()->metaInfo(typeName, 4, 7);

    foreach (const QString &propertyName, metaInfo.propertyNames())
        setupPropertyEditorValue(propertyName, &m_backendValuesPropertyMap, propertyEditor, metaInfo.propertyTypeName(propertyName));

    PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_backendValuesPropertyMap.value("className")));
    if (!valueObject)
        valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
    valueObject->setName("className");

    valueObject->setValue(typeName);
    QObject::connect(valueObject, SIGNAL(valueChanged(QString, const QVariant&)), &m_backendValuesPropertyMap, SIGNAL(valueChanged(QString, const QVariant&)));
    m_backendValuesPropertyMap.insert("className", QVariant::fromValue(valueObject));

    // id
    valueObject = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_backendValuesPropertyMap.value("id")));
    if (!valueObject)
        valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
    valueObject->setName("id");
    valueObject->setValue("id");
    QObject::connect(valueObject, SIGNAL(valueChanged(QString, const QVariant&)), &m_backendValuesPropertyMap, SIGNAL(valueChanged(QString, const QVariant&)));
    m_backendValuesPropertyMap.insert("id", QVariant::fromValue(valueObject));

    ctxt->setContextProperty("anchorBackend", &m_backendAnchorBinding);
    ctxt->setContextProperty("transaction", m_propertyEditorTransaction.data());

    m_contextObject->setSpecificsUrl(qmlSpecificsFile);

    m_contextObject->setStateName(QLatin1String("basestate"));

    m_contextObject->setIsBaseState(true);

    m_contextObject->setSpecificQmlData(QLatin1String(""));

    m_contextObject->setGlobalBaseUrl(QUrl());
}

PropertyEditor::PropertyEditor(QWidget *parent) :
        QmlModelView(parent),
        m_parent(parent),
        m_updateShortcut(0),
        m_timerId(0),
        m_stackedWidget(new StackedWidget(parent)),
        m_currentType(0),
        m_locked(false),
        m_setupCompleted(false),
        m_singleShotTimer(new QTimer(this))
{
    m_updateShortcut = new QShortcut(QKeySequence("F5"), m_stackedWidget);
    connect(m_updateShortcut, SIGNAL(activated()), this, SLOT(reloadQml()));

    QFile file(":/qmldesigner/stylesheet.css");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    m_stackedWidget->setStyleSheet(styleSheet);
    m_stackedWidget->setMinimumWidth(300);
    connect(m_stackedWidget, SIGNAL(resized()), this, SLOT(updateSize()));

    m_stackedWidget->insertWidget(0, new QWidget(m_stackedWidget));


    static bool declarativeTypesRegistered = false;
    if (!declarativeTypesRegistered) {
        declarativeTypesRegistered = true;
        BasicWidgets::registerDeclarativeTypes();
        BasicLayouts::registerDeclarativeTypes();
        ResetWidget::registerDeclarativeType();
        QLayoutObject::registerDeclarativeType();
        QmlEditorWidgets::ColorWidgets::registerDeclarativeTypes();
        BehaviorDialog::registerDeclarativeType();
        QProxyLayoutItem::registerDeclarativeTypes();
        PropertyEditorValue::registerDeclarativeTypes();
        FontWidget::registerDeclarativeTypes();
        SiblingComboBox::registerDeclarativeTypes();
        OriginWidget::registerDeclarativeType();
        GradientLineQmlAdaptor::registerDeclarativeType();
    }
}

PropertyEditor::~PropertyEditor()
{
    delete m_stackedWidget;
    qDeleteAll(m_typeHash);
}

static inline QString fixTypeNameForPanes(const QString &typeName)
{
    QString fixedTypeName = typeName;
    fixedTypeName.replace(".", "/");
    fixedTypeName.replace("QtQuick/", "Qt/");
    return fixedTypeName;
}

void PropertyEditor::setupPane(const QString &typeName)
{

    QUrl qmlFile = fileToUrl(locateQmlFile(QLatin1String("Qt/ItemPane.qml")));
    QUrl qmlSpecificsFile;

    qmlSpecificsFile = fileToUrl(locateQmlFile(fixTypeNameForPanes(typeName) + "Specifics.qml"));
    NodeType *type = m_typeHash.value(qmlFile.toString());

    if (!type) {
        type = new NodeType(this);

        QDeclarativeContext *ctxt = type->m_view->rootContext();
        ctxt->setContextProperty("finishedNotify", QVariant(false) );
        type->initialSetup(typeName, qmlSpecificsFile, this);
        type->m_view->setSource(qmlFile);
        ctxt->setContextProperty("finishedNotify", QVariant(true) );

        m_stackedWidget->addWidget(type->m_view);
        m_typeHash.insert(qmlFile.toString(), type);
    } else {
        QDeclarativeContext *ctxt = type->m_view->rootContext();
        ctxt->setContextProperty("finishedNotify", QVariant(false) );

        type->initialSetup(typeName, qmlSpecificsFile, this);
        ctxt->setContextProperty("finishedNotify", QVariant(true) );
    }
}

void PropertyEditor::changeValue(const QString &propertyName)
{
    if (propertyName.isNull())
        return;

    if (m_locked)
        return;

    if (propertyName == "type")
        return;

    if (!m_selectedNode.isValid())
        return;

    if (propertyName == "id") {
        PropertyEditorValue *value = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_currentType->m_backendValuesPropertyMap.value(propertyName)));
        const QString newId = value->value().toString();

        if (m_selectedNode.isValidId(newId)) {
            if (m_selectedNode.id().isEmpty() || newId.isEmpty()) { //no id
                try {
                    m_selectedNode.setId(newId);
                } catch (InvalidIdException &e) { //better save then sorry
                    QMessageBox::warning(0, tr("Invalid Id"), e.description());
                }
            } else { //there is already an id, so we refactor
                if (rewriterView())
                    rewriterView()->renameId(m_selectedNode.id(), newId);
            }
        } else {
            value->setValue(m_selectedNode.id());
            QMessageBox::warning(0, tr("Invalid Id"),  tr("%1 is an invalid id").arg(newId));
        }
        return;
    }

    //.replace(QLatin1Char('.'), QLatin1Char('_'))
    QString underscoreName(propertyName);
    underscoreName.replace(QLatin1Char('.'), QLatin1Char('_'));
    PropertyEditorValue *value = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_currentType->m_backendValuesPropertyMap.value(underscoreName)));

    if (value ==0) {
        return;
    }

    QmlObjectNode fxObjectNode(m_selectedNode);

    QVariant castedValue;

    if (fxObjectNode.modelNode().metaInfo().isValid() && fxObjectNode.modelNode().metaInfo().hasProperty(propertyName)) {
        castedValue = fxObjectNode.modelNode().metaInfo().propertyCastedValue(propertyName, value->value());
    } else {
        qWarning() << "PropertyEditor:" <<propertyName << "cannot be casted (metainfo)";
        return ;
    }

    if (value->value().isValid() && !castedValue.isValid()) {
        qWarning() << "PropertyEditor:" << propertyName << "not properly casted (metainfo)";
        return ;
    }

    if (fxObjectNode.modelNode().metaInfo().isValid() && fxObjectNode.modelNode().metaInfo().hasProperty(propertyName))
        if (fxObjectNode.modelNode().metaInfo().propertyTypeName(propertyName) == QLatin1String("QUrl")) { //turn absolute local file paths into relative paths
        QString filePath = castedValue.toUrl().toString();
        if (QFileInfo(filePath).exists() && QFileInfo(filePath).isAbsolute()) {
            QDir fileDir(QFileInfo(model()->fileUrl().toLocalFile()).absolutePath());
            castedValue = QUrl(fileDir.relativeFilePath(filePath));
        }
    }

        if (castedValue.type() == QVariant::Color) {
            QColor color = castedValue.value<QColor>();
            QColor newColor = QColor(color.name());
            newColor.setAlpha(color.alpha());
            castedValue = QVariant(newColor);
        }

        try {
            if (!value->value().isValid()) { //reset
                fxObjectNode.removeVariantProperty(propertyName);
            } else {
                if (castedValue.isValid() && !castedValue.isNull()) {
                    m_locked = true;
                    fxObjectNode.setVariantProperty(propertyName, castedValue);
                    m_locked = false;
                }
            }
        }
        catch (RewritingException &e) {
            QMessageBox::warning(0, "Error", e.description());
        }
}

void PropertyEditor::changeExpression(const QString &name)
{
    if (name.isNull())
        return;

    if (m_locked)
        return;

    RewriterTransaction transaction = beginRewriterTransaction();

    QString underscoreName(name);
    underscoreName.replace(QLatin1Char('.'), QLatin1Char('_'));

    QmlObjectNode fxObjectNode(m_selectedNode);
    PropertyEditorValue *value = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_currentType->m_backendValuesPropertyMap.value(underscoreName)));

    if (fxObjectNode.modelNode().metaInfo().isValid() && fxObjectNode.modelNode().metaInfo().hasProperty(name)) {
        if (fxObjectNode.modelNode().metaInfo().propertyTypeName(name) == QLatin1String("QColor")) {
            if (QColor(value->expression().remove('"')).isValid()) {
                fxObjectNode.setVariantProperty(name, QColor(value->expression().remove('"')));
                return;
            }
        } else if (fxObjectNode.modelNode().metaInfo().propertyTypeName(name) == QLatin1String("bool")) {
            if (value->expression().compare("false", Qt::CaseInsensitive) == 0 || value->expression().compare("true", Qt::CaseInsensitive) == 0) {
                if (value->expression().compare("true", Qt::CaseInsensitive) == 0)
                    fxObjectNode.setVariantProperty(name, true);
                else
                    fxObjectNode.setVariantProperty(name, false);
                return;
            }
        } else if (fxObjectNode.modelNode().metaInfo().propertyTypeName(name) == QLatin1String("int")) {
            bool ok;
            int intValue = value->expression().toInt(&ok);
            if (ok) {
                fxObjectNode.setVariantProperty(name, intValue);
                return;
            }
        } else if (fxObjectNode.modelNode().metaInfo().propertyTypeName(name) == QLatin1String("qreal")) {
            bool ok;
            qreal realValue = value->expression().toFloat(&ok);
            if (ok) {
                fxObjectNode.setVariantProperty(name, realValue);
                return;
            }
        }
    }

    if (!value) {
        qWarning() << "PropertyEditor::changeExpression no value for " << underscoreName;
        return;
    }

    try {
        if (fxObjectNode.expression(name) != value->expression() || !fxObjectNode.propertyAffectedByCurrentState(name))
            fxObjectNode.setBindingProperty(name, value->expression());
    }

    catch (RewritingException &e) {
        QMessageBox::warning(0, "Error", e.description());
    }
}

void PropertyEditor::updateSize()
{
    if (!m_currentType)
        return;
    QWidget* frame = m_currentType->m_view->findChild<QWidget*>("propertyEditorFrame");
    if (frame)
        frame->resize(m_stackedWidget->size());
}

void PropertyEditor::setupPanes()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    setupPane("QtQuick.Rectangle");
    setupPane("QtQuick.Text");
    resetView();
    m_setupCompleted = true;
    QApplication::restoreOverrideCursor();
}

void PropertyEditor::otherPropertyChanged(const QmlObjectNode &fxObjectNode, const QString &propertyName)
{
    QmlModelView::otherPropertyChanged(fxObjectNode, propertyName);

    if (fxObjectNode.isValid() && m_currentType && fxObjectNode == m_selectedNode && fxObjectNode.currentState().isValid()) {
        AbstractProperty property = fxObjectNode.modelNode().property(propertyName);
        if (fxObjectNode == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == fxObjectNode) {
            if ( !m_selectedNode.hasProperty(propertyName) || m_selectedNode.property(property.name()).isBindingProperty() )
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            else
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).modelValue(property.name()));
        }
    }
}

void PropertyEditor::transformChanged(const QmlObjectNode &fxObjectNode, const QString &propertyName)
{
    QmlModelView::transformChanged(fxObjectNode, propertyName);

    if (fxObjectNode.isValid() && m_currentType && fxObjectNode == m_selectedNode && fxObjectNode.currentState().isValid()) {
        AbstractProperty property = fxObjectNode.modelNode().property(propertyName);
        if (fxObjectNode == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == fxObjectNode) {
            if ( m_selectedNode.property(property.name()).isBindingProperty() || !m_selectedNode.hasProperty(propertyName))
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            else
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).modelValue(property.name()));
        }
    }
}

void PropertyEditor::setQmlDir(const QString &qmlDir)
{
    m_qmlDir = qmlDir;

    QFileSystemWatcher *watcher = new QFileSystemWatcher(this);
    watcher->addPath(m_qmlDir);
    connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(reloadQml()));
}

void PropertyEditor::delayedResetView()
{
    if (m_timerId == 0)
        m_timerId = startTimer(200);
}

void PropertyEditor::timerEvent(QTimerEvent *timerEvent)
{
    if (m_timerId == timerEvent->timerId()) {
        resetView();
    }
}

QString templateGeneration(NodeMetaInfo type, NodeMetaInfo superType, const QmlObjectNode &objectNode)
{
    QString qmlTemplate = QLatin1String("import Qt 4.7\nimport Bauhaus 1.0\n");
    qmlTemplate += QLatin1String("GroupBox {\n");
    qmlTemplate += QString(QLatin1String("caption: \"%1\"\n")).arg(type.typeName());
    qmlTemplate += QLatin1String("layout: VerticalLayout {\n");

    QList<QString> orderedList;
    orderedList = type.propertyNames();
    qSort(orderedList);

    foreach (const QString &name, orderedList) {
        QString properName = name;
        properName.replace('.', '_');

        QString typeName = type.propertyTypeName(name);
        //alias resolution only possible with instance
            if (typeName == QLatin1String("alias") && objectNode.isValid())
                typeName = objectNode.instanceType(name);

        if (!superType.hasProperty(name) && type.propertyIsWritable(name)) {
            if (typeName == "int") {
                qmlTemplate +=  QString(QLatin1String(
                "IntEditor { backendValue: backendValues.%2\n caption: \"%1\"\nbaseStateFlag: isBaseState\nslider: false\n}"
                )).arg(name).arg(properName);
            }
            if (typeName == "real" || typeName == "double" || typeName == "qreal") {
                qmlTemplate +=  QString(QLatin1String(
                "DoubleSpinBoxAlternate {\ntext: \"%1\"\nbackendValue: backendValues.%2\nbaseStateFlag: isBaseState\n}\n"
                )).arg(name).arg(properName);
            }
            if (typeName == "string" || typeName == "QString" || typeName == "QUrl" || typeName == "url") {
                 qmlTemplate +=  QString(QLatin1String(
                "QWidget {\nlayout: HorizontalLayout {\nLabel {\ntext: \"%1\"\ntoolTip: \"%1\"\n}\nLineEdit {\nbackendValue: backendValues.%2\nbaseStateFlag: isBaseState\n}\n}\n}\n"
                )).arg(name).arg(properName);
            }
            if (typeName == "bool") {
                 qmlTemplate +=  QString(QLatin1String(
                 "QWidget {\nlayout: HorizontalLayout {\nLabel {\ntext: \"%1\"\ntoolTip: \"%1\"\n}\nCheckBox {text: backendValues.%2.value\nbackendValue: backendValues.%2\nbaseStateFlag: isBaseState\ncheckable: true\n}\n}\n}\n"
                 )).arg(name).arg(properName);
            }
            if (typeName == "color" || typeName == "QColor") {
                qmlTemplate +=  QString(QLatin1String(
                "ColorGroupBox {\ncaption: \"%1\"\nfinished: finishedNotify\nbackendColor: backendValues.%2\n}\n\n"
                )).arg(name).arg(properName);
            }
        }
    }
    qmlTemplate += QLatin1String("}\n"); //VerticalLayout
    qmlTemplate += QLatin1String("}\n"); //GroupBox

    return qmlTemplate;
}

void PropertyEditor::resetView()
{
    if (model() == 0)
        return;

    m_locked = true;

    if (debug)
        qDebug() << "________________ RELOADING PROPERTY EDITOR QML _______________________";

    if (m_timerId)
        killTimer(m_timerId);

    if (m_selectedNode.isValid() && model() != m_selectedNode.model())
        m_selectedNode = ModelNode();

    QString specificsClassName;
    QUrl qmlFile(qmlForNode(m_selectedNode, specificsClassName));
    QUrl qmlSpecificsFile;
    if (m_selectedNode.isValid())
        qmlSpecificsFile = fileToUrl(locateQmlFile(fixTypeNameForPanes(m_selectedNode.type()) + "Specifics.qml"));

    QString specificQmlData;

    if (m_selectedNode.isValid() && !QFileInfo(qmlSpecificsFile.toLocalFile()).exists() && m_selectedNode.metaInfo().isValid()) {
        //do magic !!
        specificQmlData = templateGeneration(m_selectedNode.metaInfo(), model()->metaInfo(specificsClassName), m_selectedNode);
    }

    NodeType *type = m_typeHash.value(qmlFile.toString());

    if (!type) {
        type = new NodeType(this);

        m_stackedWidget->addWidget(type->m_view);
        m_typeHash.insert(qmlFile.toString(), type);

        QmlObjectNode fxObjectNode;
        if (m_selectedNode.isValid()) {
            fxObjectNode = QmlObjectNode(m_selectedNode);
            Q_ASSERT(fxObjectNode.isValid());
        }
        QDeclarativeContext *ctxt = type->m_view->rootContext();
        type->setup(fxObjectNode, currentState().name(), qmlSpecificsFile, this);
        ctxt->setContextProperty("finishedNotify", QVariant(false));
        if (specificQmlData.isEmpty())
            type->m_contextObject->setSpecificQmlData(specificQmlData);
            
        type->m_contextObject->setGlobalBaseUrl(qmlFile);
        type->m_contextObject->setSpecificQmlData(specificQmlData);
        type->m_view->setSource(qmlFile);
        ctxt->setContextProperty("finishedNotify", QVariant(true));
    } else {
        QmlObjectNode fxObjectNode;
        if (m_selectedNode.isValid()) {
            fxObjectNode = QmlObjectNode(m_selectedNode);
        }
        QDeclarativeContext *ctxt = type->m_view->rootContext();
        
        ctxt->setContextProperty("finishedNotify", QVariant(false));
        if (specificQmlData.isEmpty())
            type->m_contextObject->setSpecificQmlData(specificQmlData);
        
        type->setup(fxObjectNode, currentState().name(), qmlSpecificsFile, this);
        type->m_contextObject->setGlobalBaseUrl(qmlFile);
        type->m_contextObject->setSpecificQmlData(specificQmlData);
    }

    m_stackedWidget->setCurrentWidget(type->m_view);


    QDeclarativeContext *ctxt = type->m_view->rootContext();
    ctxt->setContextProperty("finishedNotify", QVariant(true));
    /*ctxt->setContextProperty("selectionChanged", QVariant(false));
    ctxt->setContextProperty("selectionChanged", QVariant(true));
    ctxt->setContextProperty("selectionChanged", QVariant(false));*/
    type->m_contextObject->triggerSelectionChanged();

    m_currentType = type;

    m_locked = false;

    if (m_timerId)
        m_timerId = 0;

    updateSize();
}

void PropertyEditor::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                          const QList<ModelNode> &lastSelectedNodeList)
{
    Q_UNUSED(lastSelectedNodeList);

    if (selectedNodeList.isEmpty() || selectedNodeList.count() > 1)
        select(ModelNode());
    else if (m_selectedNode != selectedNodeList.first())
        select(selectedNodeList.first());
}

void PropertyEditor::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    QmlModelView::nodeAboutToBeRemoved(removedNode);
    if (m_selectedNode.isValid() && removedNode.isValid() && m_selectedNode == removedNode)
        select(m_selectedNode.parentProperty().parentModelNode());
}

void PropertyEditor::modelAttached(Model *model)
{
    QmlModelView::modelAttached(model);

    if (debug)
        qDebug() << Q_FUNC_INFO;

    m_locked = true;

    resetView();
    if (!m_setupCompleted) {
        m_singleShotTimer->setSingleShot(true);
        m_singleShotTimer->setInterval(1000);
        connect(m_singleShotTimer, SIGNAL(timeout()), this, SLOT(setupPanes()));
        m_singleShotTimer->start();
    }

    m_locked = false;
}

void PropertyEditor::modelAboutToBeDetached(Model *model)
{
    QmlModelView::modelAboutToBeDetached(model);
    m_currentType->m_propertyEditorTransaction->end();

    resetView();
}


void PropertyEditor::propertiesRemoved(const QList<AbstractProperty>& propertyList)
{
    QmlModelView::propertiesRemoved(propertyList);

    if (!m_selectedNode.isValid())
        return;

    foreach (const AbstractProperty &property, propertyList) {
        ModelNode node(property.parentModelNode());
        if (node == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == node) {
            setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            if (property.name().contains("anchor", Qt::CaseInsensitive))
                m_currentType->m_backendAnchorBinding.invalidate(m_selectedNode);
        }
    }
}


void PropertyEditor::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange)
{

    QmlModelView::variantPropertiesChanged(propertyList, propertyChange);

    if (!m_selectedNode.isValid())
        return;

    foreach (const VariantProperty &property, propertyList) {
        ModelNode node(property.parentModelNode());

        if (node == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == node) {
            if ( QmlObjectNode(m_selectedNode).modelNode().property(property.name()).isBindingProperty())
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            else
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).modelValue(property.name()));
        }
    }
}

void PropertyEditor::bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    QmlModelView::bindingPropertiesChanged(propertyList, propertyChange);

    if (!m_selectedNode.isValid())
        return;

    foreach (const BindingProperty &property, propertyList) {
        ModelNode node(property.parentModelNode());

        if (node == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == node) {
            if (property.name().contains("anchor", Qt::CaseInsensitive))
                m_currentType->m_backendAnchorBinding.invalidate(m_selectedNode);
            if ( QmlObjectNode(m_selectedNode).modelNode().property(property.name()).isBindingProperty())
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            else
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).modelValue(property.name()));
        }
    }
}

void PropertyEditor::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId)
{
    QmlModelView::nodeIdChanged(node, newId, oldId);

    if (!m_selectedNode.isValid())
        return;

    if (node == m_selectedNode) {

        if (m_currentType) {
            setValue(node, "id", newId);
        }
    }
}

void PropertyEditor::scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList)
{
    QmlModelView::scriptFunctionsChanged(node, scriptFunctionList);
}

void PropertyEditor::select(const ModelNode &node)
{
    if (QmlItemNode(node).isValid())
        m_selectedNode = node;
    else
        m_selectedNode = ModelNode();

    delayedResetView();
}

QWidget *PropertyEditor::createPropertiesPage()
{
    delayedResetView();
    return m_stackedWidget;
}

void PropertyEditor::stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState)
{
    QmlModelView::stateChanged(newQmlModelState, oldQmlModelState);
    Q_ASSERT(newQmlModelState.isValid());
    if (debug)
        qDebug() << Q_FUNC_INFO << newQmlModelState.name();
    delayedResetView();
}

void PropertyEditor::setValue(const QmlObjectNode &fxObjectNode, const QString &name, const QVariant &value)
{
    m_locked = true;
    m_currentType->setValue(fxObjectNode, name, value);
    m_locked = false;
}

void PropertyEditor::reloadQml()
{
    m_typeHash.clear();
    while (QWidget *widget = m_stackedWidget->widget(0)) {
        m_stackedWidget->removeWidget(widget);
        delete widget;
    }
    m_currentType = 0;

    delayedResetView();
}

QString PropertyEditor::qmlFileName(const NodeMetaInfo &nodeInfo) const
{
    if (nodeInfo.typeName().split('.').last() == "QDeclarativeItem")
        return "Qt/ItemPane.qml";    
    const QString fixedTypeName = fixTypeNameForPanes(nodeInfo.typeName());
    return fixedTypeName + QLatin1String("Pane.qml");
}

QUrl PropertyEditor::fileToUrl(const QString &filePath) const {
    QUrl fileUrl;

    if (filePath.isEmpty())
        return fileUrl;

    if (filePath.startsWith(QLatin1Char(':'))) {
        fileUrl.setScheme("qrc");
        QString path = filePath;
        path.remove(0, 1); // remove trailing ':'
        fileUrl.setPath(path);
    } else {
        fileUrl = QUrl::fromLocalFile(filePath);
    }

    return fileUrl;
}

QUrl PropertyEditor::qmlForNode(const ModelNode &modelNode, QString &className) const
{
    if (modelNode.isValid()) {
        QList<NodeMetaInfo> hierarchy;
        hierarchy << modelNode.metaInfo();
        hierarchy << modelNode.metaInfo().superClasses();

        foreach (const NodeMetaInfo &info, hierarchy) {
            QUrl fileUrl = fileToUrl(locateQmlFile(qmlFileName(info)));
            if (fileUrl.isValid()) {
                className = info.typeName();
                return fileUrl;
            }
        }
    }
    return fileToUrl(QDir(m_qmlDir).filePath("Qt/emptyPane.qml"));
}

QString PropertyEditor::locateQmlFile(const QString &relativePath) const
{
    QDir fileSystemDir(m_qmlDir);
    static QDir resourcesDir(":/propertyeditor");

    if (fileSystemDir.exists(relativePath))
        return fileSystemDir.absoluteFilePath(relativePath);
    if (resourcesDir.exists(relativePath))
        return resourcesDir.absoluteFilePath(relativePath);

    return QString();
}


} //QmlDesigner

