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

#ifndef VALGRIND_CALLGRIND_CALLGRINDCALLMODEL_H
#define VALGRIND_CALLGRIND_CALLGRINDCALLMODEL_H

#include <QtCore/QAbstractItemModel>

#include "callgrindabstractmodel.h"

namespace Valgrind {
namespace Callgrind {

class FunctionCall;
class Function;

/**
 * Model to display list of function calls.
 */
class CallModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit CallModel(QObject *parent);
    virtual ~CallModel();

    void clear();

    /// Only one cost event column will be shown, this decides which one it is.
    /// By default it is the first event in the @c ParseData, i.e. 0.
    virtual int costEvent() const;
    virtual void setCostEvent(int event);

    virtual void setParseData(const ParseData *data);
    virtual const ParseData *parseData() const;

    void setCalls(const QVector<const FunctionCall *> &calls, const Function *function);
    QVector<const FunctionCall *> calls() const;
    const Function *function() const;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    enum Columns {
        CallerColumn,
        CalleeColumn,
        CallsColumn,
        CostColumn,
        ColumnCount
    };

    enum Roles {
        FunctionCallRole = NextCustomRole
    };

private:
    class Private;
    Private *d;
};

} // namespace Callgrind
} // namespace Valgrind

#endif // VALGRIND_CALLGRIND_CALLGRINDCALLMODEL_H
