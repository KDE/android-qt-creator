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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "LookupContext.h"
#include "ResolveExpression.h"
#include "Overview.h"
#include "DeprecatedGenTemplateInstance.h"

#include <CoreTypes.h>
#include <Symbols.h>
#include <Literals.h>
#include <Names.h>
#include <Scope.h>
#include <Control.h>

#include <QtCore/QStack>
#include <QtCore/QHash>
#include <QtCore/QVarLengthArray>
#include <QtCore/QtDebug>

using namespace CPlusPlus;

namespace {
const bool debug = ! qgetenv("CPLUSPLUS_LOOKUPCONTEXT_DEBUG").isEmpty();
} // end of anonymous namespace


static void addNames(const Name *name, QList<const Name *> *names, bool addAllNames = false)
{
    if (! name)
        return;
    else if (const QualifiedNameId *q = name->asQualifiedNameId()) {
        addNames(q->base(), names);
        addNames(q->name(), names, addAllNames);
    } else if (addAllNames || name->isNameId() || name->isTemplateNameId()) {
        names->append(name);
    }
}

static void path_helper(Symbol *symbol, QList<const Name *> *names)
{
    if (! symbol)
        return;

    path_helper(symbol->enclosingScope(), names);

    if (symbol->name()) {
        if (symbol->isClass() || symbol->isNamespace()) {
            addNames(symbol->name(), names);

        } else if (symbol->isObjCClass() || symbol->isObjCBaseClass() || symbol->isObjCProtocol()
                || symbol->isObjCForwardClassDeclaration() || symbol->isObjCForwardProtocolDeclaration()
                || symbol->isForwardClassDeclaration()) {
            addNames(symbol->name(), names);

        } else if (symbol->isFunction()) {
            if (const QualifiedNameId *q = symbol->name()->asQualifiedNameId())
                addNames(q->base(), names);
        }
    }
}

bool ClassOrNamespace::CompareName::operator()(const Name *name, const Name *other) const
{
    Q_ASSERT(name != 0);
    Q_ASSERT(other != 0);

    const Identifier *id = name->identifier();
    const Identifier *otherId = other->identifier();
    return strcmp(id->chars(), otherId->chars()) < 0;
}

/////////////////////////////////////////////////////////////////////
// LookupContext
/////////////////////////////////////////////////////////////////////
LookupContext::LookupContext()
    : _control(new Control())
{ }

LookupContext::LookupContext(Document::Ptr thisDocument,
                             const Snapshot &snapshot)
    : _expressionDocument(Document::create("<LookupContext>")),
      _thisDocument(thisDocument),
      _snapshot(snapshot),
      _control(new Control())
{
}

LookupContext::LookupContext(Document::Ptr expressionDocument,
                             Document::Ptr thisDocument,
                             const Snapshot &snapshot)
    : _expressionDocument(expressionDocument),
      _thisDocument(thisDocument),
      _snapshot(snapshot),
      _control(new Control())
{
}

LookupContext::LookupContext(const LookupContext &other)
    : _expressionDocument(other._expressionDocument),
      _thisDocument(other._thisDocument),
      _snapshot(other._snapshot),
      _bindings(other._bindings),
      _control(other._control)
{ }

LookupContext &LookupContext::operator = (const LookupContext &other)
{
    _expressionDocument = other._expressionDocument;
    _thisDocument = other._thisDocument;
    _snapshot = other._snapshot;
    _bindings = other._bindings;
    _control = other._control;
    return *this;
}

QList<const Name *> LookupContext::fullyQualifiedName(Symbol *symbol)
{
    QList<const Name *> qualifiedName = path(symbol->enclosingScope());
    addNames(symbol->name(), &qualifiedName, /*add all names*/ true);
    return qualifiedName;
}

QList<const Name *> LookupContext::path(Symbol *symbol)
{
    QList<const Name *> names;
    path_helper(symbol, &names);
    return names;
}


