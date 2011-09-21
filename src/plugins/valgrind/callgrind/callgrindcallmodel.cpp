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

#include "callgrindcallmodel.h"

#include "callgrindfunctioncall.h"
#include "callgrindfunction.h"
#include "callgrindparsedata.h"

#include <utils/qtcassert.h>

#include <QVector>

namespace Valgrind {
namespace Callgrind {

//BEGIN CallModel::Private

class CallModel::Private
{
public:
    Private();

    const ParseData *m_data;
    QVector<const FunctionCall *> m_calls;
    int m_event;
    const Function *m_function;
};

CallModel::Private::Private()
    : m_data(0)
    , m_event(0)
    , m_function(0)
{
}

//END CallModel::Private

//BEGIN CallModel
CallModel::CallModel(QObject *parent)
  : QAbstractItemModel(parent), d(new Private)
{
}

CallModel::~CallModel()
{
    delete d;
}

void CallModel::clear()
{
    beginResetModel();
    d->m_function = 0;
    d->m_calls.clear();
    endResetModel();
}

void CallModel::setCalls(const QVector<const FunctionCall *> &calls, const Function *function)
{
    beginResetModel();
    d->m_function = function;
    d->m_calls = calls;
    endResetModel();
}

QVector<const FunctionCall *> CallModel::calls() const
{
    return d->m_calls;
}

const Function *CallModel::function() const
{
    return d->m_function;
}

void CallModel::setCostEvent(int event)
{
    d->m_event = event;
}

int CallModel::costEvent() const
{
    return d->m_event;
}

void CallModel::setParseData(const ParseData *data)
{
    if (d->m_data == data)
        return;

    if (!data)
        clear();

    d->m_data = data;
}

const ParseData *CallModel::parseData() const
{
    return d->m_data;
}

int CallModel::rowCount(const QModelIndex &parent) const
{
    QTC_ASSERT(!parent.isValid() || parent.model() == this, return 0);

    if (parent.isValid())
        return 0;

    return d->m_calls.count();
}

int CallModel::columnCount(const QModelIndex &parent) const
{
    QTC_ASSERT(!parent.isValid() || parent.model() == this, return 0);

    if (parent.isValid())
        return 0;

    return ColumnCount;
}

QModelIndex CallModel::parent(const QModelIndex &child) const
{
    QTC_ASSERT(!child.isValid() || child.model() == this, return QModelIndex());

    return QModelIndex();
}

QModelIndex CallModel::index(int row, int column, const QModelIndex &parent) const
{
    QTC_ASSERT(!parent.isValid() || parent.model() == this, return QModelIndex());

    if (row == 0 && rowCount(parent) == 0) // happens with empty models
        return QModelIndex();

    QTC_ASSERT(row >= 0 && row < rowCount(parent), return QModelIndex());

    return createIndex(row, column);
}

QVariant CallModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.model() == this, return QVariant());
    QTC_ASSERT(index.column() >= 0 && index.column() < columnCount(index.parent()), return QVariant());
    QTC_ASSERT(index.row() >= 0 && index.row() < rowCount(index.parent()), return QVariant());

    const FunctionCall *call = d->m_calls.at(index.row());
    const quint64 callCost = call->cost(d->m_event);
    const quint64 parentCost = d->m_function->inclusiveCost(d->m_event);
    if (role == ParentCostRole) {
        return parentCost;
    }
    else if (role == FunctionCallRole
                || role == RelativeParentCostRole || role == RelativeTotalCostRole) {
        if (role == FunctionCallRole)
            return QVariant::fromValue(call);

        if (role == RelativeTotalCostRole) {
            const quint64 totalCost = d->m_data->totalCost(d->m_event);
            return (float) callCost / totalCost;
        }

        if (role == RelativeParentCostRole)
            return (float) callCost / parentCost;
    }
    else if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        if (index.column() == CalleeColumn)
            return call->callee()->name();
        else if (index.column() == CallerColumn)
            return call->caller()->name();
        else if (index.column() == CostColumn && role != Qt::ToolTipRole)
            return callCost;
        else if (index.column() == CallsColumn && role != Qt::ToolTipRole)
            return call->calls();
    }

    return QVariant();
}

QVariant CallModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || (role != Qt::DisplayRole && role != Qt::ToolTipRole))
        return QVariant();

    QTC_ASSERT(section >= 0 && section < columnCount(), return QVariant());

    if (role == Qt::ToolTipRole) {
        if (section == CostColumn) {
            if (!d->m_data)
                return QVariant();

            return ParseData::prettyStringForEvent(d->m_data->events().at(d->m_event));
        }
        return QVariant();
    }

    if (section == CalleeColumn)
        return tr("Callee");
    else if (section == CallerColumn)
        return tr("Caller");
    else if (section == CostColumn)
        return tr("Cost");
    else if (section == CallsColumn)
        return tr("Calls");

    return QVariant();
}

} // namespace Callgrind
} // namespace Valgrind
