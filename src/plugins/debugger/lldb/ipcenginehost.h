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

#ifndef DEBUGGER_IPCENGINE_HOST_H
#define DEBUGGER_IPCENGINE_HOST_H

#include "debuggerengine.h"
#include "threadshandler.h"
#include "stackhandler.h"
#include "breakhandler.h"
#include "sourceagent.h"

#include <QtCore/QQueue>
#include <QtCore/QVariant>
#include <QtCore/QThread>

namespace Debugger {
namespace Internal {

class IPCEngineGuest;
class IPCEngineHost : public DebuggerEngine
{
    Q_OBJECT

public:
    explicit IPCEngineHost(const DebuggerStartParameters &startParameters);
    ~IPCEngineHost();

    // use either one
    void setLocalGuest(IPCEngineGuest *);
    void setGuestDevice(QIODevice *);

    enum Function
    {
        SetupIPC               = 1,
        StateChanged           = 2,
        SetupEngine            = 3,
        SetupInferior          = 4,
        RunEngine              = 5,
        ShutdownInferior       = 6,
        ShutdownEngine         = 7,
        DetachDebugger         = 8,
        ExecuteStep            = 9,
        ExecuteStepOut         = 10,
        ExecuteNext            = 11,
        ExecuteStepI           = 12,
        ExecuteNextI           = 13,
        ContinueInferior       = 14,
        InterruptInferior      = 15,
        ExecuteRunToLine       = 16,
        ExecuteRunToFunction   = 17,
        ExecuteJumpToLine      = 18,
        ActivateFrame          = 19,
        SelectThread           = 20,
        Disassemble            = 21,
        AddBreakpoint          = 22,
        RemoveBreakpoint       = 23,
        ChangeBreakpoint       = 24,
        RequestUpdateWatchData = 25,
        FetchFrameSource       = 26
    };
    Q_ENUMS(Function)

    void setupEngine();
    void setupInferior();
    void runEngine();
    void shutdownInferior();
    void shutdownEngine();
    void detachDebugger();
    void executeStep();
    void executeStepOut() ;
    void executeNext();
    void executeStepI();
    void executeNextI();
    void continueInferior();
    void interruptInferior();
    void executeRunToLine(const QString &fileName, int lineNumber);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const QString &fileName, int lineNumber);
    void activateFrame(int index);
    void selectThread(int index);
    void fetchDisassembler(DisassemblerAgent *);
    bool acceptsBreakpoint(BreakpointId) const { return true; } // FIXME
    void insertBreakpoint(BreakpointId id);
    void removeBreakpoint(BreakpointId id);
    void changeBreakpoint(BreakpointId id);
    void updateWatchData(const WatchData &data,
            const WatchUpdateFlags &flags = WatchUpdateFlags());
    void fetchFrameSource(qint64 id);

    void rpcCall(Function f, QByteArray payload = QByteArray());
protected:
    virtual void nuke() = 0;
public slots:
    void rpcCallback(quint64 f, QByteArray payload = QByteArray());
private slots:
    void m_stateChanged(const Debugger::DebuggerState &state);
    void readyRead();
private:
    IPCEngineGuest *m_localGuest;
    quint64 m_nextMessageCookie;
    quint64 m_nextMessageFunction;
    quint64 m_nextMessagePayloadSize;
    quint64 m_cookie;
    QIODevice *m_device;
    QHash<quint64, DisassemblerAgent *> m_frameToDisassemblerAgent;
    QHash<QString, SourceAgent *> m_sourceAgents;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_LLDBENGINE_H
