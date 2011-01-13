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

#ifndef CPLUSPLUS_LOOKUPCONTEXT_H
#define CPLUSPLUS_LOOKUPCONTEXT_H

#include "CppDocument.h"
#include "LookupItem.h"
#include <FullySpecifiedType.h>
#include <Type.h>
#include <SymbolVisitor.h>
#include <Control.h>
#include <Name.h>
#include <QtCore/QSet>
#include <map>
#include <functional>

namespace CPlusPlus {

class CreateBindings;

class CPLUSPLUS_EXPORT ClassOrNamespace
{
public:
    ClassOrNamespace(CreateBindings *factory, ClassOrNamespace *parent);

    const TemplateNameId *templateId() const;
    ClassOrNamespace *parent() const;
    QList<ClassOrNamespace *> usings() const;
    QList<Enum *> enums() const;
    QList<Symbol *> symbols() const;

    ClassOrNamespace *globalNamespace() const;

    QList<LookupItem> lookup(const Name *name);
    QList<LookupItem> find(const Name *name);

    ClassOrNamespace *lookupType(const Name *name);
    ClassOrNamespace *findType(const Name *name);

private:
    /// \internal
    void flush();

    /// \internal
    ClassOrNamespace *findOrCreateType(const Name *name);

    void addTodo(Symbol *symbol);
    void addSymbol(Symbol *symbol);
    void addEnum(Enum *e);
    void addUsing(ClassOrNamespace *u);
    void addNestedType(const Name *alias, ClassOrNamespace *e);

    QList<LookupItem> lookup_helper(const Name *name, bool searchInEnclosingScope);

    void lookup_helper(const Name *name, ClassOrNamespace *binding,
                       QList<LookupItem> *result,
                       QSet<ClassOrNamespace *> *processed,
                       const TemplateNameId *templateId);

    ClassOrNamespace *lookupType_helper(const Name *name, QSet<ClassOrNamespace *> *processed,
                                        bool searchInEnclosingScope);

    ClassOrNamespace *nestedType(const Name *name) const;

private:
    struct CompareName: std::binary_function<const Name *, const Name *, bool> {
        bool operator()(const Name *name, const Name *other) const;
    };

private:
    typedef std::map<const Name *, ClassOrNamespace *, CompareName> Table;
    CreateBindings *_factory;
    ClassOrNamespace *_parent;
    QList<Symbol *> _symbols;
    QList<ClassOrNamespace *> _usings;
    Table _classOrNamespaces;
    QList<Enum *> _enums;
    QList<Symbol *> _todo;

    // it's an instantiation.
    const TemplateNameId *_templateId;

    friend class CreateBindings;
};

class CPLUSPLUS_EXPORT CreateBindings: protected SymbolVisitor
{
    Q_DISABLE_COPY(CreateBindings)

public:
    CreateBindings(Document::Ptr thisDocument, const Snapshot &snapshot, QSharedPointer<Control> control);
    virtual ~CreateBindings();

    /// Returns the binding for the global namespace.
    ClassOrNamespace *globalNamespace() const;

    /// Finds the binding associated to the given symbol.
    ClassOrNamespace *lookupType(Symbol *symbol);
    ClassOrNamespace *lookupType(const QList<const Name *> &path);

    /// Returns the Control that must be used to create temporary symbols.
    /// \internal
    QSharedPointer<Control> control() const;

    /// Searches in \a scope for symbols with the given \a name.
    /// Store the result in \a results.
    /// \internal
    void lookupInScope(const Name *name, Scope *scope, QList<LookupItem> *result,
                            const TemplateNameId *templateId, ClassOrNamespace *binding);

    /// Create bindings for the symbols reachable from \a rootSymbol.
    /// \internal
    void process(Symbol *rootSymbol, ClassOrNamespace *classOrNamespace);

