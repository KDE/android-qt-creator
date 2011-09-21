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

#include "callgrindfunctioncycle.h"
#include "callgrindfunction_p.h"

#include "callgrindfunctioncall.h"
#include "callgrindparsedata.h"

#include <QtCore/QStringList>
#include <QtCore/QDebug>

namespace Valgrind {
namespace Callgrind {

//BEGIN FunctionCycle::Private

class FunctionCycle::Private : public Function::Private
{
public:
    Private(const ParseData *data);
    QVector<const Function *> m_functions;
};

FunctionCycle::Private::Private(const ParseData *data)
    : Function::Private(data)
{
}

#define CYCLE_D static_cast<FunctionCycle::Private *>(this->d)

//BEGIN FunctionCycle

FunctionCycle::FunctionCycle(const ParseData *data)
    : Function(new Private(data))
{
}

FunctionCycle::~FunctionCycle()
{
    // d should be deleted by Function::~Function()
}

void FunctionCycle::setFunctions(const QVector<const Function *> functions)
{
    Private *d = CYCLE_D;

    d->m_functions = functions;

    d->m_incomingCallMap.clear();
    d->m_outgoingCallMap.clear();
    d->m_called = 0;
    d->m_selfCost.fill(0, d->m_data->events().size());
    d->m_inclusiveCost.fill(0, d->m_data->events().size());

    foreach (const Function *func, functions) {
        // just add up self cost
        d->accumulateCost(d->m_selfCost, func->selfCosts());
        // add outgoing calls to functions that are not part of the cycle
        foreach (const FunctionCall *call, func->outgoingCalls()) {
            if (!functions.contains(call->callee()))
                d->accumulateCall(call, Function::Private::Outgoing);
        }
        // add incoming calls from functions that are not part of the cycle
        foreach (const FunctionCall *call, func->incomingCalls()) {
            if (!functions.contains(call->caller())) {
                d->accumulateCall(call, Function::Private::Incoming);
                d->m_called += call->calls();
                d->accumulateCost(d->m_inclusiveCost, call->costs());
            }
        }
    }
    // now subtract self from incl. cost (see implementation of inclusiveCost())
    // now subtract self cost (see @c inclusiveCost() implementation)
    for (int i = 0, c = d->m_inclusiveCost.size(); i < c; ++i) {
        if (d->m_inclusiveCost.at(i) < d->m_selfCost.at(i))
            d->m_inclusiveCost[i] = 0;
        else
            d->m_inclusiveCost[i] -= d->m_selfCost.at(i);
    }
}

QVector<const Function *> FunctionCycle::functions() const
{
    return CYCLE_D->m_functions;
}

} // namespace Callgrind
} // namespace Valgrind
