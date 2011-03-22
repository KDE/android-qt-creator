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

#include "abstractproperty.h"
#include "bindingproperty.h"
#include "filemanager/firstdefinitionfinder.h"
#include "filemanager/objectlengthcalculator.h"
#include "filemanager/qmlrefactoring.h"
#include "rewriteaction.h"
#include "nodeproperty.h"
#include "propertyparser.h"
#include "textmodifier.h"
#include "texttomodelmerger.h"
#include "rewriterview.h"
#include "variantproperty.h"

#include <languageutils/componentversion.h>
#include <qmljs/qmljsevaluate.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljscheck.h>

#include <QtCore/QSet>
#include <QtGui/QMessageBox>

using namespace LanguageUtils;
using namespace QmlJS;
using namespace QmlJS::AST;

namespace {

static inline QString stripQuotes(const QString &str)
{
    if ((str.startsWith(QLatin1Char('"')) && str.endsWith(QLatin1Char('"')))
            || (str.startsWith(QLatin1Char('\'')) && str.endsWith(QLatin1Char('\''))))
        return str.mid(1, str.length() - 2);

    return str;
}

static inline QString deEscape(const QString &value)
{
    QString result = value;

    result.replace(QLatin1String("\\\\"), QLatin1String("\\"));
    result.replace(QLatin1String("\\\""), QLatin1String("\""));
    result.replace(QLatin1String("\\\t"), QLatin1String("\t"));
    result.replace(QLatin1String("\\\r"), QLatin1String("\\\r"));
    result.replace(QLatin1String("\\\n"), QLatin1String("\n"));

    return result;
}
 
static inline int fixUpMajorVersionForQt(const QString &value, int i)
{
    if (i == 4 && value == "Qt")
        return 1;
    else return i;
}

static inline int fixUpMinorVersionForQt(const QString &value, int i)
{
    if (i == 7 && value == "Qt")
        return 0;
    else return i;
}

static inline QString fixUpPackeNameForQt(const QString &value)
{
    if (value == "Qt")
        return "QtQuick";
    return value;
}

static inline bool isSignalPropertyName(const QString &signalName)
{
    // see QmlCompiler::isSignalPropertyName
    return signalName.length() >= 3 && signalName.startsWith(QLatin1String("on")) &&
           signalName.at(2).isLetter();
}

static inline QVariant cleverConvert(const QString &value)
{
    if (value == "true")
        return QVariant(true);
    if (value == "false")
        return QVariant(false);
    bool flag;
    int i = value.toInt(&flag);
    if (flag)
        return QVariant(i);
    double d = value.toDouble(&flag);
    if (flag)
        return QVariant(d);
    return QVariant(value);
}

static QString flatten(UiQualifiedId *qualifiedId)
{
    QString result;

    for (UiQualifiedId *iter = qualifiedId; iter; iter = iter->next) {
        if (!iter->name)
            continue;

        if (!result.isEmpty())
            result.append(QLatin1Char('.'));

        result.append(iter->name->asString());
    }

    return result;
}

static bool isLiteralValue(ExpressionNode *expr)
{
    if (cast<NumericLiteral*>(expr))
        return true;
    else if (cast<StringLiteral*>(expr))
        return true;
    else if (UnaryPlusExpression *plusExpr = cast<UnaryPlusExpression*>(expr))
        return isLiteralValue(plusExpr->expression);
    else if (UnaryMinusExpression *minusExpr = cast<UnaryMinusExpression*>(expr))
        return isLiteralValue(minusExpr->expression);
    else if (cast<TrueLiteral*>(expr))
        return true;
    else if (cast<FalseLiteral*>(expr))
        return true;
    else
        return false;
}

static inline bool isLiteralValue(UiScriptBinding *script)
{
    if (!script || !script->statement)
        return false;

    ExpressionStatement *exprStmt = cast<ExpressionStatement *>(script->statement);
    if (exprStmt)
        return isLiteralValue(exprStmt->expression);
    else
        return false;
}

static inline int propertyType(const QString &typeName)
{
    if (typeName == QLatin1String("bool"))
        return QMetaType::type("bool");
    else if (typeName == QLatin1String("color"))
        return QMetaType::type("QColor");
    else if (typeName == QLatin1String("date"))
        return QMetaType::type("QDate");
    else if (typeName == QLatin1String("int"))
        return QMetaType::type("int");
    else if (typeName == QLatin1String("real"))
        return QMetaType::type("double");
    else if (typeName == QLatin1String("double"))
        return QMetaType::type("double");
    else if (typeName == QLatin1String("string"))
        return QMetaType::type("QString");
    else if (typeName == QLatin1String("url"))
        return QMetaType::type("QUrl");
    else if (typeName == QLatin1String("var") || typeName == QLatin1String("variant"))
        return QMetaType::type("QVariant");
    else
        return -1;
}

static inline QVariant convertDynamicPropertyValueToVariant(const QString &astValue,
                                                            const QString &astType)
{
    const QString cleanedValue = deEscape(stripQuotes(astValue.trimmed()));

    if (astType.isEmpty())
        return QString();

    const int type = propertyType(astType);
    if (type == QMetaType::type("QVariant")) {
        if (cleanedValue.isNull()) // Explicitly isNull, NOT isEmpty!
            return QVariant(static_cast<QVariant::Type>(type));
        else
            return QVariant(cleanedValue);
    } else {
        QVariant value = QVariant(cleanedValue);
        value.convert(static_cast<QVariant::Type>(type));
        return value;
    }
}

static bool isComponentType(const QString &type)
{
    return  type == QLatin1String("Component") || type == QLatin1String("Qt.Component") || type == QLatin1String("QtQuick.Component");
}

static bool isPropertyChangesType(const QString &type)
{
    return  type == QLatin1String("PropertyChanges") || type == QLatin1String("QtQuick.PropertyChanges") || type == QLatin1String("Qt.PropertyChanges");
}


} // anonymous namespace

