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

#include "interactiveinterpreter.h"

namespace Debugger {
namespace Internal {

bool InteractiveInterpreter::canEvaluate()
{
    int yyaction = 0;
    int yytoken = -1;
    int yytos = -1;

    setCode(m_code, 1);
    m_tokens.append(T_FEED_JS_PROGRAM);

    do {
        if (++yytos == m_stateStack.size())
            m_stateStack.resize(m_stateStack.size() * 2);

        m_stateStack[yytos] = yyaction;

again:
        if (yytoken == -1 && action_index[yyaction] != -TERMINAL_COUNT) {
            if (m_tokens.isEmpty())
                yytoken = lex();
            else
                yytoken = m_tokens.takeFirst();
        }

        yyaction = t_action(yyaction, yytoken);
        if (yyaction > 0) {
            if (yyaction == ACCEPT_STATE) {
                --yytos;
                return true;
            }
            yytoken = -1;
        } else if (yyaction < 0) {
            const int ruleno = -yyaction - 1;
            yytos -= rhs[ruleno];
            yyaction = nt_action(m_stateStack[yytos], lhs[ruleno] - TERMINAL_COUNT);
        }
    } while (yyaction);

    const int errorState = m_stateStack[yytos];
    if (t_action(errorState, T_AUTOMATIC_SEMICOLON) && canInsertAutomaticSemicolon(yytoken)) {
        yyaction = errorState;
        m_tokens.prepend(yytoken);
        yytoken = T_SEMICOLON;
        goto again;
    }

    if (yytoken != EOF_SYMBOL)
        return true;

    return false;
}

}
}