const Name *LookupContext::minimalName(const Name *name,
                                       Scope *scope,
                                       ClassOrNamespace *target) const
{
    Q_UNUSED(name);
    Q_UNUSED(scope);
    Q_UNUSED(target);

    qWarning() << "TODO:" << Q_FUNC_INFO;
    return name;

#if 0
    Q_ASSERT(name);
    Q_ASSERT(source);
    Q_ASSERT(target);

    QList<Symbol *> symbols = lookup(name, source);
    if (symbols.isEmpty())
        return 0;

    Symbol *canonicalSymbol = symbols.first();
    std::vector<const Name *> fqNames = fullyQualifiedName(canonicalSymbol).toVector().toStdVector();
    if (const QualifiedNameId *qId = name->asQualifiedNameId())
        fqNames.push_back(qId->name());
    else
        fqNames.push_back(name);

    const QualifiedNameId *lastWorking = 0;
    for (unsigned i = 0; i < fqNames.size(); ++i) {
        const QualifiedNameId *newName = control()->qualifiedNameId(&fqNames[i],
                                                                    fqNames.size() - i);
        QList<Symbol *> candidates = target->lookup(newName);
        if (candidates.contains(canonicalSymbol))
            lastWorking = newName;
        else
            break;
    }

    if (lastWorking && lastWorking->nameCount() == 1)
        return lastWorking->nameAt(0);
    else
        return lastWorking;
#endif
}


QSharedPointer<CreateBindings> LookupContext::bindings() const
{
    if (! _bindings)
        _bindings = QSharedPointer<CreateBindings>(new CreateBindings(_thisDocument, _snapshot, control()));

    return _bindings;
}

void LookupContext::setBindings(QSharedPointer<CreateBindings> bindings)
{
    _bindings = bindings;
}

QSharedPointer<Control> LookupContext::control() const
{
    return _control;
}

Document::Ptr LookupContext::expressionDocument() const
{ return _expressionDocument; }

Document::Ptr LookupContext::thisDocument() const
{ return _thisDocument; }

Document::Ptr LookupContext::document(const QString &fileName) const
{ return _snapshot.document(fileName); }

Snapshot LookupContext::snapshot() const
{ return _snapshot; }

ClassOrNamespace *LookupContext::globalNamespace() const
{
    return bindings()->globalNamespace();
}

ClassOrNamespace *LookupContext::lookupType(const Name *name, Scope *scope) const
{
    if (! scope) {
        return 0;
    } else if (Block *block = scope->asBlock()) {
        for (unsigned i = 0; i < block->memberCount(); ++i) {
            if (UsingNamespaceDirective *u = block->memberAt(i)->asUsingNamespaceDirective()) {
                if (ClassOrNamespace *uu = lookupType(u->name(), scope->enclosingNamespace())) {
                    if (ClassOrNamespace *r = uu->lookupType(name))
                        return r;
                }
            }
        }
        return lookupType(name, scope->enclosingScope());
    } else if (ClassOrNamespace *b = bindings()->lookupType(scope)) {
        return b->lookupType(name);
    }

    return 0;
}

ClassOrNamespace *LookupContext::lookupType(Symbol *symbol) const
{
    return bindings()->lookupType(symbol);
}