namespace QmlDesigner {
namespace Internal {

class ReadingContext
{
public:
    ReadingContext(const Snapshot &snapshot, const Document::Ptr &doc,
                   const QStringList importPaths)
        : m_snapshot(snapshot)
        , m_doc(doc)
        , m_context(new Interpreter::Context)
        , m_link(m_context, doc, snapshot, importPaths)
        , m_lookupContext(LookupContext::create(doc, snapshot, *m_context, QList<AST::Node*>()))
        , m_scopeBuilder(m_context, doc, snapshot)
    {
    }

    ~ReadingContext()
    { delete m_context; }

    Document::Ptr doc() const
    { return m_doc; }

    void enterScope(Node *node)
    { m_scopeBuilder.push(node); }

    void leaveScope()
    { m_scopeBuilder.pop(); }

    void lookup(UiQualifiedId *astTypeNode, QString &typeName, int &majorVersion,
                int &minorVersion, QString &defaultPropertyName)
    {
        const Interpreter::ObjectValue *value = m_context->lookupType(m_doc.data(), astTypeNode);
        defaultPropertyName = m_context->defaultPropertyName(value);

        const Interpreter::QmlObjectValue * qmlValue = dynamic_cast<const Interpreter::QmlObjectValue *>(value);
        if (qmlValue) {
            typeName = fixUpPackeNameForQt(qmlValue->packageName()) + QLatin1String(".") + qmlValue->className();

            //### todo this is just a hack to support QtQuick 1.0
            majorVersion = fixUpMajorVersionForQt(qmlValue->packageName(), qmlValue->version().majorVersion());
            minorVersion = fixUpMinorVersionForQt(qmlValue->packageName(), qmlValue->version().minorVersion());
        } else {
            for (UiQualifiedId *iter = astTypeNode; iter; iter = iter->next)
                if (!iter->next && iter->name)
                    typeName = iter->name->asString();
            majorVersion = ComponentVersion::NoVersion;
            minorVersion = ComponentVersion::NoVersion;
        }
    }

    /// When something is changed here, also change Check::checkScopeObjectMember in
    /// qmljscheck.cpp
    /// ### Maybe put this into the context as a helper method.
    bool lookupProperty(const QString &prefix, const UiQualifiedId *id, const Interpreter::Value **property = 0, const Interpreter::ObjectValue **parentObject = 0, QString *name = 0)
    {
        QList<const Interpreter::ObjectValue *> scopeObjects = m_context->scopeChain().qmlScopeObjects;
        if (scopeObjects.isEmpty())
            return false;

        if (! id)
            return false; // ### error?

        if (! id->name) // possible after error recovery
            return false;

        QString propertyName;
        if (prefix.isEmpty())
            propertyName = id->name->asString();
        else
            propertyName = prefix;

        if (name)
            *name = propertyName;

        if (propertyName == QLatin1String("id") && ! id->next)
            return false; // ### should probably be a special value

        // attached properties
        bool isAttachedProperty = false;
        if (! propertyName.isEmpty() && propertyName[0].isUpper()) {
            isAttachedProperty = true;
            if (const Interpreter::ObjectValue *qmlTypes = m_context->scopeChain().qmlTypes)
                scopeObjects += qmlTypes;
        }

        if (scopeObjects.isEmpty())
            return false;

        // global lookup for first part of id
        const Interpreter::ObjectValue *objectValue = 0;
        const Interpreter::Value *value = 0;
        for (int i = scopeObjects.size() - 1; i >= 0; --i) {
            objectValue = scopeObjects[i];
            value = objectValue->lookupMember(propertyName, m_context);
            if (value)
                break;
        }
        if (parentObject)
            *parentObject = objectValue;
        if (!value) {
            qWarning() << "Skipping invalid property name" << propertyName;
            return false;
        }

        // can't look up members for attached properties
        if (isAttachedProperty)
            return false;

        // member lookup
        const UiQualifiedId *idPart = id;
        if (prefix.isEmpty())
            idPart = idPart->next;
        for (; idPart; idPart = idPart->next) {
            objectValue = Interpreter::value_cast<const Interpreter::ObjectValue *>(value);
            if (! objectValue) {
//                if (idPart->name)
//                    qDebug() << idPart->name->asString() << "has no property named"
//                             << propertyName;
                return false;
            }
            if (parentObject)
                *parentObject = objectValue;

            if (! idPart->name) {
                // somebody typed "id." and error recovery still gave us a valid tree,
                // so just bail out here.
                return false;
            }

            propertyName = idPart->name->asString();
            if (name)
                *name = propertyName;

            value = objectValue->lookupMember(propertyName, m_context);
            if (! value) {
//                if (idPart->name)
//                    qDebug() << "In" << idPart->name->asString() << ":"
//                             << objectValue->className() << "has no property named"
//                             << propertyName;
                return false;
            }
        }

        if (property)
            *property = value;
        return true;
    }

    bool isArrayProperty(const Interpreter::Value *value, const Interpreter::ObjectValue *containingObject, const QString &name)
    {
        if (!value)
            return false;
        const Interpreter::ObjectValue *objectValue = value->asObjectValue();
        if (objectValue && objectValue->prototype(m_context) == m_context->engine()->arrayPrototype())
            return true;

        Interpreter::PrototypeIterator iter(containingObject, m_context);
        while (iter.hasNext()) {
            const Interpreter::ObjectValue *proto = iter.next();
            if (proto->property(name, m_context) == m_context->engine()->arrayPrototype())
                return true;
            if (const Interpreter::QmlObjectValue *qmlIter = dynamic_cast<const Interpreter::QmlObjectValue *>(proto)) {
                if (qmlIter->isListProperty(name))
                    return true;
            }
        }
        return false;
    }

