/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QmlPropertyView_h
#define QmlPropertyView_h

#include <qmlmodelview.h>
#include <declarativewidgetview.h>
#include <QHash>
#include <QDeclarativePropertyMap>
#include <QStackedWidget>
#include <QTimer>

#include "qmlanchorbindingproxy.h"
#include "designerpropertymap.h"
#include "propertyeditorvalue.h"
#include "propertyeditorcontextobject.h"

QT_BEGIN_NAMESPACE
class QShortcut;
class QStackedWidget;
class QTimer;
QT_END_NAMESPACE

class PropertyEditorValue;

namespace QmlDesigner {

class PropertyEditorTransaction;
class CollapseButton;
class StackedWidget;

class PropertyEditor: public QmlModelView
{
    Q_OBJECT

    class NodeType {
    public:
        NodeType(PropertyEditor *propertyEditor);
        ~NodeType();

        void setup(const QmlObjectNode &fxObjectNode, const QString &stateName, const QUrl &qmlSpecificsFile, PropertyEditor *propertyEditor);
        void initialSetup(const QString &typeName, const QUrl &qmlSpecificsFile, PropertyEditor *propertyEditor);
        void setValue(const QmlObjectNode &fxObjectNode, const QString &name, const QVariant &value);

        DeclarativeWidgetView *m_view;
        Internal::QmlAnchorBindingProxy m_backendAnchorBinding;
        DesignerPropertyMap<PropertyEditorValue> m_backendValuesPropertyMap;
        QScopedPointer<PropertyEditorTransaction> m_propertyEditorTransaction;
        QScopedPointer<PropertyEditorValue> m_dummyPropertyEditorValue;
        QScopedPointer<PropertyEditorContextObject> m_contextObject;
    };

public:
    PropertyEditor(QWidget *parent = 0);
    ~PropertyEditor();

    void setQmlDir(const QString &qmlDirPath);

    QWidget* createPropertiesPage();

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList);
    void nodeAboutToBeRemoved(const ModelNode &removedNode);

    void propertiesAdded(const NodeState &state, const QList<NodeProperty>& propertyList);
    void propertiesRemoved(const QList<AbstractProperty>& propertyList);
    void propertyValuesChanged(const NodeState &state, const QList<NodeProperty>& propertyList);

    void modelAttached(Model *model);

    void modelAboutToBeDetached(Model *model);

    ModelState modelState() const;

    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange);

    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);

protected:
    void timerEvent(QTimerEvent *event);
    void otherPropertyChanged(const QmlObjectNode &, const QString &propertyName);
    void transformChanged(const QmlObjectNode &qmlObjectNode, const QString &propertyName);
    void stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState);

    void setupPane(const QString &typeName);
    void setValue(const QmlObjectNode &fxObjectNode, const QString &name, const QVariant &value);

private slots:
    void reloadQml();
    void changeValue(const QString &name);
    void changeExpression(const QString &name);
    void updateSize();
    void setupPanes();

private: //functions
    QString qmlFileName(const NodeMetaInfo &nodeInfo) const;
    QUrl fileToUrl(const QString &filePath) const;
    QUrl qmlForNode(const ModelNode &modelNode, QString &className) const;
    QString locateQmlFile(const QString &relativePath) const;
    void select(const ModelNode& node);

    void resetView();
    void delayedResetView();


private: //variables
    ModelNode m_selectedNode;
    QWidget *m_parent;
    QShortcut *m_updateShortcut;
    int m_timerId;
    StackedWidget* m_stackedWidget;
    QString m_qmlDir;
    QHash<QString, NodeType *> m_typeHash;
    NodeType *m_currentType;
    bool m_locked;
    bool m_setupCompleted;
    QTimer *m_singleShotTimer;
};


class StackedWidget : public QStackedWidget
{
Q_OBJECT

public:
    StackedWidget(QWidget *parent = 0) : QStackedWidget(parent) {}

signals:
    void resized();
protected:
    void resizeEvent(QResizeEvent * event)
    {
        QStackedWidget::resizeEvent(event);
        emit resized();
    }
};
}

#endif // QmlPropertyView_h
