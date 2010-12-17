/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef COREENGINE_H
#define COREENGINE_H

#include "cdbcom.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QSharedPointer>

namespace CdbCore {

class DebugOutputBase;
class DebugEventCallbackBase;

// helper struct to pass interfaces around
struct ComInterfaces
{
    ComInterfaces();

    CIDebugClient*          debugClient;
    CIDebugControl*         debugControl;
    CIDebugSystemObjects*   debugSystemObjects;
    CIDebugSymbols*         debugSymbols;
    CIDebugRegisters*       debugRegisters;
    CIDebugDataSpaces*      debugDataSpaces;
    CIDebugAdvanced*        debugAdvanced;
};

class CoreEngine : public QObject
{
    Q_DISABLE_COPY(CoreEngine)
    Q_OBJECT
public:
    enum ExpressionSyntax { AssemblerExpressionSyntax, CppExpressionSyntax };
    enum CodeLevel { CodeLevelAssembly, CodeLevelSource };

   typedef QSharedPointer<DebugOutputBase> DebugOutputBasePtr;
   typedef QSharedPointer<DebugEventCallbackBase> DebugEventCallbackBasePtr;

    explicit CoreEngine(QObject *parent = 0);
    virtual ~CoreEngine();

    // Preliminary release interfaces.
    void releaseInterfaces();
    bool hasInterfaces() const;
    static bool interfacesAvailable();

    bool init(const QString &dllEnginePath, QString *errorMessage);
    // code level/output

    inline const ComInterfaces &interfaces() const { return m_cif; }

    // Set handlers
    DebugOutputBasePtr setDebugOutput(const DebugOutputBasePtr &);
    DebugEventCallbackBasePtr setDebugEventCallback(const DebugEventCallbackBasePtr &);

    // Start functions
    bool startDebuggerWithExecutable(const QString &workingDirectory,
                                     const QString &filename,
                                     const QString &args,
                                     const QStringList &env,
                                     QString *errorMessage);

    bool startAttachDebugger(qint64 pid, bool suppressInitialBreakPoint,
                             QString *errorMessage);

    ULONG executionStatus() const;
    bool setExecutionStatus(ULONG ex, QString *errorMessage);

    // break & interrupt
    bool debugBreakProcess(HANDLE hProcess, QString *errorMessage);
    // Currently does not interrupt debuggee
    bool setInterrupt(QString *errorMessage);

    // Helpers for terminating the debuggees and ending the session
    bool detachCurrentProcess(QString *appendableErrorMessage);
    bool terminateCurrentProcess(QString *appendableErrorMessage);
    bool terminateProcesses(QString *appendableErrorMessage);
    bool endSession(QString *appendableErrorMessage);

    // Watch timer: Listen for debug events and emit watchTimerDebugEvent() should one
    // occur.
    void startWatchTimer();
    void killWatchTimer();
    inline bool isWatchTimerRunning() const { return m_watchTimer != -1; }
    // Synchronous wait
    HRESULT waitForEvent(int timeOutMS);

    // Commands and expressions
    bool executeDebuggerCommand(const QString &command, QString *errorMessage);

    bool evaluateExpression(const QString &expression,
                            DEBUG_VALUE *debugValue,
                            QString *errorMessage);
    bool evaluateExpression(const QString &expression, QString *value,
                            QString *type /* =0 */, QString *errorMessage);

    // Path getters/setters
    QStringList sourcePaths() const;
    bool setSourcePaths(const QStringList &s, QString *errorMessage);
    QStringList symbolPaths() const;
    bool setSymbolPaths(const QStringList &s, QString *errorMessage);

    bool isVerboseSymbolLoading() const;
    bool setVerboseSymbolLoading(bool v);

    // Options
    ExpressionSyntax expressionSyntax() const;
    ExpressionSyntax setExpressionSyntax(ExpressionSyntax es);

    CodeLevel codeLevel() const;
    CodeLevel setCodeLevel(CodeLevel);

    QString dbengDLL() const { return m_dbengDLL; }

    // Debuggee memory conveniences
    bool allocDebuggeeMemory(int size, ULONG64 *address, QString *errorMessage);
    bool createDebuggeeAscIIString(const QString &s, ULONG64 *address, QString *errorMessage);
    bool writeToDebuggee(const QByteArray &buffer, quint64 address, QString *errorMessage);
    // Write to debuggee memory in chunks
    bool disassemble(ULONG64 offset, unsigned long beforeLines, unsigned long afterLines,
                     QString *target, QString *errorMessage);

    quint64 getSourceLineAddress(const QString &file, int line, QString *errorMessage) const;

    static bool autoDetectPath(QString *outPath,
                               QStringList *checkedDirectories = 0);

    unsigned moduleCount() const;

    bool setBreakOnThrow(bool b, QString *errorMessage);
    bool setExceptionCommands(ULONG code,
                              ULONG executionCommand,
                              ULONG continueCommand,
                              QString *errorMessage);

    QString eventFilterStatus() const;

signals:
    void watchTimerDebugEvent();

    // Emitted in the first time-out of the event handler in which
    // the number of modules no longer changes. Can be used as a
    // "startup" signal due to lack of a reliable "startup" detection
    // feature of the engine.
    void modulesLoaded();

public  slots:
    void outputVersion();

protected:
    virtual void timerEvent(QTimerEvent* te);

private:
    void setModuleCount(unsigned m);
    void resetModuleLoadTimer();

    static CoreEngine *m_instance;
    ComInterfaces m_cif;
    DebugOutputBasePtr m_debugOutput;
    DebugEventCallbackBasePtr m_debugEventCallback;
    QString m_dbengDLL;
    int m_watchTimer;
    unsigned m_moduleCount;
    unsigned m_lastTimerModuleCount;
    bool m_modulesLoadedEmitted;
};

// Utility messages
QString msgDebugEngineComResult(HRESULT hr);
QString msgComFailed(const char *func, HRESULT hr);
QString msgDebuggerCommandFailed(const QString &command, HRESULT hr);
const char *msgExecutionStatusString(ULONG executionStatus);

// A class that sets an expression syntax on the debug control while in scope.
// Can be nested as it checks for the old value.
class SyntaxSetter {
    Q_DISABLE_COPY(SyntaxSetter)
public:
    explicit inline SyntaxSetter(CoreEngine *engine, CoreEngine::ExpressionSyntax es) :
                        m_oldSyntax(engine->setExpressionSyntax(es)),
                        m_engine(engine) {}
    inline  ~SyntaxSetter() { m_engine->setExpressionSyntax(m_oldSyntax); }

private:
    const CoreEngine::ExpressionSyntax m_oldSyntax;
    CoreEngine *m_engine;
};

// Helpers to convert DEBUG_VALUE structs. The optional control is required to
// convert large floating values.
QString debugValueToString(const DEBUG_VALUE &dv, QString *qType =0, int integerBase = 10, CIDebugControl *ctl = 0);
bool debugValueToInteger(const DEBUG_VALUE &dv, qint64 *value);

} // namespace CdbCore

#endif // COREENGINE_H
