/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_SCRIPTENGINE_H
#define DEBUGGER_SCRIPTENGINE_H

#include "debuggerengine.h"

#include <QtCore/QSharedPointer>
#include <QtCore/QScopedPointer>
#include <QtCore/QHash>

QT_BEGIN_NAMESPACE
class QScriptEngine;
class QScriptValue;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class ScriptAgent;
class WatchData;

/* A debugger engine for a QScriptEngine implemented using a QScriptEngineAgent.
 * listening on script events. The engine has a special execution model:
 * The script is executed in the foreground, while the debugger runs in
 * processEvents() triggered by QScriptEngine::setProcessEventsInterval().
 * Stopping is emulated by manually calling processEvents() from the debugger engine. */

class ScriptEngine : public Debugger::DebuggerEngine
{
    Q_OBJECT

public:
    explicit ScriptEngine(const DebuggerStartParameters &startParameters);
    virtual ~ScriptEngine();

private:
    // DebuggerEngine implementation
    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();
    void executeNextI();

    bool setToolTipExpression(const QPoint &mousePos,
        TextEditor::ITextEditor *editor, const DebuggerToolTipContext &);
    void setupEngine();
    void setupInferior();
    void runEngine();
    void shutdownInferior();
    void shutdownEngine();

    void continueInferior();
    void interruptInferior();

    void executeRunToLine(const ContextData &data);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const ContextData &data);

    void activateFrame(int index);
    void selectThread(int index);

    bool acceptsBreakpoint(BreakpointId id) const;
    void attemptBreakpointSynchronization();

    void assignValueInDebugger(const WatchData *w,
        const QString &expr, const QVariant &value);
    void executeDebuggerCommand(const QString &command);

    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    void requestModuleSymbols(const QString &moduleName);
    void reloadModules();
    void reloadRegisters() {}
    void reloadSourceFiles() {}
    void reloadFullStack() {}

    bool supportsThreads() const { return true; }
    bool checkForBreakCondition(bool byFunction);
    void updateWatchData(const WatchData &data, const WatchUpdateFlags &flags);
    void updateLocals();
    void updateSubItem(const WatchData &data);

private:
    friend class ScriptAgent;

    void importExtensions();

    QSharedPointer<QScriptEngine> m_scriptEngine;
    QString m_scriptContents;
    QString m_scriptFileName;
    QScopedPointer<ScriptAgent> m_scriptAgent;
    QHash<quint64,QScriptValue> m_watchIdToScriptValue;
    quint64 m_watchIdCounter;

    bool m_stopped;
    bool m_stopOnNextLine;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_SCRIPTENGINE_H
