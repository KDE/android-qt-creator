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

#include "OverviewModel.h"
#include "Overview.h"

#include <Scope.h>
#include <Literals.h>
#include <Symbols.h>

#include <QtCore/QFile>
#include <QtCore/QtDebug>

using namespace CPlusPlus;

OverviewModel::OverviewModel(QObject *parent)
    : QAbstractItemModel(parent)
{ }

OverviewModel::~OverviewModel()
{ }

bool OverviewModel::hasDocument() const
{
    return _cppDocument;
}

Document::Ptr OverviewModel::document() const
{
    return _cppDocument;
}

unsigned OverviewModel::globalSymbolCount() const
{
    unsigned count = 0;
    if (_cppDocument)
        count += _cppDocument->globalSymbolCount();
    return count;
}

Symbol *OverviewModel::globalSymbolAt(unsigned index) const
{ return _cppDocument->globalSymbolAt(index); }

QModelIndex OverviewModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row == 0) // account for no symbol item
            return createIndex(row, column);
        Symbol *symbol = globalSymbolAt(row-1); // account for no symbol item
        return createIndex(row, column, symbol);
    } else {
        Symbol *parentSymbol = static_cast<Symbol *>(parent.internalPointer());
        Q_ASSERT(parentSymbol);

        Scope *scope = parentSymbol->asScope();
        Q_ASSERT(scope != 0);
        return createIndex(row, 0, scope->memberAt(row));
    }
}

QModelIndex OverviewModel::parent(const QModelIndex &child) const
{
    Symbol *symbol = static_cast<Symbol *>(child.internalPointer());
    if (!symbol) // account for no symbol item
        return QModelIndex();

    if (Scope *scope = symbol->enclosingScope()) {
        if (scope->enclosingScope()) {
            QModelIndex index;
            if (scope->enclosingScope() && scope->enclosingScope()->enclosingScope()) // the parent doesn't have a parent
                index = createIndex(scope->index(), 0, scope);
            else //+1 to account for no symbol item
                index = createIndex(scope->index() + 1, 0, scope);
            return index;
        }
    }

    return QModelIndex();
}

int OverviewModel::rowCount(const QModelIndex &parent) const
{
    if (hasDocument()) {
        if (!parent.isValid()) {
            return globalSymbolCount()+1; // account for no symbol item
        } else {
            if (!parent.parent().isValid() && parent.row() == 0) // account for no symbol item
                return 0;
            Symbol *parentSymbol = static_cast<Symbol *>(parent.internalPointer());
            Q_ASSERT(parentSymbol);

            if (Scope *parentScope = parentSymbol->asScope()) {
                if (!parentScope->isFunction() && !parentScope->isObjCMethod()) {
                    return parentScope->memberCount();
                }
            }
            return 0;
        }
    }
    if (!parent.isValid())
        return 1; // account for no symbol item
    return 0;
}

int OverviewModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant OverviewModel::data(const QModelIndex &index, int role) const
{
    // account for no symbol item
    if (!index.parent().isValid() && index.row() == 0) {
        switch (role) {
        case Qt::DisplayRole:
            if (rowCount() > 1)
                return tr("<Select Symbol>");
            else
                return tr("<No Symbols>");
        default:
            return QVariant();
        } //switch
    }

    switch (role) {
    case Qt::DisplayRole: {
        Symbol *symbol = static_cast<Symbol *>(index.internalPointer());
        QString name = _overview.prettyName(symbol->name());
        if (name.isEmpty())
            name = QLatin1String("anonymous");
        if (symbol->isObjCForwardClassDeclaration())
            name = QLatin1String("@class ") + name;
        if (symbol->isObjCForwardProtocolDeclaration() || symbol->isObjCProtocol())
            name = QLatin1String("@protocol ") + name;
        if (symbol->isObjCClass()) {
            ObjCClass *clazz = symbol->asObjCClass();
            if (clazz->isInterface())
                name = QLatin1String("@interface ") + name;
            else
                name = QLatin1String("@implementation ") + name;

            if (clazz->isCategory())
                name += QLatin1String(" (") + _overview.prettyName(clazz->categoryName()) + QLatin1Char(')');
        }
        if (symbol->isObjCPropertyDeclaration())
            name = QLatin1String("@property ") + name;
        if (symbol->isObjCMethod()) {
            ObjCMethod *method = symbol->asObjCMethod();
            if (method->isStatic())
                name = QLatin1Char('+') + name;
            else
                name = QLatin1Char('-') + name;
        } else if (! symbol->isScope() || symbol->isFunction()) {
            QString type = _overview.prettyType(symbol->type());
            if (! type.isEmpty()) {
                if (! symbol->type()->isFunctionType())
                    name += QLatin1String(": ");
                name += type;
            }
        }
        return name;
    }

    case Qt::EditRole: {
        Symbol *symbol = static_cast<Symbol *>(index.internalPointer());
        QString name = _overview.prettyName(symbol->name());
        if (name.isEmpty())
            name = QLatin1String("anonymous");
        return name;
    }

    case Qt::DecorationRole: {
        Symbol *symbol = static_cast<Symbol *>(index.internalPointer());
        return _icons.iconForSymbol(symbol);
    } break;

    case FileNameRole: {
        Symbol *symbol = static_cast<Symbol *>(index.internalPointer());
        return QString::fromUtf8(symbol->fileName(), symbol->fileNameLength());
    }

    case LineNumberRole: {
        Symbol *symbol = static_cast<Symbol *>(index.internalPointer());
        return symbol->line();
    }

    default:
        return QVariant();
    } // switch
}

Symbol *OverviewModel::symbolFromIndex(const QModelIndex &index) const
{
    return static_cast<Symbol *>(index.internalPointer());
}

void OverviewModel::rebuild(Document::Ptr doc)
{
    _cppDocument = doc;
    reset();
}
