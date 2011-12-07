/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CPLUSPLUS_CHECKSYMBOLS_H
#define CPLUSPLUS_CHECKSYMBOLS_H

#include "cppsemanticinfo.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/TypeOfExpression.h>

#include <ASTVisitor.h>
#include <QtCore/QSet>
#include <QtCore/QFuture>
#include <QtCore/QtConcurrentRun>

namespace CPlusPlus {

class CheckSymbols:
        protected ASTVisitor,
        public QRunnable,
        public QFutureInterface<CppEditor::Internal::SemanticInfo::Use>
{
public:
    virtual ~CheckSymbols();

    typedef CppEditor::Internal::SemanticInfo::Use Use;
    typedef CppEditor::Internal::SemanticInfo::UseKind UseKind;

    virtual void run();

    typedef QFuture<Use> Future;

    Future start()
    {
        this->setRunnable(this);
        this->reportStarted();
        Future future = this->future();
        QThreadPool::globalInstance()->start(this, QThread::LowestPriority);
        return future;
    }

    static Future go(Document::Ptr doc, const LookupContext &context);

    static QMap<int, QVector<Use> > chunks(const QFuture<Use> &future, int from, int to)
    {
        QMap<int, QVector<Use> > chunks;

        for (int i = from; i < to; ++i) {
            const Use use = future.resultAt(i);
            if (! use.line)
                continue; // skip it, it's an invalid use.

            const int blockNumber = use.line - 1;
            chunks[blockNumber].append(use);
        }

        return chunks;
    }

protected:
    using ASTVisitor::visit;
    using ASTVisitor::endVisit;

    CheckSymbols(Document::Ptr doc, const LookupContext &context);

    bool hasVirtualDestructor(Class *klass) const;
    bool hasVirtualDestructor(ClassOrNamespace *binding) const;

    bool warning(unsigned line, unsigned column, const QString &text, unsigned length = 0);
    bool warning(AST *ast, const QString &text);

    QByteArray textOf(AST *ast) const;

    bool maybeType(const Name *name) const;
    bool maybeMember(const Name *name) const;
    bool maybeStatic(const Name *name) const;
    bool maybeVirtualMethod(const Name *name) const;

    void checkName(NameAST *ast, Scope *scope = 0);
    void checkNamespace(NameAST *name);

    void addUse(const Use &use);
    void addUse(unsigned tokenIndex, UseKind kind);
    void addUse(NameAST *name, UseKind kind);

    void addType(ClassOrNamespace *b, NameAST *ast);

    void addTypeOrStatic(const QList<LookupItem> &candidates, NameAST *ast);
    void addStatic(const QList<LookupItem> &candidates, NameAST *ast);
    void addClassMember(const QList<LookupItem> &candidates, NameAST *ast);
    void addVirtualMethod(const QList<LookupItem> &candidates, NameAST *ast, unsigned argumentCount);

    bool isTemplateClass(Symbol *s) const;

    Scope *enclosingScope() const;
    FunctionDefinitionAST *enclosingFunctionDefinition(bool skipTopOfStack = false) const;
    TemplateDeclarationAST *enclosingTemplateDeclaration() const;

    virtual bool preVisit(AST *);
    virtual void postVisit(AST *);

    virtual bool visit(NamespaceAST *);
    virtual bool visit(UsingDirectiveAST *);
    virtual bool visit(SimpleDeclarationAST *);
    virtual bool visit(NamedTypeSpecifierAST *);
    virtual bool visit(ElaboratedTypeSpecifierAST *ast);

    virtual bool visit(EnumeratorAST *);

    virtual bool visit(SimpleNameAST *ast);
    virtual bool visit(DestructorNameAST *ast);
    virtual bool visit(QualifiedNameAST *ast);
    virtual bool visit(TemplateIdAST *ast);

    virtual bool visit(TypenameTypeParameterAST *ast);
    virtual bool visit(TemplateTypeParameterAST *ast);

    virtual bool visit(FunctionDefinitionAST *ast);
    virtual bool visit(MemberAccessAST *ast);
    virtual bool visit(CallAST *ast);

    virtual bool visit(MemInitializerAST *ast);

    NameAST *declaratorId(DeclaratorAST *ast) const;

    void flush();

private:
    Document::Ptr _doc;
    LookupContext _context;
    TypeOfExpression typeOfExpression;
    QString _fileName;
    QList<Document::DiagnosticMessage> _diagnosticMessages;
    QSet<QByteArray> _potentialTypes;
    QSet<QByteArray> _potentialMembers;
    QSet<QByteArray> _potentialVirtualMethods;
    QSet<QByteArray> _potentialStatics;
    QList<AST *> _astStack;
    QVector<Use> _usages;
    unsigned _lineOfLastUsage;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_CHECKSYMBOLS_H
