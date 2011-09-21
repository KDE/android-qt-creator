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

#ifndef DEBUGGER_DEBUGGERENGINE_H
#define DEBUGGER_DEBUGGERENGINE_H

#include "debugger_global.h"
#include "debuggerconstants.h"
#include "breakpoint.h" // For 'BreakpointId'

#include <QtCore/QObject>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QDebug;
class QPoint;
class QMessageBox;
class QAbstractItemModel;
QT_END_NAMESPACE

namespace TextEditor {
class ITextEditor;
}

namespace Core {
class IOptionsPage;
}

namespace Debugger {

class DebuggerEnginePrivate;
class DebuggerRunControl;
class DebuggerStartParameters;

DEBUGGER_EXPORT QDebug operator<<(QDebug str, const DebuggerStartParameters &);
DEBUGGER_EXPORT QDebug operator<<(QDebug str, DebuggerState state);

namespace Internal {

class DebuggerPluginPrivate;
class DisassemblerAgent;
class MemoryAgent;
class WatchData;
class BreakHandler;
class ModulesHandler;
class RegisterHandler;
class StackHandler;
class StackFrame;
class SourceFilesHandler;
class ThreadsHandler;
class WatchHandler;
class BreakpointParameters;
class QmlCppEngine;
class DebuggerToolTipContext;
class MemoryMarkup;

struct WatchUpdateFlags
{
    WatchUpdateFlags() : tryIncremental(false) {}
    bool tryIncremental;
};

class Location
{
public:
    Location() { init(); }
    Location(quint64 address) { init(); m_address = address; }
    Location(const QString &file) { init(); m_fileName = file; }
    Location(const QString &file, int line, bool marker = true)
        { init(); m_lineNumber = line; m_fileName = file; m_needsMarker = marker; }
    Location(const StackFrame &frame, bool marker = true);
    QString fileName() const { return m_fileName; }
    QString functionName() const { return m_functionName; }
    QString from() const { return m_from; }
    int lineNumber() const { return m_lineNumber; }
    void setNeedsRaise(bool on) { m_needsRaise = on; }
    void setNeedsMarker(bool on) { m_needsMarker = on; }
    void setFileName(const QString &fileName) { m_fileName = fileName; }
    bool needsRaise() const { return m_needsRaise; }
    bool needsMarker() const { return m_needsMarker; }
    bool hasDebugInfo() const { return m_hasDebugInfo; }
    quint64 address() const { return m_address; }

private:
    void init() { m_needsMarker = false; m_needsRaise = true; m_lineNumber = -1;
        m_address = 0; m_hasDebugInfo = true; }
    bool m_needsMarker;
    bool m_needsRaise;
    bool m_hasDebugInfo;
    int m_lineNumber;
    QString m_fileName;
    QString m_functionName;
    QString m_from;
    quint64 m_address;
};

class ContextData
{
public:
    ContextData() : lineNumber(0), address(0) {}

public:
    QString fileName;
    int lineNumber;
    quint64 address;
};

} // namespace Internal


// FIXME: DEBUGGER_EXPORT?
class DEBUGGER_EXPORT DebuggerEngine : public QObject
{
    Q_OBJECT

public:
    explicit DebuggerEngine(const DebuggerStartParameters &sp,
        DebuggerEngine *parentEngine = 0);
    virtual ~DebuggerEngine();

    const DebuggerStartParameters &startParameters() const;
    DebuggerStartParameters &startParameters();

    virtual bool setToolTipExpression(const QPoint & mousePos,
        TextEditor::ITextEditor *editor, const Internal::DebuggerToolTipContext &);

    virtual void updateWatchData(const Internal::WatchData &data,
        const Internal::WatchUpdateFlags & flags = Internal::WatchUpdateFlags());
    virtual void startDebugger(DebuggerRunControl *runControl);

    virtual void watchPoint(const QPoint &);

    enum MemoryViewFlags
    {
        MemoryReadOnly = 0x1,      //!< Read-only.
        MemoryTrackRegister = 0x2, //!< Address parameter is register number to track
        MemoryView = 0x4           //!< Open a separate view (using the pos-parameter).
    };

    virtual void openMemoryView(quint64 startAddr, unsigned flags,
                                const QList<Internal::MemoryMarkup> &ml,
                                const QPoint &pos,
                                const QString &title = QString(), QWidget *parent = 0);
    virtual void fetchMemory(Internal::MemoryAgent *, QObject *,
                             quint64 addr, quint64 length);
    virtual void changeMemory(Internal::MemoryAgent *, QObject *,
                              quint64 addr, const QByteArray &data);
    virtual void updateMemoryViews();
    virtual void openDisassemblerView(const Internal::Location &location);
    virtual void fetchDisassembler(Internal::DisassemblerAgent *);
    virtual void activateFrame(int index);