QList<LookupItem> LookupContext::lookup(const Name *name, Scope *scope) const
{
    QList<LookupItem> candidates;

    if (! name)
        return candidates;

    for (; scope; scope = scope->enclosingScope()) {
        if (name->identifier() != 0 && scope->isBlock()) {
            bindings()->lookupInScope(name, scope, &candidates, /*templateId = */ 0, /*binding=*/ 0);

            if (! candidates.isEmpty())
                break; // it's a local.

            for (unsigned i = 0; i < scope->memberCount(); ++i) {
                if (UsingNamespaceDirective *u = scope->memberAt(i)->asUsingNamespaceDirective()) {
                    if (ClassOrNamespace *uu = lookupType(u->name(), scope->enclosingNamespace())) {
                        candidates = uu->find(name);

                        if (! candidates.isEmpty())
                            return candidates;
                    }
                }
            }

        } else if (Function *fun = scope->asFunction()) {
            bindings()->lookupInScope(name, fun, &candidates, /*templateId = */ 0, /*binding=*/ 0);

            if (! candidates.isEmpty())
                break; // it's an argument or a template parameter.

            if (fun->name() && fun->name()->isQualifiedNameId()) {
                if (ClassOrNamespace *binding = bindings()->lookupType(fun)) {
                    candidates = binding->find(name);

                    if (! candidates.isEmpty())
                        return candidates;
                }
            }

            // contunue, and look at the enclosing scope.

        } else if (ObjCMethod *method = scope->asObjCMethod()) {
            bindings()->lookupInScope(name, method, &candidates, /*templateId = */ 0, /*binding=*/ 0);

            if (! candidates.isEmpty())
                break; // it's a formal argument.

        } else if (Template *templ = scope->asTemplate()) {
            bindings()->lookupInScope(name, templ, &candidates, /*templateId = */ 0, /*binding=*/ 0);

            if (! candidates.isEmpty())
                return candidates;  // it's a template parameter.

        } else if (Class *klass = scope->asClass()) {

            if (ClassOrNamespace *binding = bindings()->lookupType(klass)) {
                candidates = binding->find(name);

                if (! candidates.isEmpty())
                    return candidates;
            }

        } else if (Namespace *ns = scope->asNamespace()) {
            if (ClassOrNamespace *binding = bindings()->lookupType(ns))
                candidates = binding->find(name);

                if (! candidates.isEmpty())
                    return candidates;

        } else if (scope->isObjCClass() || scope->isObjCProtocol()) {
            if (ClassOrNamespace *binding = bindings()->lookupType(scope))
                candidates = binding->find(name);

                if (! candidates.isEmpty())
                    return candidates;
        }
    }

    return candidates;
}

ClassOrNamespace *LookupContext::lookupParent(Symbol *symbol) const
{
    QList<const Name *> fqName = path(symbol);
    ClassOrNamespace *binding = globalNamespace();
    foreach (const Name *name, fqName) {
        binding = binding->findType(name);
        if (!binding)
            return 0;
    }

    return binding;
}

ClassOrNamespace::ClassOrNamespace(CreateBindings *factory, ClassOrNamespace *parent)
    : _factory(factory), _parent(parent), _templateId(0)
{
}

const TemplateNameId *ClassOrNamespace::templateId() const
{
    return _templateId;
}

ClassOrNamespace *ClassOrNamespace::parent() const
{
    return _parent;
}

QList<ClassOrNamespace *> ClassOrNamespace::usings() const
{
    const_cast<ClassOrNamespace *>(this)->flush();
    return _usings;
}

QList<Enum *> ClassOrNamespace::enums() const
{
    const_cast<ClassOrNamespace *>(this)->flush();
    return _enums;
}

QList<Symbol *> ClassOrNamespace::symbols() const
{
    if (_templateId && ! _usings.isEmpty())
        return _usings.first()->symbols(); // ask to the base implementation

    const_cast<ClassOrNamespace *>(this)->flush();
    return _symbols;
}

ClassOrNamespace *ClassOrNamespace::globalNamespace() const
{
    ClassOrNamespace *e = const_cast<ClassOrNamespace *>(this);

    do {
        if (! e->_parent)
            break;

        e = e->_parent;
    } while (e);

    return e;
}

QList<LookupItem> ClassOrNamespace::find(const Name *name)
{
    return lookup_helper(name, false);
}

QList<LookupItem> ClassOrNamespace::lookup(const Name *name)
{
    return lookup_helper(name, true);
}

QList<LookupItem> ClassOrNamespace::lookup_helper(const Name *name, bool searchInEnclosingScope)
{
    QList<LookupItem> result;

    if (name) {
        if (const QualifiedNameId *q = name->asQualifiedNameId()) {
            if (! q->base())
                result = globalNamespace()->find(q->name());

            else if (ClassOrNamespace *binding = lookupType(q->base()))
                result = binding->find(q->name());

            return result;
        }

        QSet<ClassOrNamespace *> processed;
        ClassOrNamespace *binding = this;
        do {
            lookup_helper(name, binding, &result, &processed, /*templateId = */ 0);
            binding = binding->_parent;
        } while (searchInEnclosingScope && binding);
    }

    return result;
}