    /// Create an empty ClassOrNamespace binding with the given \a parent.
    /// \internal
    ClassOrNamespace *allocClassOrNamespace(ClassOrNamespace *parent);

protected:
    using SymbolVisitor::visit;

    /// Change the current ClassOrNamespace binding.
    ClassOrNamespace *switchCurrentClassOrNamespace(ClassOrNamespace *classOrNamespace);

    /// Enters the ClassOrNamespace binding associated with the given \a symbol.
    ClassOrNamespace *enterClassOrNamespaceBinding(Symbol *symbol);

    /// Enters a ClassOrNamespace binding for the given \a symbol in the global
    /// namespace binding.
    ClassOrNamespace *enterGlobalClassOrNamespace(Symbol *symbol);

    /// Creates bindings for the given \a document.
    void process(Document::Ptr document);

    /// Creates bindings for the symbols reachable from the \a root symbol.
    void process(Symbol *root);

    virtual bool visit(Template *templ);
    virtual bool visit(Namespace *ns);
    virtual bool visit(Class *klass);
    virtual bool visit(ForwardClassDeclaration *klass);
    virtual bool visit(Enum *e);
    virtual bool visit(Declaration *decl);
    virtual bool visit(Function *);
    virtual bool visit(BaseClass *b);
    virtual bool visit(UsingNamespaceDirective *u);
    virtual bool visit(UsingDeclaration *u);
    virtual bool visit(NamespaceAlias *a);

    virtual bool visit(ObjCClass *klass);
    virtual bool visit(ObjCBaseClass *b);
    virtual bool visit(ObjCForwardClassDeclaration *klass);
    virtual bool visit(ObjCProtocol *proto);
    virtual bool visit(ObjCBaseProtocol *b);
    virtual bool visit(ObjCForwardProtocolDeclaration *proto);
    virtual bool visit(ObjCMethod *);

private:
    Snapshot _snapshot;
    QSharedPointer<Control> _control;
    QSet<Namespace *> _processed;
    QList<ClassOrNamespace *> _entities;
    ClassOrNamespace *_globalNamespace;
    ClassOrNamespace *_currentClassOrNamespace;
};

class CPLUSPLUS_EXPORT LookupContext
{
public:
    LookupContext();

    LookupContext(Document::Ptr thisDocument,
                  const Snapshot &snapshot);

    LookupContext(Document::Ptr expressionDocument,
                  Document::Ptr thisDocument,
                  const Snapshot &snapshot);

    LookupContext(const LookupContext &other);
    LookupContext &operator = (const LookupContext &other);

    Document::Ptr expressionDocument() const;
    Document::Ptr thisDocument() const;
    Document::Ptr document(const QString &fileName) const;
    Snapshot snapshot() const;

    ClassOrNamespace *globalNamespace() const;

    QList<LookupItem> lookup(const Name *name, Scope *scope) const;
    ClassOrNamespace *lookupType(const Name *name, Scope *scope) const;
    ClassOrNamespace *lookupType(Symbol *symbol) const;
    ClassOrNamespace *lookupParent(Symbol *symbol) const;

    /// \internal
    QSharedPointer<CreateBindings> bindings() const;

    /// \internal
    void setBindings(QSharedPointer<CreateBindings> bindings);

    QSharedPointer<Control> control() const; // ### deprecate

    static QList<const Name *> fullyQualifiedName(Symbol *symbol);
    static QList<const Name *> path(Symbol *symbol);

    const Name *minimalName(const Name *name, Scope *source,
                            ClassOrNamespace *target) const;

private:
    // The current expression.
    Document::Ptr _expressionDocument;

    // The current document.
    Document::Ptr _thisDocument;

    // All documents.
    Snapshot _snapshot;

    // Bindings
    mutable QSharedPointer<CreateBindings> _bindings;

    QSharedPointer<Control> _control;
};

} // end of namespace CPlusPlus

#endif // CPLUSPLUS_LOOKUPCONTEXT_H
