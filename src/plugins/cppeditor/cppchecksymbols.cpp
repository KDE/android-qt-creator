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

#include "cppchecksymbols.h"
#include "cpplocalsymbols.h"

#include <cplusplus/Overview.h>

#include <Names.h>
#include <Literals.h>
#include <Symbols.h>
#include <TranslationUnit.h>
#include <Scope.h>
#include <AST.h>
#include <SymbolVisitor.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QThreadPool>
#include <QtCore/QDebug>

#include <qtconcurrent/runextensions.h>

using namespace CPlusPlus;
using namespace CppEditor::Internal;

namespace {

class FriendlyThread: public QThread
{
public:
    using QThread::msleep;
};

class CollectSymbols: protected SymbolVisitor
{
    Document::Ptr _doc;
    Snapshot _snapshot;
    QSet<QByteArray> _types;
    QSet<QByteArray> _members;
    QSet<QByteArray> _virtualMethods;
    QSet<QByteArray> _statics;
    bool _mainDocument;

public:
    CollectSymbols(Document::Ptr doc, const Snapshot &snapshot)
        : _doc(doc), _snapshot(snapshot), _mainDocument(false)
    {
        QSet<Namespace *> processed;
        process(doc, &processed);
    }

    const QSet<QByteArray> &types() const
    {
        return _types;
    }

    const QSet<QByteArray> &members() const
    {
        return _members;
    }

    const QSet<QByteArray> &virtualMethods() const
    {
        return _virtualMethods;
    }

    const QSet<QByteArray> &statics() const
    {
        return _statics;
    }

protected:
    void process(Document::Ptr doc, QSet<Namespace *> *processed)
    {
        if (! doc)
            return;
        else if (! processed->contains(doc->globalNamespace())) {
            processed->insert(doc->globalNamespace());

            foreach (const Document::Include &i, doc->includes())
                process(_snapshot.document(i.fileName()), processed);

            _mainDocument = (doc == _doc); // ### improve
            accept(doc->globalNamespace());
        }
    }

    void addType(const Identifier *id)
    {
        if (id)
            _types.insert(QByteArray::fromRawData(id->chars(), id->size()));
    }

    void addType(const Name *name)
    {
        if (! name) {
            return;

        } else if (const QualifiedNameId *q = name->asQualifiedNameId()) {
            addType(q->base());
            addType(q->name());

        } else if (name->isNameId() || name->isTemplateNameId()) {
            addType(name->identifier());

        }
    }

    void addMember(const Name *name)
    {
        if (! name) {
            return;

        } else if (name->isNameId()) {
            const Identifier *id = name->identifier();
            _members.insert(QByteArray::fromRawData(id->chars(), id->size()));

        }
    }

    void addVirtualMethod(const Name *name)
    {
        if (! name) {
            return;

        } else if (name->isNameId()) {
            const Identifier *id = name->identifier();
            _virtualMethods.insert(QByteArray::fromRawData(id->chars(), id->size()));

        }
    }

    void addStatic(const Name *name)
    {
        if (! name) {
            return;

        } else if (name->isNameId() || name->isTemplateNameId()) {
            const Identifier *id = name->identifier();
            _statics.insert(QByteArray::fromRawData(id->chars(), id->size()));

        }
    }

    // nothing to do
    virtual bool visit(UsingNamespaceDirective *) { return true; }
    virtual bool visit(UsingDeclaration *) { return true; }
    virtual bool visit(Argument *) { return true; }
    virtual bool visit(BaseClass *) { return true; }

    virtual bool visit(Function *symbol)
    {
        if (symbol->isVirtual())
            addVirtualMethod(symbol->name());

        return true;
    }

    virtual bool visit(Block *)
    {
        return true;
    }