void ClassOrNamespace::lookup_helper(const Name *name, ClassOrNamespace *binding,
                                          QList<LookupItem> *result,
                                          QSet<ClassOrNamespace *> *processed,
                                          const TemplateNameId *templateId)
{
    if (binding && ! processed->contains(binding)) {
        processed->insert(binding);

        const Identifier *nameId = name->identifier();

        foreach (Symbol *s, binding->symbols()) {
            if (s->isFriend())
                continue;
            else if (s->isUsingNamespaceDirective())
                continue;

            if (Scope *scope = s->asScope()) {
                if (Class *klass = scope->asClass()) {
                    if (const Identifier *id = klass->identifier()) {
                        if (nameId && nameId->isEqualTo(id)) {
                            LookupItem item;
                            item.setDeclaration(klass);
                            item.setBinding(binding);
                            result->append(item);
                        }
                    }
                }
                _factory->lookupInScope(name, scope, result, templateId, binding);
            }
        }

        foreach (Enum *e, binding->enums())
            _factory->lookupInScope(name, e, result, templateId, binding);

        foreach (ClassOrNamespace *u, binding->usings())
            lookup_helper(name, u, result, processed, binding->_templateId);
    }
}

void CreateBindings::lookupInScope(const Name *name, Scope *scope,
                                   QList<LookupItem> *result,
                                   const TemplateNameId *templateId,
                                   ClassOrNamespace *binding)
{
    Q_UNUSED(templateId);

    if (! name) {
        return;

    } else if (const OperatorNameId *op = name->asOperatorNameId()) {
        for (Symbol *s = scope->find(op->kind()); s; s = s->next()) {
            if (! s->name())
                continue;
            else if (s->isFriend())
                continue;
            else if (! s->name()->isEqualTo(op))
                continue;

            LookupItem item;
            item.setDeclaration(s);
            item.setBinding(binding);
            result->append(item);
        }

    } else if (const Identifier *id = name->identifier()) {
        for (Symbol *s = scope->find(id); s; s = s->next()) {
            if (s->isFriend())
                continue; // skip friends
            else if (s->isUsingNamespaceDirective())
                continue; // skip using namespace directives
            else if (! id->isEqualTo(s->identifier()))
                continue;
            else if (s->name()->isQualifiedNameId())
                continue; // skip qualified ids.

            LookupItem item;
            item.setDeclaration(s);
            item.setBinding(binding);

            if (templateId && (s->isDeclaration() || s->isFunction())) {
                FullySpecifiedType ty = DeprecatedGenTemplateInstance::instantiate(templateId, s, _control);
                item.setType(ty); // override the type.
            }

            result->append(item);
        }
    }
}

ClassOrNamespace *ClassOrNamespace::lookupType(const Name *name)
{
    if (! name)
        return 0;

    QSet<ClassOrNamespace *> processed;
    return lookupType_helper(name, &processed, /*searchInEnclosingScope =*/ true);
}

ClassOrNamespace *ClassOrNamespace::findType(const Name *name)
{
    QSet<ClassOrNamespace *> processed;
    return lookupType_helper(name, &processed, /*searchInEnclosingScope =*/ false);
}