    QVariant convertToVariant(const QString &astValue, const QString &propertyPrefix, UiQualifiedId *propertyId)
    {
        const bool hasQuotes = astValue.trimmed().left(1) == QLatin1String("\"") && astValue.trimmed().right(1) == QLatin1String("\"");
        const QString cleanedValue = deEscape(stripQuotes(astValue.trimmed()));
        const Interpreter::Value *property = 0;
        const Interpreter::ObjectValue *containingObject = 0;
        QString name;
        if (!lookupProperty(propertyPrefix, propertyId, &property, &containingObject, &name)) {
            qWarning() << "Unknown property" << propertyPrefix + QLatin1Char('.') + flatten(propertyId)
                       << "on line" << propertyId->identifierToken.startLine
                       << "column" << propertyId->identifierToken.startColumn;
            return hasQuotes ? QVariant(cleanedValue) : cleverConvert(cleanedValue);
        }

        if (containingObject)
            containingObject->lookupMember(name, m_context, &containingObject);

        if (const Interpreter::QmlObjectValue * qmlObject = dynamic_cast<const Interpreter::QmlObjectValue *>(containingObject)) {
            const QString typeName = qmlObject->propertyType(name);
            if (qmlObject->isEnum(typeName)) {
                return QVariant(cleanedValue);
            } else {
                int type = QMetaType::type(typeName.toUtf8().constData());
                QVariant result;
                if (type)
                    result = PropertyParser::read(type, cleanedValue);
                if (result.isValid())
                    return result;
            }
        }

        QVariant v(cleanedValue);
        if (property->asBooleanValue()) {
            v.convert(QVariant::Bool);
        } else if (property->asColorValue()) {
            v.convert(QVariant::Color);
        } else if (property->asNumberValue()) {
            v.convert(QVariant::Double);
        } else if (property->asStringValue()) {
            // nothing to do
        } else { //property alias et al
            if (!hasQuotes)
                return cleverConvert(cleanedValue);
        }
        return v;
    }

    QVariant convertToEnum(Statement *rhs, const QString &propertyPrefix, UiQualifiedId *propertyId)
    {
        ExpressionStatement *eStmt = cast<ExpressionStatement *>(rhs);
        if (!eStmt || !eStmt->expression)
            return QVariant();

        const Interpreter::ObjectValue *containingObject = 0;
        QString name;
        if (!lookupProperty(propertyPrefix, propertyId, 0, &containingObject, &name)) {
            return QVariant();
        }

        if (containingObject)
            containingObject->lookupMember(name, m_context, &containingObject);
        const Interpreter::QmlObjectValue * lhsQmlObject = dynamic_cast<const Interpreter::QmlObjectValue *>(containingObject);
        if (!lhsQmlObject)
            return QVariant();
        const QString lhsPropertyTypeName = lhsQmlObject->propertyType(name);

        const Interpreter::ObjectValue *rhsValueObject = 0;
        QString rhsValueName;
        if (IdentifierExpression *idExp = cast<IdentifierExpression *>(eStmt->expression)) {
            if (!m_context->scopeChain().qmlScopeObjects.isEmpty())
                rhsValueObject = m_context->scopeChain().qmlScopeObjects.last();
            if (idExp->name)
                rhsValueName = idExp->name->asString();
        } else if (FieldMemberExpression *memberExp = cast<FieldMemberExpression *>(eStmt->expression)) {
            Evaluate evaluate(m_context);
            const Interpreter::Value *result = evaluate(memberExp->base);
            rhsValueObject = result->asObjectValue();

            if (memberExp->name)
                rhsValueName = memberExp->name->asString();
        }

        if (rhsValueObject)
            rhsValueObject->lookupMember(rhsValueName, m_context, &rhsValueObject);

        const Interpreter::QmlObjectValue *rhsQmlObjectValue = dynamic_cast<const Interpreter::QmlObjectValue *>(rhsValueObject);
        if (!rhsQmlObjectValue)
            return QVariant();

        if (rhsQmlObjectValue->enumContainsKey(lhsPropertyTypeName, rhsValueName))
            return QVariant(rhsValueName);
        else
            return QVariant();
    }


    LookupContext::Ptr lookupContext() const
    { return m_lookupContext; }

    QList<DiagnosticMessage> diagnosticLinkMessages() const
    { return m_link.diagnosticMessages(); }

private:
    Snapshot m_snapshot;
    Document::Ptr m_doc;
    Interpreter::Context *m_context;
    Link m_link;
    LookupContext::Ptr m_lookupContext;
    ScopeBuilder m_scopeBuilder;
};

} // namespace Internal
} // namespace QmlDesigner

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

static inline bool equals(const QVariant &a, const QVariant &b)
{
    if (a.type() == QVariant::Double && b.type() == QVariant::Double)
        return qFuzzyCompare(a.toDouble(), b.toDouble());
    else
        return a == b;
}

TextToModelMerger::TextToModelMerger(RewriterView *reWriterView):
        m_rewriterView(reWriterView),
        m_isActive(false)
{
    Q_ASSERT(reWriterView);
}

void TextToModelMerger::setActive(bool active)
{
    m_isActive = active;
}

bool TextToModelMerger::isActive() const
{
    return m_isActive;
}

void TextToModelMerger::setupImports(const Document::Ptr &doc,
                                     DifferenceHandler &differenceHandler)
{
    QList<Import> existingImports = m_rewriterView->model()->imports();

    for (UiImportList *iter = doc->qmlProgram()->imports; iter; iter = iter->next) {
        UiImport *import = iter->import;
        if (!import)
            continue;

        QString version;
        if (import->versionToken.isValid())
            version = textAt(doc, import->versionToken);
        QString as;
        if (import->importId)
            as = import->importId->asString();

        if (import->fileName) {
            const QString strippedFileName = stripQuotes(import->fileName->asString());
            const Import newImport = Import::createFileImport(strippedFileName,
                                                              version, as, m_rewriterView->textModifier()->importPaths());

            if (!existingImports.removeOne(newImport))
                differenceHandler.modelMissesImport(newImport);
        } else {
            QString importUri = flatten(import->importUri);
            if (importUri == QLatin1String("Qt") && version == QLatin1String("4.7")) {
                importUri = QLatin1String("QtQuick");
                version = QLatin1String("1.0");
            }

            const Import newImport =
                    Import::createLibraryImport(importUri, version, as, m_rewriterView->textModifier()->importPaths());

            if (!existingImports.removeOne(newImport))
                differenceHandler.modelMissesImport(newImport);
        }
    }

    foreach (const Import &import, existingImports)
        differenceHandler.importAbsentInQMl(import);
}