    virtual void reloadModules();
    virtual void examineModules();
    virtual void loadSymbols(const QString &moduleName);
    virtual void loadSymbolsForStack();
    virtual void loadAllSymbols();
    virtual void requestModuleSymbols(const QString &moduleName);

    virtual void reloadRegisters();
    virtual void reloadSourceFiles();
    virtual void reloadFullStack();

    virtual void setRegisterValue(int regnr, const QString &value);
    virtual void addOptionPages(QList<Core::IOptionsPage*> *) const;
    virtual unsigned debuggerCapabilities() const;

    virtual bool isSynchronous() const;
    virtual QByteArray qtNamespace() const;

    virtual void createSnapshot();
    virtual void updateAll();

    typedef Internal::BreakpointModelId BreakpointModelId;
    virtual bool stateAcceptsBreakpointChanges() const { return true; }
    virtual void attemptBreakpointSynchronization();
    virtual bool acceptsBreakpoint(BreakpointModelId id) const = 0;
    virtual void insertBreakpoint(BreakpointModelId id);  // FIXME: make pure
    virtual void removeBreakpoint(BreakpointModelId id);  // FIXME: make pure
    virtual void changeBreakpoint(BreakpointModelId id);  // FIXME: make pure

    virtual bool acceptsDebuggerCommands() const { return true; }
    virtual void assignValueInDebugger(const Internal::WatchData *data,
        const QString &expr, const QVariant &value);
    virtual void selectThread(int index);

    virtual void handleRemoteSetupDone(int gdbServerPort, int qmlPort);
    virtual void handleRemoteSetupFailed(const QString &message);

    virtual Internal::ModulesHandler *modulesHandler() const;
    virtual Internal::RegisterHandler *registerHandler() const;
    virtual Internal::StackHandler *stackHandler() const;
    virtual Internal::ThreadsHandler *threadsHandler() const;
    virtual Internal::WatchHandler *watchHandler() const;
    virtual Internal::SourceFilesHandler *sourceFilesHandler() const;
    virtual Internal::BreakHandler *breakHandler() const;

    virtual QAbstractItemModel *modulesModel() const;
    virtual QAbstractItemModel *registerModel() const;
    virtual QAbstractItemModel *stackModel() const;
    virtual QAbstractItemModel *threadsModel() const;
    virtual QAbstractItemModel *localsModel() const;
    virtual QAbstractItemModel *watchersModel() const;
    virtual QAbstractItemModel *returnModel() const;
    virtual QAbstractItemModel *toolTipsModel() const;
    virtual QAbstractItemModel *sourceFilesModel() const;

    void progressPing();
    void handleFinished();
    void handleStartFailed();
    bool debuggerActionsEnabled() const;
    static bool debuggerActionsEnabled(DebuggerState state);

    DebuggerState state() const;
    DebuggerState lastGoodState() const;
    DebuggerState targetState() const;
    bool isDying() const;

    // Dumper stuff (common to cdb and gdb).
    bool qtDumperLibraryEnabled() const;
    QString qtDumperLibraryName() const;
    QStringList qtDumperLibraryLocations() const;
    void showQtDumperLibraryWarning(const QString &details);

    static const char *stateName(int s);

    void notifyInferiorPid(qint64 pid);
    qint64 inferiorPid() const;
    bool isReverseDebugging() const;
    void handleCommand(int role, const QVariant &value);

    // Convenience
    Q_SLOT virtual void showMessage(const QString &msg, int channel = LogDebug,
        int timeout = -1) const;
    Q_SLOT void showStatusMessage(const QString &msg, int timeout = -1) const;

    void resetLocation();
    virtual void gotoLocation(const Internal::Location &location);
    virtual void quitDebugger(); // called by DebuggerRunControl

    virtual void updateViews();
    bool isSlaveEngine() const;
    bool isMasterEngine() const;
    DebuggerEngine *masterEngine() const;

    virtual bool setupQmlStep(bool /*on*/) { return false; }
    virtual void readyToExecuteQmlStep() {}

    virtual bool canDisplayTooltip() const { return state() == InferiorStopOk; }

