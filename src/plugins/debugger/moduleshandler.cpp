/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "moduleshandler.h"

#include <utils/elfreader.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QSortFilterProxyModel>

using namespace Utils;

//////////////////////////////////////////////////////////////////
//
// ModulesModel
//
//////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

class ModulesModel : public QAbstractItemModel
{
public:
    explicit ModulesModel(QObject *parent)
      : QAbstractItemModel(parent)
    {}

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
    void removeModule(const QString &modulePath);
    void setModules(const Modules &modules);
    void updateModule(const Module &module);

    int indexOfModule(const QString &modulePath) const;

    Modules m_modules;
};


QVariant ModulesModel::headerData(int section,
    Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static QString headers[] = {
            ModulesHandler::tr("Module name") + QLatin1String("        "),
            ModulesHandler::tr("Module path") + QLatin1String("        "),
            ModulesHandler::tr("Symbols read") + QLatin1String("        "),
            ModulesHandler::tr("Symbols type") + QLatin1String("        "),
            ModulesHandler::tr("Start address") + QLatin1String("        "),
            ModulesHandler::tr("End address") + QLatin1String("        ")
        };
        return headers[section];
    }
    return QVariant();
}

QVariant ModulesModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= m_modules.size())
        return QVariant();

    const Module &module = m_modules.at(row);

    switch (index.column()) {
        case 0:
            if (role == Qt::DisplayRole)
                return module.moduleName;
            // FIXME: add icons
            //if (role == Qt::DecorationRole)
            //    return module.symbolsRead ? icon2 : icon;
            break;
        case 1:
            if (role == Qt::DisplayRole)
                return module.modulePath;
            if (role == Qt::ToolTipRole) {
                QString msg;
                if (!module.elfData.buildId.isEmpty())
                    msg += QString::fromLatin1("Build Id: " + module.elfData.buildId);
                if (!module.elfData.debugLink.isEmpty())
                    msg += QString::fromLatin1("Debug Link: " + module.elfData.debugLink);
                return msg;
            }
            break;
        case 2:
            if (role == Qt::DisplayRole)
                switch (module.symbolsRead) {
                    case Module::UnknownReadState: return ModulesHandler::tr("unknown");
                    case Module::ReadFailed: return ModulesHandler::tr("no");
                    case Module::ReadOk: return ModulesHandler::tr("yes");
                }
            break;
        case 3:
            if (role == Qt::DisplayRole)
                switch (module.elfData.symbolsType) {
                    case UnknownSymbols:
                        return ModulesHandler::tr("unknown");
                    case NoSymbols:
                        return ModulesHandler::tr("none");
                    case PlainSymbols:
                        return ModulesHandler::tr("plain");
                    case FastSymbols:
                        return ModulesHandler::tr("fast");
                    case LinkedSymbols:
                        return ModulesHandler::tr("debuglnk");
                    case BuildIdSymbols:
                        return ModulesHandler::tr("buildid");
                }
            else if (role == Qt::ToolTipRole)
                switch (module.elfData.symbolsType) {
                    case UnknownSymbols:
                        return ModulesHandler::tr(
                        "It is unknown whether this module contains debug "
                        "information.\nUse \"Examine Symbols\" from the "
                        "context menu to initiate a check.");
                    case NoSymbols:
                        return ModulesHandler::tr(
                        "This module neither contains nor references debug "
                        "information.\nStepping into the module or setting "
                        "breakpoints by file and line will not work.");
                    case PlainSymbols:
                        return ModulesHandler::tr(
                        "This module contains debug information.\nStepping "
                        "into the module or setting breakpoints by file and "
                        "is expected to work.");
                    case FastSymbols:
                        return ModulesHandler::tr(
                        "This module contains debug information.\nStepping "
                        "into the module or setting breakpoints by file and "
                        "is expected to work.");
                    case LinkedSymbols:
                    case BuildIdSymbols:
                        return ModulesHandler::tr(
                        "This module does not contains debug information "
                        "itself, but contains a reference to external "
                        "debug information.");
                }
            break;
        case 4:
            if (role == Qt::DisplayRole)
                if (module.startAddress)
                    return QString(QLatin1String("0x")
                                + QString::number(module.startAddress, 16));
            break;
        case 5:
            if (role == Qt::DisplayRole) {
                if (module.endAddress)
                    return QString(QLatin1String("0x")
                                + QString::number(module.endAddress, 16));
                //: End address of loaded module
                return ModulesHandler::tr("<unknown>", "address");
            }
            break;
    }
    return QVariant();
}

void ModulesModel::setModules(const Modules &m)
{
    m_modules = m;
    reset();
}

void ModulesModel::clearModel()
{
    if (!m_modules.isEmpty()) {
        m_modules.clear();
        reset();
    }
}

int ModulesModel::indexOfModule(const QString &modulePath) const
{
    // Recent modules are more likely to be unloaded first.
    for (int i = m_modules.size() - 1; i >= 0; i--)
        if (m_modules.at(i).modulePath == modulePath)
            return i;
    return -1;
}

void ModulesModel::removeModule(const QString &modulePath)
{
    const int row = indexOfModule(modulePath);
    QTC_ASSERT(row != -1, return);
    beginRemoveRows(QModelIndex(), row, row);
    m_modules.remove(row);
    endRemoveRows();
}

void ModulesModel::updateModule(const Module &module)
{
    const int row = indexOfModule(module.modulePath);
    ElfReader reader(module.modulePath);
    ElfData elfData = reader.readHeaders();

    if (row == -1) {
        const int n = m_modules.size();
        beginInsertRows(QModelIndex(), n, n);
        m_modules.push_back(module);
        m_modules.back().elfData = elfData;
        endInsertRows();
    } else {
        m_modules[row] = module;
        m_modules[row].elfData = elfData;
        dataChanged(index(row, 0, QModelIndex()), index(row, 4, QModelIndex()));
    }
}

//////////////////////////////////////////////////////////////////
//
// ModulesHandler
//
//////////////////////////////////////////////////////////////////

ModulesHandler::ModulesHandler()
{
    m_model = new ModulesModel(this);
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
}

QAbstractItemModel *ModulesHandler::model() const
{
    return m_proxyModel;
}

void ModulesHandler::removeAll()
{
    m_model->clearModel();
}

void ModulesHandler::removeModule(const QString &modulePath)
{
    m_model->removeModule(modulePath);
}

void ModulesHandler::updateModule(const Module &module)
{
    m_model->updateModule(module);
}

void ModulesHandler::setModules(const Modules &modules)
{
    m_model->setModules(modules);
}

Modules ModulesHandler::modules() const
{
    return m_model->m_modules;
}

} // namespace Internal
} // namespace Debugger