bool TextToModelMerger::load(const QString &data, DifferenceHandler &differenceHandler)
{
//    qDebug() << "TextToModelMerger::load with data:" << data;

    const QUrl url = m_rewriterView->model()->fileUrl();
    const QStringList importPaths = m_rewriterView->textModifier()->importPaths();
    setActive(true);


    try {
        Snapshot snapshot = m_rewriterView->textModifier()->getSnapshot();
        const QString fileName = url.toLocalFile();
        Document::Ptr doc = Document::create(fileName.isEmpty() ? QLatin1String("<internal>") : fileName);
        doc->setSource(data);
        doc->parseQml();

        if (!doc->isParsedCorrectly()) {
            QList<RewriterView::Error> errors;
            foreach (const QmlJS::DiagnosticMessage &message, doc->diagnosticMessages())
                errors.append(RewriterView::Error(message, QUrl::fromLocalFile(doc->fileName())));
            m_rewriterView->setErrors(errors);
            setActive(false);
            return false;
        }
        snapshot.insert(doc);
        ReadingContext ctxt(snapshot, doc, importPaths);
        m_lookupContext = ctxt.lookupContext();
        m_document = doc;

        QList<RewriterView::Error> errors;

        foreach (const QmlJS::DiagnosticMessage &diagnosticMessage, ctxt.diagnosticLinkMessages()) {
            errors.append(RewriterView::Error(diagnosticMessage, QUrl::fromLocalFile(doc->fileName())));
        }

        Check check(doc, snapshot, m_lookupContext->context());
        check.setOptions(check.options() & ~Check::ErrCheckTypeErrors);
        foreach (const QmlJS::DiagnosticMessage &diagnosticMessage, check())
            if (diagnosticMessage.isError())
            errors.append(RewriterView::Error(diagnosticMessage, QUrl::fromLocalFile(doc->fileName())));

        if (!errors.isEmpty()) {
            m_rewriterView->setErrors(errors);
            setActive(false);
            return false;
        }

        setupImports(doc, differenceHandler);

        UiObjectMember *astRootNode = 0;
        if (UiProgram *program = doc->qmlProgram())
            if (program->members)
                astRootNode = program->members->member;
        ModelNode modelRootNode = m_rewriterView->rootModelNode();
        syncNode(modelRootNode, astRootNode, &ctxt, differenceHandler);
        m_rewriterView->positionStorage()->cleanupInvalidOffsets();
        m_rewriterView->clearErrors();

        setActive(false);
        return true;
    } catch (Exception &e) {
        RewriterView::Error error(&e);
        // Somehow, the error below gets eaten in upper levels, so printing the
        // exception info here for debugging purposes:
        qDebug() << "*** An exception occurred while reading the QML file:"
                 << error.toString();
        m_rewriterView->addError(error);

        setActive(false);

        return false;
    }
}

