/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "parser/qmljsast_p.h"
#include "qmljsbind.h"
#include "qmljscheck.h"
#include "qmljsdocument.h"

#include <languageutils/componentversion.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

using namespace LanguageUtils;
using namespace QmlJS;
using namespace QmlJS::AST;

/*!
    \class QmlJS::Bind
    \brief Collected information about a single Document.
    \sa QmlJS::Document QmlJS::Link

    Each QmlJS::Document owns a instance of Bind. It provides access to data
    that can be derived by looking at the document in isolation. If you need
    information that goes beyond that, you need to create a
    \l{QmlJS::Context} using \l{QmlJS::Link}.

    The document's imports are classified and available through imports().

    It allows AST to code model lookup through findQmlObject() and findFunctionScope().
*/

Bind::Bind(Document *doc, QList<DiagnosticMessage> *messages)
    : _doc(doc),
      _currentObjectValue(0),
      _idEnvironment(0),
      _rootObjectValue(0),
      _diagnosticMessages(messages)
{
    if (_doc)
        accept(_doc->ast());
}

Bind::~Bind()
{
}

QList<ImportInfo> Bind::imports() const
{
    return _imports;
}

ObjectValue *Bind::idEnvironment() const
{
    return _idEnvironment;
}

ObjectValue *Bind::rootObjectValue() const
{
    return _rootObjectValue;
}

ObjectValue *Bind::findQmlObject(AST::Node *node) const
{
    return _qmlObjects.value(node);
}

bool Bind::usesQmlPrototype(ObjectValue *prototype,
                            const ContextPtr &context) const
{
    if (!prototype)
        return false;

    const QString componentName = prototype->className();

    // all component objects have classname set
    if (componentName.isEmpty())
        return false;

    foreach (const ObjectValue *object, _qmlObjectsByPrototypeName.values(componentName)) {
        // resolve and check the prototype
        const ObjectValue *resolvedPrototype = object->prototype(context);
        if (resolvedPrototype == prototype)
            return true;
    }

    return false;
}

ObjectValue *Bind::findAttachedJSScope(AST::Node *node) const
{
    return _attachedJSScopes.value(node);
}

bool Bind::isGroupedPropertyBinding(AST::Node *node) const
{
    return _groupedPropertyBindings.contains(node);
}

ObjectValue *Bind::switchObjectValue(ObjectValue *newObjectValue)
{
    ObjectValue *oldObjectValue = _currentObjectValue;
    _currentObjectValue = newObjectValue;
    return oldObjectValue;
}

QString Bind::toString(UiQualifiedId *qualifiedId, QChar delimiter)
{
    QString result;

    for (UiQualifiedId *iter = qualifiedId; iter; iter = iter->next) {
        if (iter != qualifiedId)
            result += delimiter;

        result += iter->name;
    }

    return result;
}

ObjectValue *Bind::bindObject(UiQualifiedId *qualifiedTypeNameId, UiObjectInitializer *initializer)
{
    ObjectValue *parentObjectValue = 0;

    // normal component instance
    ASTObjectValue *objectValue = new ASTObjectValue(qualifiedTypeNameId, initializer, _doc, &_valueOwner);
    QmlPrototypeReference *prototypeReference =
            new QmlPrototypeReference(qualifiedTypeNameId, _doc, &_valueOwner);
    objectValue->setPrototype(prototypeReference);

    // add the prototype name to the prototypes hash
    for (UiQualifiedId *it = qualifiedTypeNameId; it; it = it->next) {
        if (!it->next && !it->name.isEmpty())
            _qmlObjectsByPrototypeName.insert(it->name.toString(), objectValue);
    }

    parentObjectValue = switchObjectValue(objectValue);

    if (parentObjectValue)
        objectValue->setMember(QLatin1String("parent"), parentObjectValue);
    else {
        _rootObjectValue = objectValue;
        _rootObjectValue->setClassName(_doc->componentName());
    }

    accept(initializer);

    return switchObjectValue(parentObjectValue);
}

void Bind::accept(Node *node)
{
    Node::accept(node, this);
}

bool Bind::visit(AST::UiProgram *)
{
    _idEnvironment = _valueOwner.newObject(/*prototype =*/ 0);
    return true;
}

bool Bind::visit(AST::Program *)
{
    _currentObjectValue = _valueOwner.newObject(/*prototype =*/ 0);
    _rootObjectValue = _currentObjectValue;
    return true;
}

bool Bind::visit(UiImport *ast)
{
    ComponentVersion version;
    ImportInfo::Type type = ImportInfo::InvalidImport;
    QString path;
    QString name;

    if (ast->versionToken.isValid()) {
        const QString versionString = _doc->source().mid(ast->versionToken.offset, ast->versionToken.length);
        version = ComponentVersion(versionString);
        if (!version.isValid()) {
            _diagnosticMessages->append(
                        errorMessage(ast->versionToken, tr("expected two numbers separated by a dot")));
        }
    }

    if (ast->importUri) {
        type = ImportInfo::LibraryImport;
        path = toString(ast->importUri, QDir::separator());
        name = toString(ast->importUri, QLatin1Char('.'));

        // treat Qt 4.7 as QtQuick 1.0
        if (path == QLatin1String("Qt") && version == ComponentVersion(4, 7)) {
            path = QLatin1String("QtQuick");
            name = path;
            version = ComponentVersion(1, 0);
        }

        if (!version.isValid()) {
            _diagnosticMessages->append(
                        errorMessage(ast, tr("package import requires a version number")));
        }
    } else if (!ast->fileName.isEmpty()) {
        name = ast->fileName.toString();
        QFileInfo importFileInfo(name);
        if (!importFileInfo.isAbsolute()) {
            importFileInfo = QFileInfo(_doc->path() + QDir::separator() + name);
        }
        path = importFileInfo.absoluteFilePath();
        if (importFileInfo.isFile())
            type = ImportInfo::FileImport;
        else if (importFileInfo.isDir())
            type = ImportInfo::DirectoryImport;
        else {
            type = ImportInfo::UnknownFileImport;
        }
    }
    _imports += ImportInfo(type, path, name, version, ast);

    return false;
}

