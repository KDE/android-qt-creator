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

#include "searchsymbols.h"

#include <Literals.h>
#include <Scope.h>
#include <Names.h>
#include <cplusplus/LookupContext.h>
#include <QDebug>

using namespace CPlusPlus;
using namespace CppTools::Internal;

SearchSymbols::SymbolTypes SearchSymbols::AllTypes =
        SearchSymbols::Classes
        | SearchSymbols::Functions
        | SearchSymbols::Enums
        | SearchSymbols::Declarations;

SearchSymbols::SearchSymbols():
    symbolsToSearchFor(Classes | Functions | Enums),
    separateScope(false)
{
}

void SearchSymbols::setSymbolsToSearchFor(SymbolTypes types)
{
    symbolsToSearchFor = types;
}

void SearchSymbols::setSeparateScope(bool separateScope)
{
    this->separateScope = separateScope;
}

QList<ModelItemInfo> SearchSymbols::operator()(Document::Ptr doc, const QString &scope)
{
    QString previousScope = switchScope(scope);
    items.clear();
    for (unsigned i = 0; i < doc->globalSymbolCount(); ++i) {
        accept(doc->globalSymbolAt(i));
    }
    (void) switchScope(previousScope);
    QList<ModelItemInfo> result = items;
    strings.clear();
    items.clear();
    m_paths.clear();
    return result;
}

QString SearchSymbols::switchScope(const QString &scope)
{
    QString previousScope = _scope;
    _scope = scope;
    return previousScope;
}

bool SearchSymbols::visit(Enum *symbol)
{
    if (!(symbolsToSearchFor & Enums))
        return false;

    QString name = symbolName(symbol);
    QString scopedName = scopedSymbolName(name);
    QString previousScope = switchScope(scopedName);
    appendItem(separateScope ? name : scopedName,
               separateScope ? previousScope : QString(),
               ModelItemInfo::Enum, symbol);
    for (unsigned i = 0; i < symbol->memberCount(); ++i) {
        accept(symbol->memberAt(i));
    }
    (void) switchScope(previousScope);
    return false;
}

bool SearchSymbols::visit(Function *symbol)
{
    if (!(symbolsToSearchFor & Functions))
        return false;

    QString extraScope;
    if (const Name *name = symbol->name()) {
        if (const QualifiedNameId *q = name->asQualifiedNameId()) {
            if (q->base())
                extraScope = overview.prettyName(q->base());
        }
    }
    QString fullScope = _scope;
    if (!_scope.isEmpty() && !extraScope.isEmpty())
        fullScope += QLatin1String("::");
    fullScope += extraScope;
    QString name = symbolName(symbol);
    QString scopedName = scopedSymbolName(name);
    QString type = overview.prettyType(symbol->type(),
                                       separateScope ? symbol->unqualifiedName() : 0);
    appendItem(separateScope ? type : scopedName,
               separateScope ? fullScope : type,
               ModelItemInfo::Method, symbol);
    return false;
}

bool SearchSymbols::visit(Namespace *symbol)
{
    QString name = scopedSymbolName(symbol);
    QString previousScope = switchScope(name);
    for (unsigned i = 0; i < symbol->memberCount(); ++i) {
        accept(symbol->memberAt(i));
    }
    (void) switchScope(previousScope);
    return false;
}

bool SearchSymbols::visit(Declaration *symbol)
{
    if (!(symbolsToSearchFor & Declarations))
        return false;

    QString name = symbolName(symbol);
    QString scopedName = scopedSymbolName(name);
    QString type = overview.prettyType(symbol->type(),
                                       separateScope ? symbol->unqualifiedName() : 0);
    appendItem(separateScope ? type : scopedName,
               separateScope ? _scope : type,
               ModelItemInfo::Declaration, symbol);
    return false;
}

bool SearchSymbols::visit(Class *symbol)
{
    QString name = symbolName(symbol);
    QString scopedName = scopedSymbolName(name);
    QString previousScope = switchScope(scopedName);
    if (symbolsToSearchFor & Classes) {
        appendItem(separateScope ? name : scopedName,
                   separateScope ? previousScope : QString(),
                   ModelItemInfo::Class, symbol);
    }
    for (unsigned i = 0; i < symbol->memberCount(); ++i) {
        accept(symbol->memberAt(i));
    }
    (void) switchScope(previousScope);
    return false;
}

bool SearchSymbols::visit(CPlusPlus::UsingNamespaceDirective *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::UsingDeclaration *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::NamespaceAlias *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::Argument *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::TypenameArgument *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::BaseClass *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::Template *)
{
    return true;
}

bool SearchSymbols::visit(CPlusPlus::Block *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ForwardClassDeclaration *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCBaseClass *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCBaseProtocol *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCClass *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCForwardClassDeclaration *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCProtocol *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCForwardProtocolDeclaration *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCMethod *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCPropertyDeclaration *)
{
    return false;
}

QString SearchSymbols::scopedSymbolName(const QString &symbolName) const
{
    QString name = _scope;
    if (!name.isEmpty())
        name += QLatin1String("::");
    name += symbolName;
    return name;
}

QString SearchSymbols::scopedSymbolName(const Symbol *symbol) const
{
    return scopedSymbolName(symbolName(symbol));
}

QString SearchSymbols::symbolName(const Symbol *symbol) const
{
    QString symbolName = overview.prettyName(symbol->name());
    if (symbolName.isEmpty()) {
        QString type;
        if (symbol->isNamespace()) {
            type = QLatin1String("namespace");
        } else if (symbol->isEnum()) {
            type = QLatin1String("enum");
        } else if (const Class *c = symbol->asClass())  {
            if (c->isUnion()) {
                type = QLatin1String("union");
            } else if (c->isStruct()) {
                type = QLatin1String("struct");
            } else {
                type = QLatin1String("class");
            }
        } else {
            type = QLatin1String("symbol");
        }
        symbolName = QLatin1String("<anonymous ");
        symbolName += type;
        symbolName += QLatin1Char('>');
    }
    return symbolName;
}

void SearchSymbols::appendItem(const QString &name,
                               const QString &info,
                               ModelItemInfo::ItemType type,
                               Symbol *symbol)
{
    if (!symbol->name())
        return;

    QStringList fullyQualifiedName;
    foreach (const Name *name, LookupContext::fullyQualifiedName(symbol))
        fullyQualifiedName.append(findOrInsert(overview.prettyName(name)));

    QString path = m_paths.value(symbol->fileId(), QString());
    if (path.isEmpty()) {
        path = QString::fromUtf8(symbol->fileName(), symbol->fileNameLength());
        m_paths.insert(symbol->fileId(), path);
    }

    const QIcon icon = icons.iconForSymbol(symbol);
    items.append(ModelItemInfo(findOrInsert(name), findOrInsert(info), type,
                               fullyQualifiedName,
                               path,
                               symbol->line(),
                               symbol->column() - 1, // 1-based vs 0-based column
                               icon));
}