void TextToModelMerger::syncNode(ModelNode &modelNode,
                                 UiObjectMember *astNode,
                                 ReadingContext *context,
                                 DifferenceHandler &differenceHandler)
{
    UiQualifiedId *astObjectType = 0;
    UiObjectInitializer *astInitializer = 0;
    if (UiObjectDefinition *def = cast<UiObjectDefinition *>(astNode)) {
        astObjectType = def->qualifiedTypeNameId;
        astInitializer = def->initializer;
    } else if (UiObjectBinding *bin = cast<UiObjectBinding *>(astNode)) {
        astObjectType = bin->qualifiedTypeNameId;
        astInitializer = bin->initializer;
    }

    if (!astObjectType || !astInitializer)
        return;

    m_rewriterView->positionStorage()->setNodeOffset(modelNode, astObjectType->identifierToken.offset);

    QString typeName, defaultPropertyName;
    int majorVersion;
    int minorVersion;
    context->lookup(astObjectType, typeName, majorVersion, minorVersion, defaultPropertyName);

    if (typeName.isEmpty()) {
        qWarning() << "Skipping node with unknown type" << flatten(astObjectType);
        return;
    }

    if (modelNode.type() != typeName
            /*|| modelNode.majorVersion() != domObject.objectTypeMajorVersion()
            || modelNode.minorVersion() != domObject.objectTypeMinorVersion()*/) {
        const bool isRootNode = m_rewriterView->rootModelNode() == modelNode;
        differenceHandler.typeDiffers(isRootNode, modelNode, typeName,
                                      majorVersion, minorVersion,
                                      astNode, context);
        if (!isRootNode)
            return; // the difference handler will create a new node, so we're done.
    }

    context->enterScope(astNode);

    QSet<QString> modelPropertyNames = QSet<QString>::fromList(modelNode.propertyNames());
    if (!modelNode.id().isEmpty())
        modelPropertyNames.insert(QLatin1String("id"));
    QList<UiObjectMember *> defaultPropertyItems;

    for (UiObjectMemberList *iter = astInitializer->members; iter; iter = iter->next) {
        UiObjectMember *member = iter->member;
        if (!member)
            continue;

        if (UiArrayBinding *array = cast<UiArrayBinding *>(member)) {
            const QString astPropertyName = flatten(array->qualifiedId);
            if (isPropertyChangesType(typeName) || context->lookupProperty(QString(), array->qualifiedId)) {
                AbstractProperty modelProperty = modelNode.property(astPropertyName);
                QList<UiObjectMember *> arrayMembers;
                for (UiArrayMemberList *iter = array->members; iter; iter = iter->next)
                    if (UiObjectMember *member = iter->member)
                        arrayMembers.append(member);

                syncArrayProperty(modelProperty, arrayMembers, context, differenceHandler);
                modelPropertyNames.remove(astPropertyName);
            } else {
                qWarning() << "Skipping invalid array property" << astPropertyName
                           << "for node type" << modelNode.type();
            }
        } else if (UiObjectDefinition *def = cast<UiObjectDefinition *>(member)) {
            const QString name = def->qualifiedTypeNameId->name->asString();
            if (name.isEmpty() || !name.at(0).isUpper()) {
                QStringList props = syncGroupedProperties(modelNode,
                                                          name,
                                                          def->initializer->members,
                                                          context,
                                                          differenceHandler);
                foreach (const QString &prop, props)
                    modelPropertyNames.remove(prop);
            } else {
                defaultPropertyItems.append(member);
            }
        } else if (UiObjectBinding *binding = cast<UiObjectBinding *>(member)) {
            const QString astPropertyName = flatten(binding->qualifiedId);
            if (binding->hasOnToken) {
                // skip value sources
            } else {
                const Interpreter::Value *propertyType = 0;
                const Interpreter::ObjectValue *containingObject = 0;
                QString name;
                if (context->lookupProperty(QString(), binding->qualifiedId, &propertyType, &containingObject, &name) || isPropertyChangesType(typeName)) {
                    AbstractProperty modelProperty = modelNode.property(astPropertyName);
                    if (context->isArrayProperty(propertyType, containingObject, name)) {
                        syncArrayProperty(modelProperty, QList<QmlJS::AST::UiObjectMember*>() << member, context, differenceHandler);
                    } else {
                        syncNodeProperty(modelProperty, binding, context, differenceHandler);
                    }
                    modelPropertyNames.remove(astPropertyName);
                } else {
                    qWarning() << "Skipping invalid node property" << astPropertyName
                               << "for node type" << modelNode.type();
                }
            }
        } else if (UiScriptBinding *script = cast<UiScriptBinding *>(member)) {
            modelPropertyNames.remove(syncScriptBinding(modelNode, QString(), script, context, differenceHandler));
        } else if (UiPublicMember *property = cast<UiPublicMember *>(member)) {
            if (property->type == UiPublicMember::Signal)
                continue; // QML designer doesn't support this yet.

            if (!property->name || !property->memberType)
                continue; // better safe than sorry.

            const QString astName = property->name->asString();
            QString astValue;
            if (property->expression)
                astValue = textAt(context->doc(),
                                  property->expression->firstSourceLocation(),
                                  property->expression->lastSourceLocation());
            const QString astType = property->memberType->asString();
            AbstractProperty modelProperty = modelNode.property(astName);
            if (!property->expression || isLiteralValue(property->expression)) {
                const QVariant variantValue = convertDynamicPropertyValueToVariant(astValue, astType);
                syncVariantProperty(modelProperty, variantValue, astType, differenceHandler);
            } else {
                syncExpressionProperty(modelProperty, astValue, astType, differenceHandler);
            }
            modelPropertyNames.remove(astName);
        } else {
            qWarning() << "Found an unknown QML value.";
        }
    }

    if (!defaultPropertyItems.isEmpty()) {
        if (defaultPropertyName.isEmpty()) {
            if (!isComponentType(modelNode.type())) {
                qWarning() << "No default property for node type" << modelNode.type() << ", ignoring child items.";
            } else {
                setupComponent(modelNode);
                modelPropertyNames.remove(QLatin1String("__component_data"));
            }
        } else {
            AbstractProperty modelProperty = modelNode.property(defaultPropertyName);
            if (modelProperty.isNodeListProperty()) {
                NodeListProperty nodeListProperty = modelProperty.toNodeListProperty();
                syncNodeListProperty(nodeListProperty, defaultPropertyItems, context,
                                     differenceHandler);
            } else {
                differenceHandler.shouldBeNodeListProperty(modelProperty,
                                                           defaultPropertyItems,
                                                           context);
            }
            modelPropertyNames.remove(defaultPropertyName);
        }
    }

    foreach (const QString &modelPropertyName, modelPropertyNames) {
        AbstractProperty modelProperty = modelNode.property(modelPropertyName);

        // property deleted.
        if (modelPropertyName == QLatin1String("id"))
            differenceHandler.idsDiffer(modelNode, QString());
        else
            differenceHandler.propertyAbsentFromQml(modelProperty);
    }

    context->leaveScope();
}

QString TextToModelMerger::syncScriptBinding(ModelNode &modelNode,
                                             const QString &prefix,
                                             UiScriptBinding *script,
                                             ReadingContext *context,
                                             DifferenceHandler &differenceHandler)
{
    QString astPropertyName = flatten(script->qualifiedId);
    if (!prefix.isEmpty())
        astPropertyName.prepend(prefix + QLatin1Char('.'));

    QString astValue;
    if (script->statement) {
        astValue = textAt(context->doc(),
                          script->statement->firstSourceLocation(),
                          script->statement->lastSourceLocation());
        astValue = astValue.trimmed();
        if (astValue.endsWith(QLatin1Char(';')))
            astValue = astValue.left(astValue.length() - 1);
        astValue = astValue.trimmed();
    }

    if (astPropertyName == QLatin1String("id")) {
        syncNodeId(modelNode, astValue, differenceHandler);
        return astPropertyName;
    }

    if (isSignalPropertyName(astPropertyName))
        return QString();

    if (isLiteralValue(script)) {
        if (isPropertyChangesType(modelNode.type())) {
            AbstractProperty modelProperty = modelNode.property(astPropertyName);
            const QVariant variantValue(deEscape(stripQuotes(astValue)));
            syncVariantProperty(modelProperty, variantValue, QString(), differenceHandler);
            return astPropertyName;
        } else {
            const QVariant variantValue = context->convertToVariant(astValue, prefix, script->qualifiedId);
            if (variantValue.isValid()) {
                AbstractProperty modelProperty = modelNode.property(astPropertyName);
                syncVariantProperty(modelProperty, variantValue, QString(), differenceHandler);
                return astPropertyName;
            } else {
                qWarning() << "Skipping invalid variant property" << astPropertyName
                           << "for node type" << modelNode.type();
                return QString();
            }
        }
    }

    const QVariant enumValue = context->convertToEnum(script->statement, prefix, script->qualifiedId);
    if (enumValue.isValid()) { // It is a qualified enum:
        AbstractProperty modelProperty = modelNode.property(astPropertyName);
        syncVariantProperty(modelProperty, enumValue, QString(), differenceHandler); // TODO: parse type
        return astPropertyName;
    } else { // Not an enum, so:
        if (isPropertyChangesType(modelNode.type()) || context->lookupProperty(prefix, script->qualifiedId)) {
            AbstractProperty modelProperty = modelNode.property(astPropertyName);
            syncExpressionProperty(modelProperty, astValue, QString(), differenceHandler); // TODO: parse type
            return astPropertyName;
        } else {
            qWarning() << "Skipping invalid expression property" << astPropertyName
                    << "for node type" << modelNode.type();
            return QString();
        }
    }
}