ClassOrNamespace *ClassOrNamespace::lookupType_helper(const Name *name,
                                                      QSet<ClassOrNamespace *> *processed,
                                                      bool searchInEnclosingScope)
{
    if (const QualifiedNameId *q = name->asQualifiedNameId()) {

        if (! q->base())
            return globalNamespace()->findType(q->name());

        else if (ClassOrNamespace *binding = lookupType(q->base()))
            return binding->findType(q->name());

        return 0;

    } else if (! processed->contains(this)) {
        processed->insert(this);

        if (name->isNameId() || name->isTemplateNameId()) {
            flush();

            foreach (Symbol *s, symbols()) {
                if (Class *klass = s->asClass()) {
                    if (klass->identifier() && klass->identifier()->isEqualTo(name->identifier()))
                        return this;
                }
            }

            if (ClassOrNamespace *e = nestedType(name))
                return e;

            else if (_templateId) {
                if (_usings.size() == 1) {
                    ClassOrNamespace *delegate = _usings.first();

                    if (ClassOrNamespace *r = delegate->lookupType_helper(name, processed, /*searchInEnclosingScope = */ true))
                        return r;
                } else {
                    if (debug)
                        qWarning() << "expected one using declaration. Number of using declarations is:" << _usings.size();
                }
            }

            foreach (ClassOrNamespace *u, usings()) {
                if (ClassOrNamespace *r = u->lookupType_helper(name, processed, /*searchInEnclosingScope =*/ false))
                    return r;
            }
        }

        if (_parent && searchInEnclosingScope)
            return _parent->lookupType_helper(name, processed, searchInEnclosingScope);
    }

    return 0;
}

ClassOrNamespace *ClassOrNamespace::nestedType(const Name *name) const
{
    Q_ASSERT(name != 0);
    Q_ASSERT(name->isNameId() || name->isTemplateNameId());

    const_cast<ClassOrNamespace *>(this)->flush();

    Table::const_iterator it = _classOrNamespaces.find(name);

    if (it == _classOrNamespaces.end())
        return 0;

    ClassOrNamespace *c = it->second;

    if (const TemplateNameId *templId = name->asTemplateNameId()) {
        ClassOrNamespace *i = _factory->allocClassOrNamespace(c);
        i->_templateId = templId;
        i->_usings.append(c);
        return i;
    }

    return c;
}

void ClassOrNamespace::flush()
{
    if (! _todo.isEmpty()) {
        const QList<Symbol *> todo = _todo;
        _todo.clear();

        foreach (Symbol *member, todo)
            _factory->process(member, this);
    }
}

void ClassOrNamespace::addSymbol(Symbol *symbol)
{
    _symbols.append(symbol);
}

void ClassOrNamespace::addTodo(Symbol *symbol)
{
    _todo.append(symbol);
}

void ClassOrNamespace::addEnum(Enum *e)
{
    _enums.append(e);
}

void ClassOrNamespace::addUsing(ClassOrNamespace *u)
{
    _usings.append(u);
}

void ClassOrNamespace::addNestedType(const Name *alias, ClassOrNamespace *e)
{
    _classOrNamespaces[alias] = e;
}

ClassOrNamespace *ClassOrNamespace::findOrCreateType(const Name *name)
{
    if (! name)
        return this;

    if (const QualifiedNameId *q = name->asQualifiedNameId()) {
        if (! q->base())
            return globalNamespace()->findOrCreateType(q->name());

        return findOrCreateType(q->base())->findOrCreateType(q->name());

    } else if (name->isNameId() || name->isTemplateNameId()) {
        ClassOrNamespace *e = nestedType(name);

        if (! e) {
            e = _factory->allocClassOrNamespace(this);
            _classOrNamespaces[name] = e;
        }

        return e;
    }

    return 0;
}

CreateBindings::CreateBindings(Document::Ptr thisDocument, const Snapshot &snapshot, QSharedPointer<Control> control)
    : _snapshot(snapshot), _control(control)
{
    _globalNamespace = allocClassOrNamespace(/*parent = */ 0);
    _currentClassOrNamespace = _globalNamespace;

    process(thisDocument);
}

CreateBindings::~CreateBindings()
{
    qDeleteAll(_entities);
}

ClassOrNamespace *CreateBindings::switchCurrentClassOrNamespace(ClassOrNamespace *classOrNamespace)
{
    ClassOrNamespace *previous = _currentClassOrNamespace;
    _currentClassOrNamespace = classOrNamespace;
    return previous;
}

ClassOrNamespace *CreateBindings::globalNamespace() const
{
    return _globalNamespace;
}