    virtual bool visit(NamespaceAlias *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(Declaration *symbol)
    {
        if (symbol->enclosingEnum() != 0)
            addStatic(symbol->name());

        if (Function *funTy = symbol->type()->asFunctionType()) {
            if (funTy->isVirtual())
                addVirtualMethod(symbol->name());
        }

        if (symbol->isTypedef())
            addType(symbol->name());
        else if (! symbol->type()->isFunctionType() && symbol->enclosingScope()->isClass())
            addMember(symbol->name());

        return true;
    }

    virtual bool visit(TypenameArgument *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(Enum *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(Namespace *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(Template *)
    {
        return true;
    }

    virtual bool visit(Class *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(ForwardClassDeclaration *symbol)
    {
        addType(symbol->name());
        return true;
    }

    // Objective-C
    virtual bool visit(ObjCBaseClass *) { return true; }
    virtual bool visit(ObjCBaseProtocol *) { return true; }
    virtual bool visit(ObjCPropertyDeclaration *) { return true; }
    virtual bool visit(ObjCMethod *) { return true; }

    virtual bool visit(ObjCClass *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(ObjCForwardClassDeclaration *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(ObjCProtocol *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(ObjCForwardProtocolDeclaration *symbol)
    {
        addType(symbol->name());
        return true;
    }
};

} // end of anonymous namespace

CheckSymbols::Future CheckSymbols::go(Document::Ptr doc, const LookupContext &context)
{
    Q_ASSERT(doc);

    return (new CheckSymbols(doc, context))->start();
}

CheckSymbols::CheckSymbols(Document::Ptr doc, const LookupContext &context)
    : ASTVisitor(doc->translationUnit()), _doc(doc), _context(context)
{
    CollectSymbols collectTypes(doc, context.snapshot());

    _fileName = doc->fileName();
    _potentialTypes = collectTypes.types();
    _potentialMembers = collectTypes.members();
    _potentialVirtualMethods = collectTypes.virtualMethods();
    _potentialStatics = collectTypes.statics();
    _flushRequested = false;
    _flushLine = 0;

    typeOfExpression.init(_doc, _context.snapshot(), _context.bindings());
}

CheckSymbols::~CheckSymbols()
{ }

void CheckSymbols::run()
{
    _diagnosticMessages.clear();

    if (! isCanceled()) {
        if (_doc->translationUnit()) {
            accept(_doc->translationUnit()->ast());
            flush();
        }
    }

    reportFinished();
}

bool CheckSymbols::warning(unsigned line, unsigned column, const QString &text, unsigned length)
{
    Document::DiagnosticMessage m(Document::DiagnosticMessage::Warning, _fileName, line, column, text, length);
    _diagnosticMessages.append(m);
    return false;
}

bool CheckSymbols::warning(AST *ast, const QString &text)
{
    const Token &firstToken = tokenAt(ast->firstToken());
    const Token &lastToken = tokenAt(ast->lastToken() - 1);

    const unsigned length = lastToken.end() - firstToken.begin();
    unsigned line = 1, column = 1;
    getTokenStartPosition(ast->firstToken(), &line, &column);

    warning(line, column, text, length);
    return false;
}

FunctionDefinitionAST *CheckSymbols::enclosingFunctionDefinition(bool skipTopOfStack) const
{
    int index = _astStack.size() - 1;
    if (skipTopOfStack && !_astStack.isEmpty())
        --index;
    for (; index != -1; --index) {
        AST *ast = _astStack.at(index);

        if (FunctionDefinitionAST *funDef = ast->asFunctionDefinition())
            return funDef;
    }

    return 0;
}

TemplateDeclarationAST *CheckSymbols::enclosingTemplateDeclaration() const
{
    for (int index = _astStack.size() - 1; index != -1; --index) {
        AST *ast = _astStack.at(index);

        if (TemplateDeclarationAST *funDef = ast->asTemplateDeclaration())
            return funDef;
    }

    return 0;
}

Scope *CheckSymbols::enclosingScope() const
{
    for (int index = _astStack.size() - 1; index != -1; --index) {
        AST *ast = _astStack.at(index);

        if (NamespaceAST *ns = ast->asNamespace()) {
            if (ns->symbol)
                return ns->symbol;

        } else if (ClassSpecifierAST *classSpec = ast->asClassSpecifier()) {
            if (classSpec->symbol)
                return classSpec->symbol;

        } else if (FunctionDefinitionAST *funDef = ast->asFunctionDefinition()) {
            if (funDef->symbol)
                return funDef->symbol;

        } else if (CompoundStatementAST *blockStmt = ast->asCompoundStatement()) {
            if (blockStmt->symbol)
                return blockStmt->symbol;

        } else if (IfStatementAST *ifStmt = ast->asIfStatement()) {
            if (ifStmt->symbol)
                return ifStmt->symbol;

        } else if (WhileStatementAST *whileStmt = ast->asWhileStatement()) {
            if (whileStmt->symbol)
                return whileStmt->symbol;

        } else if (ForStatementAST *forStmt = ast->asForStatement()) {
            if (forStmt->symbol)
                return forStmt->symbol;

        } else if (ForeachStatementAST *foreachStmt = ast->asForeachStatement()) {
            if (foreachStmt->symbol)
                return foreachStmt->symbol;

        } else if (SwitchStatementAST *switchStmt = ast->asSwitchStatement()) {
            if (switchStmt->symbol)
                return switchStmt->symbol;

        } else if (CatchClauseAST *catchClause = ast->asCatchClause()) {
            if (catchClause->symbol)
                return catchClause->symbol;

        }
    }

    return _doc->globalNamespace();
}

bool CheckSymbols::preVisit(AST *ast)
{
    _astStack.append(ast);

    if (isCanceled())
        return false;

    return true;
}

void CheckSymbols::postVisit(AST *)
{
    _astStack.takeLast();
}

bool CheckSymbols::visit(NamespaceAST *ast)
{
    if (ast->identifier_token) {
        const Token &tok = tokenAt(ast->identifier_token);
        if (! tok.generated()) {
            unsigned line, column;
            getTokenStartPosition(ast->identifier_token, &line, &column);
            Use use(line, column, tok.length());
            addUse(use);
        }
    }

    return true;
}

bool CheckSymbols::visit(UsingDirectiveAST *)
{
    return true;
}

bool CheckSymbols::visit(EnumeratorAST *ast)
{
    addUse(ast->identifier_token, Use::Static);
    return true;
}

bool CheckSymbols::visit(SimpleDeclarationAST *ast)
{
    if (ast->declarator_list && !ast->declarator_list->next) {
        if (ast->symbols && ! ast->symbols->next && !ast->symbols->value->isGenerated()) {
            Symbol *decl = ast->symbols->value;
            if (NameAST *declId = declaratorId(ast->declarator_list->value)) {
                if (Function *funTy = decl->type()->asFunctionType()) {
                    if (funTy->isVirtual()) {
                        addUse(declId, Use::VirtualMethod);
                    } else if (maybeVirtualMethod(decl->name())) {
                        addVirtualMethod(_context.lookup(decl->name(), decl->enclosingScope()), declId, funTy->argumentCount());
                    }
                }
            }
        }
    }

    return true;
}

bool CheckSymbols::visit(NamedTypeSpecifierAST *)
{
    return true;
}

bool CheckSymbols::visit(ElaboratedTypeSpecifierAST *ast)
{
    accept(ast->attribute_list);
    accept(ast->name);
    addUse(ast->name, Use::Type);
    return false;
}

bool CheckSymbols::visit(MemberAccessAST *ast)
{
    accept(ast->base_expression);
    if (! ast->member_name)
        return false;

    if (const Name *name = ast->member_name->name) {
        if (const Identifier *ident = name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialMembers.contains(id)) {
                const Token start = tokenAt(ast->firstToken());
                const Token end = tokenAt(ast->lastToken() - 1);
                const QByteArray expression = _doc->source().mid(start.begin(), end.end() - start.begin());

                const QList<LookupItem> candidates = typeOfExpression(expression, enclosingScope(), TypeOfExpression::Preprocess);
                addClassMember(candidates, ast->member_name);
            }
        }
    }

    return false;
}

bool CheckSymbols::visit(CallAST *ast)
{
    if (ast->base_expression) {
        accept(ast->base_expression);

        unsigned argumentCount = 0;

        for (ExpressionListAST *it = ast->expression_list; it; it = it->next)
            ++argumentCount;

        if (MemberAccessAST *access = ast->base_expression->asMemberAccess()) {
            if (access->member_name && access->member_name->name) {
                if (maybeVirtualMethod(access->member_name->name)) {
                    const QByteArray expression = textOf(access);

                    const QList<LookupItem> candidates = typeOfExpression(expression, enclosingScope(),
                                                                          TypeOfExpression::Preprocess);

                    NameAST *memberName = access->member_name;
                    if (QualifiedNameAST *q = memberName->asQualifiedName())
                        memberName = q->unqualified_name;

                    addVirtualMethod(candidates, memberName, argumentCount);
                }
            }
        } else if (IdExpressionAST *idExpr = ast->base_expression->asIdExpression()) {
            if (const Name *name = idExpr->name->name) {
                if (maybeVirtualMethod(name)) {
                    NameAST *exprName = idExpr->name;
                    if (QualifiedNameAST *q = exprName->asQualifiedName())
                        exprName = q->unqualified_name;

                    const QList<LookupItem> candidates = typeOfExpression(textOf(idExpr), enclosingScope(),
                                                                          TypeOfExpression::Preprocess);

                    addVirtualMethod(candidates, exprName, argumentCount);
                }
            }
        }

        accept(ast->expression_list);
    }

    return false;
}

QByteArray CheckSymbols::textOf(AST *ast) const
{
    const Token start = tokenAt(ast->firstToken());
    const Token end = tokenAt(ast->lastToken() - 1);
    const QByteArray text = _doc->source().mid(start.begin(), end.end() - start.begin());
    return text;
}

void CheckSymbols::checkNamespace(NameAST *name)
{
    if (! name)
        return;

    unsigned line, column;
    getTokenStartPosition(name->firstToken(), &line, &column);

    if (ClassOrNamespace *b = _context.lookupType(name->name, enclosingScope())) {
        foreach (Symbol *s, b->symbols()) {
            if (s->isNamespace())
                return;
        }
    }

    const unsigned length = tokenAt(name->lastToken() - 1).end() - tokenAt(name->firstToken()).begin();
    warning(line, column, QCoreApplication::translate("CheckUndefinedSymbols", "Expected a namespace-name"), length);
}

bool CheckSymbols::hasVirtualDestructor(Class *klass) const
{
    if (! klass)
        return false;
    const Identifier *id = klass->identifier();
    if (! id)
        return false;
    for (Symbol *s = klass->find(id); s; s = s->next()) {
        if (! s->name())
            continue;
        else if (s->name()->isDestructorNameId()) {
            if (Function *funTy = s->type()->asFunctionType()) {
                if (funTy->isVirtual() && id->isEqualTo(s->identifier()))
                    return true;
            }
        }
    }
    return false;
}

bool CheckSymbols::hasVirtualDestructor(ClassOrNamespace *binding) const
{
    QSet<ClassOrNamespace *> processed;
    QList<ClassOrNamespace *> todo;
    todo.append(binding);

    while (! todo.isEmpty()) {
        ClassOrNamespace *b = todo.takeFirst();
        if (b && ! processed.contains(b)) {
            processed.insert(b);
            foreach (Symbol *s, b->symbols()) {
                if (Class *k = s->asClass()) {
                    if (hasVirtualDestructor(k))
                        return true;
                }
            }

            todo += b->usings();
        }
    }

    return false;
}

void CheckSymbols::checkName(NameAST *ast, Scope *scope)
{
    if (ast && ast->name) {
        if (! scope)
            scope = enclosingScope();

        if (ast->asDestructorName() != 0) {
            Class *klass = scope->asClass();
            if (hasVirtualDestructor(_context.lookupType(klass)))
                addUse(ast, Use::VirtualMethod);
        } else if (maybeType(ast->name) || maybeStatic(ast->name)) {
            const QList<LookupItem> candidates = _context.lookup(ast->name, scope);
            addTypeOrStatic(candidates, ast);
        } else if (maybeMember(ast->name)) {
            const QList<LookupItem> candidates = _context.lookup(ast->name, scope);
            addClassMember(candidates, ast);
        }
    }
}

bool CheckSymbols::visit(SimpleNameAST *ast)
{
    checkName(ast);
    return true;
}

bool CheckSymbols::visit(TemplateIdAST *ast)
{
    checkName(ast);
    return true;
}

bool CheckSymbols::visit(DestructorNameAST *ast)
{
    checkName(ast);
    return true;
}

bool CheckSymbols::visit(QualifiedNameAST *ast)
{
    if (ast->name) {
        ClassOrNamespace *binding = 0;
        if (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list) {
            NestedNameSpecifierAST *nested_name_specifier = it->value;
            if (NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name) { // ### remove shadowing

                if (TemplateIdAST *template_id = class_or_namespace_name->asTemplateId())
                    accept(template_id->template_argument_list);

                const Name *name = class_or_namespace_name->name;
                binding = _context.lookupType(name, enclosingScope());
                addType(binding, class_or_namespace_name);

                for (it = it->next; it; it = it->next) {
                    NestedNameSpecifierAST *nested_name_specifier = it->value;

                    if (NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name) {
                        if (TemplateIdAST *template_id = class_or_namespace_name->asTemplateId()) {
                            if (template_id->template_token) {
                                addUse(template_id, Use::Type);
                                binding = 0; // there's no way we can find a binding.
                            }

                            accept(template_id->template_argument_list);
                            if (! binding)
                                continue;
                        }

                        if (binding) {
                            binding = binding->findType(class_or_namespace_name->name);
                            addType(binding, class_or_namespace_name);
                        }
                    }
                }
            }
        }

        if (binding && ast->unqualified_name) {
            if (ast->unqualified_name->asDestructorName() != 0) {
                if (hasVirtualDestructor(binding))
                    addUse(ast->unqualified_name, Use::VirtualMethod);
            } else {
                addTypeOrStatic(binding->find(ast->unqualified_name->name), ast->unqualified_name);
            }

            if (TemplateIdAST *template_id = ast->unqualified_name->asTemplateId())
                accept(template_id->template_argument_list);
        }
    }

    return false;
}

bool CheckSymbols::visit(TypenameTypeParameterAST *ast)
{
    addUse(ast->name, Use::Type);
    accept(ast->type_id);
    return false;
}

bool CheckSymbols::visit(TemplateTypeParameterAST *ast)
{
    accept(ast->template_parameter_list);
    addUse(ast->name, Use::Type);
    accept(ast->type_id);
    return false;
}

bool CheckSymbols::visit(MemInitializerAST *ast)
{
    if (FunctionDefinitionAST *enclosingFunction = enclosingFunctionDefinition()) {
        if (ast->name && enclosingFunction->symbol) {
            if (ClassOrNamespace *binding = _context.lookupType(enclosingFunction->symbol)) {
                foreach (Symbol *s, binding->symbols()) {
                    if (Class *klass = s->asClass()){
                        checkName(ast->name, klass);
                        break;
                    }
                }
            }
        }

        accept(ast->expression_list);
    }

    return false;
}

bool CheckSymbols::visit(FunctionDefinitionAST *ast)
{
    AST *thisFunction = _astStack.takeLast();
    accept(ast->decl_specifier_list);
    _astStack.append(thisFunction);

    if (ast->declarator && ast->symbol && ! ast->symbol->isGenerated()) {
        Function *fun = ast->symbol;
        if (NameAST *declId = declaratorId(ast->declarator)) {
            if (QualifiedNameAST *q = declId->asQualifiedName())
                declId = q->unqualified_name;

            if (fun->isVirtual()) {
                addUse(declId, Use::VirtualMethod);
            } else if (maybeVirtualMethod(fun->name())) {
                addVirtualMethod(_context.lookup(fun->name(), fun->enclosingScope()), declId, fun->argumentCount());
            }
        }
    }

    accept(ast->declarator);
    accept(ast->ctor_initializer);
    accept(ast->function_body);

    const LocalSymbols locals(_doc, ast);
    foreach (const QList<SemanticInfo::Use> &uses, locals.uses) {
        foreach (const SemanticInfo::Use &u, uses)
            addUse(u);
    }

    if (!enclosingFunctionDefinition(true))
        flush();

    return false;
}

void CheckSymbols::addUse(NameAST *ast, Use::Kind kind)
{
    if (! ast)
        return;

    if (QualifiedNameAST *q = ast->asQualifiedName())
        ast = q->unqualified_name;

    if (! ast)
        return; // nothing to do
    else if (ast->asOperatorFunctionId() != 0 || ast->asConversionFunctionId() != 0)
        return; // nothing to do

    unsigned startToken = ast->firstToken();

    if (DestructorNameAST *dtor = ast->asDestructorName())
        startToken = dtor->identifier_token;

    else if (TemplateIdAST *templ = ast->asTemplateId())
        startToken = templ->identifier_token;

    addUse(startToken, kind);
}

void CheckSymbols::addUse(unsigned tokenIndex, Use::Kind kind)
{
    if (! tokenIndex)
        return;

    const Token &tok = tokenAt(tokenIndex);
    if (tok.generated())
        return;

    unsigned line, column;
    getTokenStartPosition(tokenIndex, &line, &column);
    const unsigned length = tok.length();

    const Use use(line, column, length, kind);
    addUse(use);
}

void CheckSymbols::addUse(const Use &use)
{
    if (! enclosingFunctionDefinition()) {
        if (_usages.size() >= 50) {
            if (_flushRequested && use.line != _flushLine)
                flush();
            else if (! _flushRequested) {
                _flushRequested = true;
                _flushLine = use.line;
            }
        }
    }

    _usages.append(use);
}

void CheckSymbols::addType(ClassOrNamespace *b, NameAST *ast)
{
    if (! b)
        return;

    unsigned startToken = ast->firstToken();
    if (DestructorNameAST *dtor = ast->asDestructorName())
        startToken = dtor->identifier_token;

    const Token &tok = tokenAt(startToken);
    if (tok.generated())
        return;

    unsigned line, column;
    getTokenStartPosition(startToken, &line, &column);
    const unsigned length = tok.length();
    const Use use(line, column, length, Use::Type);
    addUse(use);
    //qDebug() << "added use" << oo(ast->name) << line << column << length;
}

bool CheckSymbols::isTemplateClass(Symbol *symbol) const
{
    if (symbol) {
        if (Template *templ = symbol->asTemplate()) {
            if (Symbol *declaration = templ->declaration()) {
                if (declaration->isClass() || declaration->isForwardClassDeclaration())
                    return true;
            }
        }
    }
    return false;
}

void CheckSymbols::addTypeOrStatic(const QList<LookupItem> &candidates, NameAST *ast)
{
    unsigned startToken = ast->firstToken();
    if (DestructorNameAST *dtor = ast->asDestructorName())
        startToken = dtor->identifier_token;

    const Token &tok = tokenAt(startToken);
    if (tok.generated())
        return;

    foreach (const LookupItem &r, candidates) {
        Symbol *c = r.declaration();
        if (c->isUsingDeclaration()) // skip using declarations...
            continue;
        else if (c->isUsingNamespaceDirective()) // ... and using namespace directives.
            continue;
        else if (c->isTypedef() || c->isNamespace() ||
                 c->isClass() || c->isEnum() || isTemplateClass(c) ||
                 c->isForwardClassDeclaration() || c->isTypenameArgument() || c->enclosingEnum() != 0) {

            unsigned line, column;
            getTokenStartPosition(startToken, &line, &column);
            const unsigned length = tok.length();

            Use::Kind kind = Use::Type;

            if (c->enclosingEnum() != 0)
                kind = Use::Static;

            const Use use(line, column, length, kind);
            addUse(use);
            //qDebug() << "added use" << oo(ast->name) << line << column << length;
            break;
        }
    }
}

void CheckSymbols::addClassMember(const QList<LookupItem> &candidates, NameAST *ast)
{
    unsigned startToken = ast->firstToken();
    if (DestructorNameAST *dtor = ast->asDestructorName())
        startToken = dtor->identifier_token;

    const Token &tok = tokenAt(startToken);
    if (tok.generated())
        return;

    foreach (const LookupItem &r, candidates) {
        Symbol *c = r.declaration();
        if (! c)
            continue;
        else if (! c->isDeclaration())
            return;
        else if (! (c->enclosingScope() && c->enclosingScope()->isClass()))
            return; // shadowed
        else if (c->isTypedef() || c->type()->isFunctionType())
            return; // shadowed

        unsigned line, column;
        getTokenStartPosition(startToken, &line, &column);
        const unsigned length = tok.length();

        const Use use(line, column, length, Use::Field);
        addUse(use);
        break;
    }
}

void CheckSymbols::addStatic(const QList<LookupItem> &candidates, NameAST *ast)
{
    if (ast->asDestructorName() != 0)
        return;

    unsigned startToken = ast->firstToken();
    const Token &tok = tokenAt(startToken);
    if (tok.generated())
        return;

    foreach (const LookupItem &r, candidates) {
        Symbol *c = r.declaration();
        if (! c)
            return;
        if (c->enclosingScope()->isEnum()) {
            unsigned line, column;
            getTokenStartPosition(startToken, &line, &column);
            const unsigned length = tok.length();

            const Use use(line, column, length, Use::Static);
            addUse(use);
            //qDebug() << "added use" << oo(ast->name) << line << column << length;
            break;
        }
    }
}

void CheckSymbols::addVirtualMethod(const QList<LookupItem> &candidates, NameAST *ast, unsigned argumentCount)
{
    unsigned startToken = ast->firstToken();
    if (DestructorNameAST *dtor = ast->asDestructorName())
        startToken = dtor->identifier_token;

    const Token &tok = tokenAt(startToken);
    if (tok.generated())
        return;

    foreach (const LookupItem &r, candidates) {
        Symbol *c = r.declaration();
        if (! c)
            continue;

        Function *funTy = r.type()->asFunctionType();
        if (! funTy)
            continue;
        if (! funTy->isVirtual())
            continue;
        else if (argumentCount < funTy->minimumArgumentCount())
            continue;
        else if (argumentCount > funTy->argumentCount()) {
            if (! funTy->isVariadic())
                continue;
        }

        unsigned line, column;
        getTokenStartPosition(startToken, &line, &column);
        const unsigned length = tok.length();

        const Use use(line, column, length, Use::VirtualMethod);
        addUse(use);
        break;
    }
}

NameAST *CheckSymbols::declaratorId(DeclaratorAST *ast) const
{
    if (ast && ast->core_declarator) {
        if (NestedDeclaratorAST *nested = ast->core_declarator->asNestedDeclarator())
            return declaratorId(nested->declarator);
        else if (DeclaratorIdAST *declId = ast->core_declarator->asDeclaratorId()) {
            return declId->name;
        }
    }

    return 0;
}

bool CheckSymbols::maybeType(const Name *name) const
{
    if (name) {
        if (const Identifier *ident = name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialTypes.contains(id))
                return true;
        }
    }

    return false;
}

bool CheckSymbols::maybeMember(const Name *name) const
{
    if (name) {
        if (const Identifier *ident = name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialMembers.contains(id))
                return true;
        }
    }

    return false;
}

bool CheckSymbols::maybeStatic(const Name *name) const
{
    if (name) {
        if (const Identifier *ident = name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialStatics.contains(id))
                return true;
        }
    }

    return false;
}

bool CheckSymbols::maybeVirtualMethod(const Name *name) const
{
    if (name) {
        if (const Identifier *ident = name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialVirtualMethods.contains(id))
                return true;
        }
    }

    return false;
}

void CheckSymbols::flush()
{
    _flushRequested = false;
    _flushLine = 0;

    if (_usages.isEmpty())
        return;

    reportResults(_usages);
    _usages.clear();
}