    virtual void notifyInferiorIll();

signals:
    void stateChanged(const Debugger::DebuggerState &state);
    // A new stack frame is on display including locals.
    void stackFrameCompleted();
    void updateViewsRequested();
    /*
     * For "external" clients of a debugger run control that needs to do
     * further setup before the debugger is started (e.g. Maemo).
     * Afterwards, handleSetupDone() or handleSetupFailed() must be called
     * to continue or abort debugging, respectively.
     * This signal is only emitted if the start parameters indicate that
     * a server start script should be used, but none is given.
     */
    void requestRemoteSetup();

protected:
    // The base notify*() function implementation should be sufficient
    // in most cases, but engines are free to override them to do some
    // engine specific cleanup like stopping timers etc.
    virtual void notifyEngineSetupOk();
    virtual void notifyEngineSetupFailed();
    virtual void notifyEngineRunFailed();

    virtual void notifyInferiorSetupOk();
    virtual void notifyInferiorSetupFailed();

    virtual void notifyEngineRunAndInferiorRunOk();
    virtual void notifyEngineRunAndInferiorStopOk();
    virtual void notifyInferiorUnrunnable(); // Called by CoreAdapter.

    // Use notifyInferiorRunRequested() plus notifyInferiorRunOk() instead.
    //virtual void notifyInferiorSpontaneousRun();

    virtual void notifyInferiorRunRequested();
    virtual void notifyInferiorRunOk();
    virtual void notifyInferiorRunFailed();

    virtual void notifyInferiorStopOk();
    virtual void notifyInferiorSpontaneousStop();
    virtual void notifyInferiorStopFailed();
    virtual void notifyInferiorExited();

    virtual void notifyInferiorShutdownOk();
    virtual void notifyInferiorShutdownFailed();

    virtual void notifyEngineSpontaneousShutdown();
    virtual void notifyEngineShutdownOk();
    virtual void notifyEngineShutdownFailed();

    virtual void notifyEngineIll();

    virtual void setupEngine() = 0;
    virtual void setupInferior() = 0;
    virtual void runEngine() = 0;
    virtual void shutdownInferior() = 0;
    virtual void shutdownEngine() = 0;

    virtual void detachDebugger();
    virtual void exitDebugger();
    virtual void executeStep();
    virtual void executeStepOut() ;
    virtual void executeNext();
    virtual void executeStepI();
    virtual void executeNextI();
    virtual void executeReturn();

    virtual void continueInferior();
    virtual void interruptInferior();
    virtual void requestInterruptInferior();

    virtual void executeRunToLine(const Internal::ContextData &data);
    virtual void executeRunToFunction(const QString &functionName);
    virtual void executeJumpToLine(const Internal::ContextData &data);
    virtual void executeDebuggerCommand(const QString &command);

    virtual void frameUp();
    virtual void frameDown();

    DebuggerRunControl *runControl() const;

    static QString msgWatchpointByAddressTriggered(BreakpointModelId id,
        int number, quint64 address);
    static QString msgWatchpointByAddressTriggered(BreakpointModelId id,
        int number, quint64 address, const QString &threadId);
    static QString msgWatchpointByExpressionTriggered(BreakpointModelId id,
        int number, const QString &expr);
    static QString msgWatchpointByExpressionTriggered(BreakpointModelId id,
        int number, const QString &expr, const QString &threadId);
    static QString msgBreakpointTriggered(BreakpointModelId id,
        int number, const QString &threadId);
    static QString msgStopped(const QString &reason = QString());
    static QString msgStoppedBySignal(const QString &meaning, const QString &name);
    static QString msgStoppedByException(const QString &description,
        const QString &threadId);
    static QString msgInterrupted();
    void showStoppedBySignalMessageBox(const QString meaning, QString name);
    void showStoppedByExceptionMessageBox(const QString &description);

    static bool isCppBreakpoint(const Internal::BreakpointParameters &p);

    bool isStateDebugging() const;
    void setStateDebugging(bool on);

    virtual void setupSlaveInferior();
    virtual void setupSlaveEngine();
    virtual void runSlaveEngine();
    virtual void shutdownSlaveEngine();

    virtual void slaveEngineStateChanged(DebuggerEngine *engine,
        DebuggerState state);

private:
    // Wrapper engine needs access to state of its subengines.
    friend class Internal::QmlCppEngine;
    friend class Internal::DebuggerPluginPrivate;
    friend class QmlAdapter;

    virtual void setState(DebuggerState state, bool forced = false);

    friend class DebuggerEnginePrivate;
    DebuggerEnginePrivate *d;
};

} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::ContextData)

#endif // DEBUGGER_DEBUGGERENGINE_H