ClassOrNamespace *CreateBindings::lookupType(Symbol *symbol)
{
    const QList<const Name *> path = LookupContext::path(symbol);
    return lookupType(path);
}

ClassOrNamespace *CreateBindings::lookupType(const QList<const Name *> &path)
{
    if (path.isEmpty())
        return _globalNamespace;

    ClassOrNamespace *b = _globalNamespace->lookupType(path.at(0));

    for (int i = 1; b && i < path.size(); ++i)
        b = b->findType(path.at(i));

    return b;
}

void CreateBindings::process(Symbol *s, ClassOrNamespace *classOrNamespace)
{
    ClassOrNamespace *previous = switchCurrentClassOrNamespace(classOrNamespace);
    accept(s);
    (void) switchCurrentClassOrNamespace(previous);
}

void CreateBindings::process(Symbol *symbol)
{
    _currentClassOrNamespace->addTodo(symbol);
}

QSharedPointer<Control> CreateBindings::control() const
{
    return _control;
}

ClassOrNamespace *CreateBindings::allocClassOrNamespace(ClassOrNamespace *parent)
{
    ClassOrNamespace *e = new ClassOrNamespace(this, parent);
    _entities.append(e);
    return e;
}

void CreateBindings::process(Document::Ptr doc)
{
    if (! doc)
        return;

    else if (Namespace *globalNamespace = doc->globalNamespace()) {
        if (! _processed.contains(globalNamespace)) {
            _processed.insert(globalNamespace);

            foreach (const Document::Include &i, doc->includes()) {
                if (Document::Ptr incl = _snapshot.document(i.fileName()))
                    process(incl);
            }

            accept(globalNamespace);
        }
    }
}

ClassOrNamespace *CreateBindings::enterClassOrNamespaceBinding(Symbol *symbol)
{
    ClassOrNamespace *entity = _currentClassOrNamespace->findOrCreateType(symbol->name());
    entity->addSymbol(symbol);

    return switchCurrentClassOrNamespace(entity);
}

ClassOrNamespace *CreateBindings::enterGlobalClassOrNamespace(Symbol *symbol)
{
    ClassOrNamespace *entity = _globalNamespace->findOrCreateType(symbol->name());
    entity->addSymbol(symbol);

    return switchCurrentClassOrNamespace(entity);
}

bool CreateBindings::visit(Template *templ)
{
    if (Symbol *d = templ->declaration())
        accept(d);

    return false;
}