void TextToModelMerger::syncNodeId(ModelNode &modelNode, const QString &astObjectId,
                                   DifferenceHandler &differenceHandler)
{
    if (astObjectId.isEmpty()) {
        if (!modelNode.id().isEmpty()) {
            ModelNode existingNodeWithId = m_rewriterView->modelNodeForId(astObjectId);
            if (existingNodeWithId.isValid())
                existingNodeWithId.setId(QString());
            differenceHandler.idsDiffer(modelNode, astObjectId);
        }
    } else {
        if (modelNode.id() != astObjectId) {
            ModelNode existingNodeWithId = m_rewriterView->modelNodeForId(astObjectId);
            if (existingNodeWithId.isValid())
                existingNodeWithId.setId(QString());
            differenceHandler.idsDiffer(modelNode, astObjectId);
        }
    }
}

void TextToModelMerger::syncNodeProperty(AbstractProperty &modelProperty,
                                         UiObjectBinding *binding,
                                         ReadingContext *context,
                                         DifferenceHandler &differenceHandler)
{
    QString typeName, dummy;
    int majorVersion;
    int minorVersion;
    context->lookup(binding->qualifiedTypeNameId, typeName, majorVersion, minorVersion, dummy);

    if (typeName.isEmpty()) {
        qWarning() << "Skipping node with unknown type" << flatten(binding->qualifiedTypeNameId);
        return;
    }

    if (modelProperty.isNodeProperty()) {
        ModelNode nodePropertyNode = modelProperty.toNodeProperty().modelNode();
        syncNode(nodePropertyNode, binding, context, differenceHandler);
    } else {
        differenceHandler.shouldBeNodeProperty(modelProperty,
                                               typeName,
                                               majorVersion,
                                               minorVersion,
                                               binding, context);
    }
}

void TextToModelMerger::syncExpressionProperty(AbstractProperty &modelProperty,
                                               const QString &javascript,
                                               const QString &astType,
                                               DifferenceHandler &differenceHandler)
{
    if (modelProperty.isBindingProperty()) {
        BindingProperty bindingProperty = modelProperty.toBindingProperty();
        if (bindingProperty.expression() != javascript
                || !astType.isEmpty() != bindingProperty.isDynamic()
                || astType != bindingProperty.dynamicTypeName()) {
            differenceHandler.bindingExpressionsDiffer(bindingProperty, javascript, astType);
        }
    } else {
        differenceHandler.shouldBeBindingProperty(modelProperty, javascript, astType);
    }
}

void TextToModelMerger::syncArrayProperty(AbstractProperty &modelProperty,
                                          const QList<UiObjectMember *> &arrayMembers,
                                          ReadingContext *context,
                                          DifferenceHandler &differenceHandler)
{
    if (modelProperty.isNodeListProperty()) {
        NodeListProperty nodeListProperty = modelProperty.toNodeListProperty();
        syncNodeListProperty(nodeListProperty, arrayMembers, context, differenceHandler);
    } else {
        differenceHandler.shouldBeNodeListProperty(modelProperty,
                                                   arrayMembers,
                                                   context);
    }
}

void TextToModelMerger::syncVariantProperty(AbstractProperty &modelProperty,
                                            const QVariant &astValue,
                                            const QString &astType,
                                            DifferenceHandler &differenceHandler)
{
    if (modelProperty.isVariantProperty()) {
        VariantProperty modelVariantProperty = modelProperty.toVariantProperty();

        if (!equals(modelVariantProperty.value(), astValue)
                || !astType.isEmpty() != modelVariantProperty.isDynamic()
                || astType != modelVariantProperty.dynamicTypeName()) {
            differenceHandler.variantValuesDiffer(modelVariantProperty,
                                                  astValue,
                                                  astType);
        }
    } else {
        differenceHandler.shouldBeVariantProperty(modelProperty,
                                                  astValue,
                                                  astType);
    }
}

void TextToModelMerger::syncNodeListProperty(NodeListProperty &modelListProperty,
                                             const QList<UiObjectMember *> arrayMembers,
                                             ReadingContext *context,
                                             DifferenceHandler &differenceHandler)
{
    QList<ModelNode> modelNodes = modelListProperty.toModelNodeList();
    int i = 0;
    for (; i < modelNodes.size() && i < arrayMembers.size(); ++i) {
        ModelNode modelNode = modelNodes.at(i);
        syncNode(modelNode, arrayMembers.at(i), context, differenceHandler);
    }

    for (int j = i; j < arrayMembers.size(); ++j) {
        // more elements in the dom-list, so add them to the model
        UiObjectMember *arrayMember = arrayMembers.at(j);
        const ModelNode newNode = differenceHandler.listPropertyMissingModelNode(modelListProperty, context, arrayMember);
        QString name;
        if (UiObjectDefinition *definition = cast<UiObjectDefinition *>(arrayMember))
            name = flatten(definition->qualifiedTypeNameId);
        if (isComponentType(name))
            setupComponent(newNode);
    }

    for (int j = i; j < modelNodes.size(); ++j) {
        // more elements in the model, so remove them.
        ModelNode modelNode = modelNodes.at(j);
        differenceHandler.modelNodeAbsentFromQml(modelNode);
    }
}