bool Bind::visit(UiPublicMember *ast)
{
    const Block *block = AST::cast<const Block*>(ast->statement);
    if (block) {
        // build block scope
        ObjectValue *blockScope = _valueOwner.newObject(/*prototype=*/0);
        _attachedJSScopes.insert(ast, blockScope); // associated with the UiPublicMember, not with the block
        ObjectValue *parent = switchObjectValue(blockScope);
        accept(ast->statement);
        switchObjectValue(parent);
        return false;
    }
    return true;
}

bool Bind::visit(UiObjectDefinition *ast)
{
    // an UiObjectDefinition may be used to group property bindings
    // think anchors { ... }
    bool isGroupedBinding = ast->qualifiedTypeNameId
            && !ast->qualifiedTypeNameId->name.isEmpty()
            && ast->qualifiedTypeNameId->name.at(0).isLower();

    if (!isGroupedBinding) {
        ObjectValue *value = bindObject(ast->qualifiedTypeNameId, ast->initializer);
        _qmlObjects.insert(ast, value);
    } else {
        _groupedPropertyBindings.insert(ast);
        ObjectValue *oldObjectValue = switchObjectValue(0);
        accept(ast->initializer);
        switchObjectValue(oldObjectValue);
    }

    return false;
}

bool Bind::visit(UiObjectBinding *ast)
{
//    const QString name = serialize(ast->qualifiedId);
    ObjectValue *value = bindObject(ast->qualifiedTypeNameId, ast->initializer);
    _qmlObjects.insert(ast, value);
    // ### FIXME: we don't handle dot-properties correctly (i.e. font.size)
//    _currentObjectValue->setProperty(name, value);

    return false;
}

bool Bind::visit(UiScriptBinding *ast)
{
    if (_currentObjectValue && toString(ast->qualifiedId) == QLatin1String("id")) {
        if (ExpressionStatement *e = cast<ExpressionStatement*>(ast->statement))
            if (IdentifierExpression *i = cast<IdentifierExpression*>(e->expression))
                if (!i->name.isEmpty())
                    _idEnvironment->setMember(i->name.toString(), _currentObjectValue);
    }
    const Block *block = AST::cast<const Block*>(ast->statement);
    if (block) {
        // build block scope
        ObjectValue *blockScope = _valueOwner.newObject(/*prototype=*/0);
        _attachedJSScopes.insert(ast, blockScope); // associated with the UiScriptBinding, not with the block
        ObjectValue *parent = switchObjectValue(blockScope);
        accept(ast->statement);
        switchObjectValue(parent);
        return false;
    }
    return true;
}

bool Bind::visit(UiArrayBinding *)
{
    // ### FIXME: do we need to store the members into the property? Or, maybe the property type is an JS Array?

    return true;
}

bool Bind::visit(VariableDeclaration *ast)
{
    if (ast->name.isEmpty())
        return false;

    ASTVariableReference *ref = new ASTVariableReference(ast, _doc, &_valueOwner);
    if (_currentObjectValue)
        _currentObjectValue->setMember(ast->name.toString(), ref);
    return true;
}

bool Bind::visit(FunctionExpression *ast)
{
    // ### FIXME: the first declaration counts
    //if (_currentObjectValue->property(ast->name->asString(), 0))
    //    return false;

    ASTFunctionValue *function = new ASTFunctionValue(ast, _doc, &_valueOwner);
    if (_currentObjectValue && !ast->name.isEmpty() && cast<FunctionDeclaration *>(ast))
        _currentObjectValue->setMember(ast->name.toString(), function);

    // build function scope
    ObjectValue *functionScope = _valueOwner.newObject(/*prototype=*/0);
    _attachedJSScopes.insert(ast, functionScope);
    ObjectValue *parent = switchObjectValue(functionScope);

    // The order of the following is important. Example: A function with name "arguments"
    // overrides the arguments object, a variable doesn't.

    // 1. Function formal arguments
    for (FormalParameterList *it = ast->formals; it; it = it->next) {
        if (!it->name.isEmpty())
            functionScope->setMember(it->name.toString(), _valueOwner.undefinedValue());
    }

    // 2. Functions defined inside the function body
    // ### TODO, currently covered by the accept(body)

    // 3. Arguments object
    ObjectValue *arguments = _valueOwner.newObject(/*prototype=*/0);
    arguments->setMember(QLatin1String("callee"), function);
    arguments->setMember(QLatin1String("length"), _valueOwner.numberValue());
    functionScope->setMember(QLatin1String("arguments"), arguments);

    // 4. Variables defined inside the function body
    // ### TODO, currently covered by the accept(body)

    // visit body
    accept(ast->body);
    switchObjectValue(parent);

    return false;
}

bool Bind::visit(FunctionDeclaration *ast)
{
    return visit(static_cast<FunctionExpression *>(ast));
}
