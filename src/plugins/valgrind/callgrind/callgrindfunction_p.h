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

#ifndef LIBVALGRIND_CALLGRINDFUNCTION_P_H
#define LIBVALGRIND_CALLGRINDFUNCTION_P_H

#include "callgrindfunction.h"
#include "callgrindparsedata.h"
#include "callgrindcostitem.h"
#include "callgrindfunctioncall.h"

#include <QtCore/QVector>
#include <QtCore/QHash>

namespace Valgrind {
namespace Callgrind {

class Function::Private
{
public:
    Private(const ParseData *data);
    ~Private();

    static void accumulateCost(QVector<quint64> &base, const QVector<quint64> &add);
    enum CallType {
        Incoming,
        Outgoing
    };
    ///@return accumulated call
    FunctionCall *accumulateCall(const FunctionCall *call, CallType type);

    const ParseData *m_data;
    qint64 m_fileId;
    qint64 m_objectId;
    qint64 m_nameId;

    QVector<quint64> m_selfCost;
    QVector<quint64> m_inclusiveCost;

    QVector<const CostItem *> m_costItems;
    // used to accumulate, hence values not const
    QHash<const Function *, FunctionCall *> m_outgoingCallMap;
    QHash<const Function *, FunctionCall *> m_incomingCallMap;
    // used in public api, hence const
    QVector<const FunctionCall *> m_outgoingCalls;
    QVector<const FunctionCall *> m_incomingCalls;
    quint64 m_called;
};

} // namespace Callgrind
} // namespace Valgrind

#endif // LIBVALGRIND_CALLGRINDFUNCTION_P_H