bool CreateBindings::visit(Namespace *ns)
{
    ClassOrNamespace *previous = enterClassOrNamespaceBinding(ns);

    for (unsigned i = 0; i < ns->memberCount(); ++i)
        process(ns->memberAt(i));

    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(Class *klass)
{
    ClassOrNamespace *previous = _currentClassOrNamespace;
    ClassOrNamespace *binding = 0;

    if (klass->name() && klass->name()->isQualifiedNameId())
        binding = _currentClassOrNamespace->lookupType(klass->name());

    if (! binding)
        binding = _currentClassOrNamespace->findOrCreateType(klass->name());

    _currentClassOrNamespace = binding;
    _currentClassOrNamespace->addSymbol(klass);

    for (unsigned i = 0; i < klass->baseClassCount(); ++i)
        process(klass->baseClassAt(i));

    for (unsigned i = 0; i < klass->memberCount(); ++i)
        process(klass->memberAt(i));

    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(ForwardClassDeclaration *klass)
{
    if (! klass->isFriend()) {
        ClassOrNamespace *previous = enterClassOrNamespaceBinding(klass);
        _currentClassOrNamespace = previous;
    }

    return false;
}

bool CreateBindings::visit(Enum *e)
{
    _currentClassOrNamespace->addEnum(e);
    return false;
}

bool CreateBindings::visit(Declaration *decl)
{
    if (decl->isTypedef()) {
        FullySpecifiedType ty = decl->type();
        const Identifier *typedefId = decl->identifier();

        if (typedefId && ! (ty.isConst() || ty.isVolatile())) {
            if (const NamedType *namedTy = ty->asNamedType()) {
                if (ClassOrNamespace *e = _currentClassOrNamespace->lookupType(namedTy->name())) {
                    _currentClassOrNamespace->addNestedType(decl->name(), e);
                } else if (false) {
                    Overview oo;
                    qDebug() << "found entity not found for" << oo(namedTy->name());
                }
            } else if (Class *klass = ty->asClassType()) {
                if (const Identifier *nameId = decl->name()->asNameId()) {
                    ClassOrNamespace *binding = _currentClassOrNamespace->findOrCreateType(nameId);
                    binding->addSymbol(klass);
                }
            }
        }
    }

    return false;
}

bool CreateBindings::visit(Function *)
{
    return false;
}

bool CreateBindings::visit(BaseClass *b)
{
    if (ClassOrNamespace *base = _currentClassOrNamespace->lookupType(b->name())) {
        _currentClassOrNamespace->addUsing(base);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for:" << oo(b->name());
    }
    return false;
}

bool CreateBindings::visit(UsingDeclaration *u)
{
    if (u->name()) {
        if (const QualifiedNameId *q = u->name()->asQualifiedNameId()) {
            if (const Identifier *unqualifiedId = q->name()->asNameId()) {
                if (ClassOrNamespace *delegate = _currentClassOrNamespace->lookupType(q)) {
                    ClassOrNamespace *b = _currentClassOrNamespace->findOrCreateType(unqualifiedId);
                    b->addUsing(delegate);
                }
            }
        }
    }
    return false;
}

bool CreateBindings::visit(UsingNamespaceDirective *u)
{
    if (ClassOrNamespace *e = _currentClassOrNamespace->lookupType(u->name())) {
        _currentClassOrNamespace->addUsing(e);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for namespace:" << oo(u->name());
    }
    return false;
}

bool CreateBindings::visit(NamespaceAlias *a)
{
    if (! a->identifier()) {
        return false;

    } else if (ClassOrNamespace *e = _currentClassOrNamespace->lookupType(a->namespaceName())) {
        if (a->name()->isNameId() || a->name()->isTemplateNameId())
            _currentClassOrNamespace->addNestedType(a->name(), e);

    } else if (false) {
        Overview oo;
        qDebug() << "no entity for namespace:" << oo(a->namespaceName());
    }

    return false;
}

bool CreateBindings::visit(ObjCClass *klass)
{
    ClassOrNamespace *previous = enterGlobalClassOrNamespace(klass);

    process(klass->baseClass());

    for (unsigned i = 0; i < klass->protocolCount(); ++i)
        process(klass->protocolAt(i));

    for (unsigned i = 0; i < klass->memberCount(); ++i)
        process(klass->memberAt(i));

    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(ObjCBaseClass *b)
{
    if (ClassOrNamespace *base = _globalNamespace->lookupType(b->name())) {
        _currentClassOrNamespace->addUsing(base);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for:" << oo(b->name());
    }
    return false;
}

bool CreateBindings::visit(ObjCForwardClassDeclaration *klass)
{
    ClassOrNamespace *previous = enterGlobalClassOrNamespace(klass);
    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(ObjCProtocol *proto)
{
    ClassOrNamespace *previous = enterGlobalClassOrNamespace(proto);

    for (unsigned i = 0; i < proto->protocolCount(); ++i)
        process(proto->protocolAt(i));

    for (unsigned i = 0; i < proto->memberCount(); ++i)
        process(proto->memberAt(i));

    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(ObjCBaseProtocol *b)
{
    if (ClassOrNamespace *base = _globalNamespace->lookupType(b->name())) {
        _currentClassOrNamespace->addUsing(base);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for:" << oo(b->name());
    }
    return false;
}

bool CreateBindings::visit(ObjCForwardProtocolDeclaration *proto)
{
    ClassOrNamespace *previous = enterGlobalClassOrNamespace(proto);
    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(ObjCMethod *)
{
    return false;
}