ModelNode TextToModelMerger::createModelNode(const QString &typeName,
                                             int majorVersion,
                                             int minorVersion,
                                             UiObjectMember *astNode,
                                             ReadingContext *context,
                                             DifferenceHandler &differenceHandler)
{
    ModelNode newNode = m_rewriterView->createModelNode(typeName,
                                                        majorVersion,
                                                        minorVersion);
    syncNode(newNode, astNode, context, differenceHandler);
    return newNode;
}

QStringList TextToModelMerger::syncGroupedProperties(ModelNode &modelNode,
                                                     const QString &name,
                                                     UiObjectMemberList *members,
                                                     ReadingContext *context,
                                                     DifferenceHandler &differenceHandler)
{
    QStringList props;

    for (UiObjectMemberList *iter = members; iter; iter = iter->next) {
        UiObjectMember *member = iter->member;

        if (UiScriptBinding *script = cast<UiScriptBinding *>(member)) {
            const QString prop = syncScriptBinding(modelNode, name, script, context, differenceHandler);
            if (!prop.isEmpty())
                props.append(prop);
        }
    }

    return props;
}

void ModelValidator::modelMissesImport(const Import &import)
{
    Q_ASSERT(m_merger->view()->model()->imports().contains(import));
}

void ModelValidator::importAbsentInQMl(const Import &import)
{
    Q_ASSERT(! m_merger->view()->model()->imports().contains(import));
}

void ModelValidator::bindingExpressionsDiffer(BindingProperty &modelProperty,
                                              const QString &javascript,
                                              const QString &astType)
{
    Q_ASSERT(modelProperty.expression() == javascript);
    Q_ASSERT(modelProperty.dynamicTypeName() == astType);
    Q_ASSERT(0);
}

void ModelValidator::shouldBeBindingProperty(AbstractProperty &modelProperty,
                                             const QString &/*javascript*/,
                                             const QString &/*astType*/)
{
    Q_ASSERT(modelProperty.isBindingProperty());
    Q_ASSERT(0);
}

void ModelValidator::shouldBeNodeListProperty(AbstractProperty &modelProperty,
                                              const QList<UiObjectMember *> /*arrayMembers*/,
                                              ReadingContext * /*context*/)
{
    Q_ASSERT(modelProperty.isNodeListProperty());
    Q_ASSERT(0);
}

void ModelValidator::variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName)
{
    Q_ASSERT(modelProperty.isDynamic() == !dynamicTypeName.isEmpty());
    if (modelProperty.isDynamic()) {
        Q_ASSERT(modelProperty.dynamicTypeName() == dynamicTypeName);
    }

    Q_ASSERT(equals(modelProperty.value(), qmlVariantValue));
    Q_ASSERT(0);
}

void ModelValidator::shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &/*qmlVariantValue*/, const QString &/*dynamicTypeName*/)
{
    Q_ASSERT(modelProperty.isVariantProperty());
    Q_ASSERT(0);
}

void ModelValidator::shouldBeNodeProperty(AbstractProperty &modelProperty,
                                          const QString &/*typeName*/,
                                          int /*majorVersion*/,
                                          int /*minorVersion*/,
                                          UiObjectMember * /*astNode*/,
                                          ReadingContext * /*context*/)
{
    Q_ASSERT(modelProperty.isNodeProperty());
    Q_ASSERT(0);
}

void ModelValidator::modelNodeAbsentFromQml(ModelNode &modelNode)
{
    Q_ASSERT(!modelNode.isValid());
    Q_ASSERT(0);
}

ModelNode ModelValidator::listPropertyMissingModelNode(NodeListProperty &/*modelProperty*/,
                                                       ReadingContext * /*context*/,
                                                       UiObjectMember * /*arrayMember*/)
{
    Q_ASSERT(0);
    return ModelNode();
}

void ModelValidator::typeDiffers(bool /*isRootNode*/,
                                 ModelNode &modelNode,
                                 const QString &typeName,
                                 int majorVersion,
                                 int minorVersion,
                                 QmlJS::AST::UiObjectMember * /*astNode*/,
                                 ReadingContext * /*context*/)
{
    Q_ASSERT(modelNode.type() == typeName);
    Q_ASSERT(modelNode.majorVersion() == majorVersion);
    Q_ASSERT(modelNode.minorVersion() == minorVersion);
    Q_ASSERT(0);
}

void ModelValidator::propertyAbsentFromQml(AbstractProperty &modelProperty)
{
    Q_ASSERT(!modelProperty.isValid());
    Q_ASSERT(0);
}

void ModelValidator::idsDiffer(ModelNode &modelNode, const QString &qmlId)
{
    Q_ASSERT(modelNode.id() == qmlId);
    Q_ASSERT(0);
}

void ModelAmender::modelMissesImport(const Import &import)
{
    m_merger->view()->model()->changeImports(QList<Import>() << import, QList<Import>());
}

void ModelAmender::importAbsentInQMl(const Import &import)
{
    m_merger->view()->model()->changeImports(QList<Import>(), QList<Import>() << import);
}

void ModelAmender::bindingExpressionsDiffer(BindingProperty &modelProperty,
                                            const QString &javascript,
                                            const QString &astType)
{
    if (astType.isEmpty()) {
        modelProperty.setExpression(javascript);
    } else {
        modelProperty.setDynamicTypeNameAndExpression(astType, javascript);
    }
}

void ModelAmender::shouldBeBindingProperty(AbstractProperty &modelProperty,
                                           const QString &javascript,
                                           const QString &astType)
{
    ModelNode theNode = modelProperty.parentModelNode();
    BindingProperty newModelProperty = theNode.bindingProperty(modelProperty.name());
    if (astType.isEmpty()) {
        newModelProperty.setExpression(javascript);
    } else {
        newModelProperty.setDynamicTypeNameAndExpression(astType, javascript);
    }
}

void ModelAmender::shouldBeNodeListProperty(AbstractProperty &modelProperty,
                                            const QList<UiObjectMember *> arrayMembers,
                                            ReadingContext *context)
{
    ModelNode theNode = modelProperty.parentModelNode();
    NodeListProperty newNodeListProperty = theNode.nodeListProperty(modelProperty.name());
    m_merger->syncNodeListProperty(newNodeListProperty,
                                   arrayMembers,
                                   context,
                                   *this);
}



