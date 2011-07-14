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

#ifndef QMLGDBENGINE_H
#define QMLGDBENGINE_H

#include "debuggerengine.h"

#include <QtCore/QScopedPointer>

namespace Debugger {
namespace Internal {

class QmlCppEnginePrivate;

class DEBUGGER_EXPORT QmlCppEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    explicit QmlCppEngine(const DebuggerStartParameters &sp,
                          DebuggerEngineType slaveEngineType,
                          QString *errorMessage);
    ~QmlCppEngine();

    bool setToolTipExpression(const QPoint &mousePos,
        TextEditor::ITextEditor * editor, const DebuggerToolTipContext &);
    void updateWatchData(const WatchData &data,
        const WatchUpdateFlags &flags);

    void watchPoint(const QPoint &);
    void fetchMemory(MemoryAgent *, QObject *, quint64 addr, quint64 length);
    void fetchDisassembler(DisassemblerAgent *);
    void activateFrame(int index);

    void reloadModules();
    void examineModules();
    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    void requestModuleSymbols(const QString &moduleName);

    void reloadRegisters();
    void reloadSourceFiles();
    void reloadFullStack();

    void setRegisterValue(int regnr, const QString &value);
    unsigned debuggerCapabilities() const;
    virtual bool canWatchWidgets() const;
    virtual bool acceptsWatchesWhileRunning() const;

    bool isSynchronous() const;
    QByteArray qtNamespace() const;

    void createSnapshot();
    void updateAll();

    void attemptBreakpointSynchronization();
    bool acceptsBreakpoint(BreakpointModelId id) const;
    void selectThread(int index);

    void assignValueInDebugger(const WatchData *data,
        const QString &expr, const QVariant &value);

    DebuggerEngine *cppEngine() const;
    void handleRemoteSetupDone(int gdbServerPort, int qmlPort);
    void handleRemoteSetupFailed(const QString &message);

    void showMessage(const QString &msg, int channel = LogDebug,
        int timeout = -1) const;

protected:
    void detachDebugger();
    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();
    void executeNextI();
    void executeReturn();
    void continueInferior();
    void interruptInferior();
    void requestInterruptInferior();

    void executeRunToLine(const ContextData &data);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const ContextData &data);
    void executeDebuggerCommand(const QString &command);

    void setupEngine();
    void setupInferior();
    void runEngine();
    void shutdownInferior();
    void shutdownEngine();

    void notifyInferiorRunOk();
    void notifyInferiorSpontaneousStop();
    void notifyEngineRunAndInferiorRunOk();
    void notifyInferiorShutdownOk();

protected slots:
    void skipCppBreakpoint();

private:
    void engineStateChanged(DebuggerState newState);
    void setState(DebuggerState newState, bool forced = false);
    void slaveEngineStateChanged(DebuggerEngine *slaveEngine, DebuggerState state);

    void readyToExecuteQmlStep();

private:
    QScopedPointer<QmlCppEnginePrivate> d;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLGDBENGINE_H
