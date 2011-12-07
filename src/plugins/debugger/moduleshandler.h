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

#ifndef DEBUGGER_MODULESHANDLER_H
#define DEBUGGER_MODULESHANDLER_H

#include <QtCore/QAbstractItemModel>
#include <QtCore/QObject>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class ModulesModel;
class ModulesHandler;


//////////////////////////////////////////////////////////////////
//
// Symbol
//
//////////////////////////////////////////////////////////////////

class Symbol
{
public:
    QString address;
    QString state;
    QString name;
    QString section;
    QString demangled;
};

typedef QVector<Symbol> Symbols;

//////////////////////////////////////////////////////////////////
//
// Module
//
//////////////////////////////////////////////////////////////////

class Module
{
public:
    Module() : symbolsRead(UnknownReadState), symbolsType(UnknownType) {}

public:
    enum SymbolReadState {
        UnknownReadState,  // Not tried.
        ReadFailed,        // Tried to read, but failed.
        ReadOk            // Dwarf index available.
    };
    enum SymbolType {
        UnknownType,       // Unknown.
        PlainSymbols,      // Ordinary symbols available.
        FastSymbols       // Dwarf index available.
    };
    QString moduleName;
    QString modulePath;
    SymbolReadState symbolsRead;
    SymbolType symbolsType;
    quint64 startAddress;
    quint64 endAddress;
};

typedef QVector<Module> Modules;


//////////////////////////////////////////////////////////////////
//
// ModulesModel
//
//////////////////////////////////////////////////////////////////

class ModulesModel : public QAbstractItemModel
{
    // Needs tr - context.
    Q_OBJECT
public:
    ModulesModel(ModulesHandler *parent);

    // QAbstractItemModel
    int columnCount(const QModelIndex &parent) const
        { return parent.isValid() ? 0 : 5; }
    int rowCount(const QModelIndex &parent) const
        { return parent.isValid() ? 0 : m_modules.size(); }
    QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }
    QModelIndex index(int row, int column, const QModelIndex &) const
        { return createIndex(row, column); }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    void clearModel();
    void addModule(const Module &module);
    void removeModule(const QString &moduleName);
    void setModules(const Modules &modules);
    void updateModule(const QString &moduleName, const Module &module);

    const Modules &modules() const { return m_modules; }

private:
    int indexOfModule(const QString &name) const;

    Modules m_modules;
};


//////////////////////////////////////////////////////////////////
//
// ModulesHandler
//
//////////////////////////////////////////////////////////////////

class ModulesHandler : public QObject
{
    Q_OBJECT

public:
    ModulesHandler();

    QAbstractItemModel *model() const;

    void setModules(const Modules &modules);
    void addModule(const Module &module);
    void removeModule(const QString &moduleName);
    void updateModule(const QString &moduleName, const Module &module);

    Modules modules() const;
    void removeAll();

private:
    ModulesModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_MODULESHANDLER_H