void ModelAmender::variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicType)
{
//    qDebug()<< "ModelAmender::variantValuesDiffer for property"<<modelProperty.name()
//            << "in node" << modelProperty.parentModelNode().id()
//            << ", old value:" << modelProperty.value()
//            << "new value:" << qmlVariantValue;

    if (dynamicType.isEmpty())
        modelProperty.setValue(qmlVariantValue);
    else
        modelProperty.setDynamicTypeNameAndValue(dynamicType, qmlVariantValue);
}

void ModelAmender::shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName)
{
    ModelNode theNode = modelProperty.parentModelNode();
    VariantProperty newModelProperty = theNode.variantProperty(modelProperty.name());

    if (dynamicTypeName.isEmpty())
        newModelProperty.setValue(qmlVariantValue);
    else
        newModelProperty.setDynamicTypeNameAndValue(dynamicTypeName, qmlVariantValue);
}

void ModelAmender::shouldBeNodeProperty(AbstractProperty &modelProperty,
                                        const QString &typeName,
                                        int majorVersion,
                                        int minorVersion,
                                        UiObjectMember *astNode,
                                        ReadingContext *context)
{
    ModelNode theNode = modelProperty.parentModelNode();
    NodeProperty newNodeProperty = theNode.nodeProperty(modelProperty.name());
    newNodeProperty.setModelNode(m_merger->createModelNode(typeName,
                                                           majorVersion,
                                                           minorVersion,
                                                           astNode,
                                                           context,
                                                           *this));
}

void ModelAmender::modelNodeAbsentFromQml(ModelNode &modelNode)
{
    modelNode.destroy();
}

ModelNode ModelAmender::listPropertyMissingModelNode(NodeListProperty &modelProperty,
                                                     ReadingContext *context,
                                                     UiObjectMember *arrayMember)
{
    UiQualifiedId *astObjectType = 0;
    UiObjectInitializer *astInitializer = 0;
    if (UiObjectDefinition *def = cast<UiObjectDefinition *>(arrayMember)) {
        astObjectType = def->qualifiedTypeNameId;
        astInitializer = def->initializer;
    } else if (UiObjectBinding *bin = cast<UiObjectBinding *>(arrayMember)) {
        astObjectType = bin->qualifiedTypeNameId;
        astInitializer = bin->initializer;
    }

    if (!astObjectType || !astInitializer)
        return ModelNode();

    QString typeName, dummy;
    int majorVersion;
    int minorVersion;
    context->lookup(astObjectType, typeName, majorVersion, minorVersion, dummy);

    if (typeName.isEmpty()) {
        qWarning() << "Skipping node with unknown type" << flatten(astObjectType);
        return ModelNode();
    }

    const ModelNode &newNode = m_merger->createModelNode(typeName,
                                                         majorVersion,
                                                         minorVersion,
                                                         arrayMember,
                                                         context,
                                                         *this);
    modelProperty.reparentHere(newNode);
    return newNode;
}

void ModelAmender::typeDiffers(bool isRootNode,
                               ModelNode &modelNode,
                               const QString &typeName,
                               int majorVersion,
                               int minorVersion,
                               QmlJS::AST::UiObjectMember *astNode,
                               ReadingContext *context)
{
    if (isRootNode) {
        modelNode.view()->changeRootNodeType(typeName, majorVersion, minorVersion);
    } else {
        NodeAbstractProperty parentProperty = modelNode.parentProperty();
        int nodeIndex = -1;
        if (parentProperty.isNodeListProperty()) {
            nodeIndex = parentProperty.toNodeListProperty().toModelNodeList().indexOf(modelNode);
            Q_ASSERT(nodeIndex >= 0);
        }

        modelNode.destroy();

        const ModelNode &newNode = m_merger->createModelNode(typeName,
                                                             majorVersion,
                                                             minorVersion,
                                                             astNode,
                                                             context,
                                                             *this);
        parentProperty.reparentHere(newNode);
        if (nodeIndex >= 0) {
            int currentIndex = parentProperty.toNodeListProperty().toModelNodeList().indexOf(newNode);
            if (nodeIndex != currentIndex)
                parentProperty.toNodeListProperty().slide(currentIndex, nodeIndex);
        }
    }
}

void ModelAmender::propertyAbsentFromQml(AbstractProperty &modelProperty)
{
    modelProperty.parentModelNode().removeProperty(modelProperty.name());
}

void ModelAmender::idsDiffer(ModelNode &modelNode, const QString &qmlId)
{
    modelNode.setId(qmlId);
}

void TextToModelMerger::setupComponent(const ModelNode &node)
{
    Q_ASSERT(isComponentType(node.type()));

    QString componentText = m_rewriterView->extractText(QList<ModelNode>() << node).value(node);

    if (componentText.isEmpty())
        return;

    QString result;
    if (componentText.contains("Component")) { //explicit component
        FirstDefinitionFinder firstDefinitionFinder(componentText);
        int offset = firstDefinitionFinder(0);
        ObjectLengthCalculator objectLengthCalculator;
        unsigned length;
        if (objectLengthCalculator(componentText, offset, length)) {
            result = componentText.mid(offset, length);
        } else {
            result = componentText;
        }
    } else {
        result = componentText; //implicit component
    }

    if (node.hasVariantProperty("__component_data")
            && node.variantProperty("__component_data").value().toString() == result)
        return;

    node.variantProperty("__component_data").setValue(result);
}

QString TextToModelMerger::textAt(const Document::Ptr &doc,
                                  const SourceLocation &location)
{
    return doc->source().mid(location.offset, location.length);
}

QString TextToModelMerger::textAt(const Document::Ptr &doc,
                                  const SourceLocation &from,
                                  const SourceLocation &to)
{
    return doc->source().mid(from.offset, to.end() - from.begin());
}
