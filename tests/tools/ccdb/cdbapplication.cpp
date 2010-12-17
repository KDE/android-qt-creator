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

#include "cdbapplication.h"
#include "coreengine.h"
#include "cdbdebugoutput.h"
#include "cdbpromptthread.h"
#include "debugeventcallback.h"
#include "symbolgroupcontext.h"
#include "stacktracecontext.h"
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QDebug>

#include <cstdio>
#include <cerrno>

const char usage[] =
"CDB command line test tool\n\n"
"ccdb <Options>\n"
"Options: -p engine path\n"
"         -c initial_command_file\n";

class PrintfOutputHandler : public CdbCore::DebugOutputBase
{
public:
    PrintfOutputHandler() {}

protected:
    virtual void output(ULONG mask, const QString &message)
    { std::printf("%10s: %s\n", maskDescription(mask), qPrintable(message)); }
};


// -------------- CdbApplication
CdbApplication::CdbApplication(int argc, char *argv[]) :
        QCoreApplication(argc, argv),
        m_engine(new CdbCore::CoreEngine),
        m_promptThread(0),
        m_processHandle(0)
{
}

CdbApplication::~CdbApplication()
{
}

CdbApplication::InitResult CdbApplication::init()
{
    FILE *inputFile;
    if (!parseOptions(&inputFile)) {
        printf(usage);
        return InitUsageShown;
    }
    QString errorMessage;
    std::printf("Initializing engine %s...\n", qPrintable(m_engineDll));
    if (!m_engine->init(m_engineDll, &errorMessage)) {
        std::fprintf(stderr, "Failed: %s\n", qPrintable(errorMessage));
        return InitFailed;
    }
    m_engine->setDebugOutput(CdbCore::CoreEngine::DebugOutputBasePtr(new PrintfOutputHandler));
    DebugEventCallback *evt = new DebugEventCallback;
    connect(evt, SIGNAL(processAttached(void*)), this, SLOT(processAttached(void*)));
    m_engine->setDebugEventCallback(CdbCore::CoreEngine::DebugEventCallbackBasePtr(evt));
    m_engine->setExpressionSyntax(CdbCore::CoreEngine::CppExpressionSyntax);
    m_engine->setCodeLevel(CdbCore::CoreEngine::CodeLevelSource);
    connect(m_engine.data(), SIGNAL(watchTimerDebugEvent()), this, SLOT(debugEvent()));
    std::printf("Succeded.\n");
    // Prompt
    m_promptThread = new CdbPromptThread(inputFile, this);
    connect(m_promptThread, SIGNAL(finished()), this, SLOT(promptThreadTerminated()));
    connect(m_promptThread, SIGNAL(asyncCommand(int,QString)),
            this, SLOT(asyncCommand(int,QString)), Qt::QueuedConnection);
    connect(m_promptThread, SIGNAL(syncCommand(int,QString)),
            this, SLOT(syncCommand(int,QString)), Qt::BlockingQueuedConnection);
    connect(m_promptThread, SIGNAL(executionCommand(int,QString)),
            this, SLOT(executionCommand(int,QString)), Qt::BlockingQueuedConnection);
    connect(m_engine.data(), SIGNAL(watchTimerDebugEvent()), m_promptThread, SLOT(notifyDebugEvent()),
            Qt::QueuedConnection);
    m_promptThread->start();
    return InitOk;
}

void CdbApplication::promptThreadTerminated()
{
    QString errorMessage;
    m_engine->endSession(&errorMessage);
    std::printf("Terminating.\n");
    m_promptThread->wait();
    quit();
}

bool CdbApplication::parseOptions(FILE **inputFile)
{
    *inputFile = NULL;
    const QStringList args = QCoreApplication::arguments();
    const QStringList::const_iterator cend = args.constEnd();
    QStringList::const_iterator it = args.constBegin();
    for (++it; it != cend ; ++it) {
        const QString &a = *it;
        if (a.startsWith(QLatin1Char('-')) && a.size() >= 2) {
            switch (a.at(1).toAscii()) {
            case 'p':
                ++it;
                if (it == cend) {
                    std::fprintf(stderr, "Option -p is missing an argument.\n");
                    return false;
                }
                m_engineDll = *it;
                break;
            case 'c':
                ++it;
                if (it == cend) {
                    std::fprintf(stderr, "Option -c is missing an argument.\n");
                    return false;
                }
                *inputFile = std::fopen( it->toLocal8Bit().constData(), "r");
                if (*inputFile == NULL) {
                    std::fprintf(stderr, "Cannot open %s: %s\n", qPrintable(*it), std::strerror(errno));
                    return false;
                }
                break;

            default:
                std::fprintf(stderr, "Invalid option %s\n", qPrintable(a));
                return false;
            }
        }
    }
    return true;
}

void CdbApplication::asyncCommand(int command, const QString &arg)
{
    Q_UNUSED(arg)
    QString errorMessage;
    switch (command) {
    case Async_Interrupt:
        if (m_processHandle) {
            if (m_engine->debugBreakProcess(m_processHandle, &errorMessage)) {
                std::printf("Stopped\n");
            } else {
                std::printf("%s\n", qPrintable(errorMessage));
            }
        }
        break;
    }
}

void CdbApplication::printFrame(const QString &arg)
{
    QString errorMessage;
    do {
        if (m_stackTrace.isNull()) {
            errorMessage = QLatin1String("No trace.");
            break;
        }
        bool ok;
        const int frame = arg.toInt(&ok);
        if (!ok || frame < 0 || frame >= m_stackTrace->frameCount()) {
            errorMessage = QLatin1String("Invalid or out of range.");
            break;
        }
        CdbCore::SymbolGroupContext *ctx = m_stackTrace->symbolGroupContextAt(frame, &errorMessage);
        if (!ctx)
            break;
        printf("%s\n", qPrintable(ctx->toString()));
    } while (false);
    if (!errorMessage.isEmpty())
        printf("%s\n", qPrintable(errorMessage));
}

// Return address or 0 on failure
quint64 CdbApplication::addQueuedBreakPoint(const QString &arg, QString *errorMessage)
{
    // Queue file:line
    const int cpos = arg.lastIndexOf(QLatin1Char(':'));
    if (cpos == -1) {
        *errorMessage = QString::fromLatin1("Syntax error in '%1': No colon.").arg(arg);
        return 0;
    }

    const QString fileName = arg.left(cpos);
    bool ok;
    const int lineNumber = arg.mid(cpos + 1).toInt(&ok);
    if (!ok || lineNumber < 1) {
        *errorMessage = QString::fromLatin1("Syntax error in '%1': No line number.").arg(arg);
        return 0;
    }
    CdbCore::BreakPoint bp;
    bp.address = m_engine->getSourceLineAddress(fileName, lineNumber, errorMessage);
    if (!bp.address)
        return 0;
    if (!bp.add(m_engine->interfaces().debugControl, errorMessage))
        return 0;
    return bp.address;
}

void CdbApplication::syncCommand(int command, const QString &arg)
{
    QString errorMessage;
    switch (command) {
    case Sync_EvalExpression: {
            QString value;
            QString type;
            std::printf("Evaluating '%s' in code level %d, syntax %d\n",
                        qPrintable(arg), m_engine->codeLevel(), m_engine->expressionSyntax());
            if (m_engine->evaluateExpression(arg, &value, &type, &errorMessage)) {
                std::printf("[%s] %s\n", qPrintable(type), qPrintable(value));
            } else {
                std::printf("%s\n", qPrintable(errorMessage));
            }
        }
        break;
    case Sync_Queue: {
        const QString targs = arg.trimmed();
        if (targs.isEmpty()) {
            std::printf("Queue cleared\n");
            m_queuedCommands.clear();
        } else {
            std::printf("Queueing %s\n", qPrintable(targs));
            m_queuedCommands.push_back(targs);
        }
    }
        break;
    case Sync_QueueBreakPoint: {
        const QString targs = arg.trimmed();
        if (targs.isEmpty()) {
            std::printf("Breakpoint queue cleared\n");
            m_queuedBreakPoints.clear();
        } else {
            m_queuedBreakPoints.push_back(targs);
            std::printf("Queueing breakpoint %s\n", qPrintable(targs));
        }
    }
        break;
    case Sync_ListBreakPoints: {
            QList<CdbCore::BreakPoint> bps;
            if (CdbCore::BreakPoint::getBreakPoints(m_engine->interfaces().debugControl, &bps, &errorMessage)) {
                foreach (const CdbCore::BreakPoint &bp, bps)
                    std::printf("%s\n", qPrintable(bp.expression()));
            } else {
                std::printf("BREAKPOINT LIST FAILED: %s\n", qPrintable(errorMessage));
            }
}
        break;
    case Sync_PrintFrame:
        printFrame(arg);
        break;
    case Sync_OutputVersion:
        m_engine->outputVersion();
        break;
    case Sync_Python:
        break;
    case Unknown:
        std::printf("Executing '%s' in code level %d, syntax %d\n",
                    qPrintable(arg), m_engine->codeLevel(), m_engine->expressionSyntax());
        if (!m_engine->executeDebuggerCommand(arg, &errorMessage))
            std::printf("%s\n", qPrintable(errorMessage));
        break;
    }
}

void CdbApplication::executionCommand(int command, const QString &arg)
{
    bool ok = false;
    QString errorMessage;
    switch (command) {
    case Execution_StartBinary: {
            QStringList args = arg.split(QLatin1Char(' '), QString::SkipEmptyParts);
            if (args.isEmpty()) {
                errorMessage = QLatin1String("Specify executable.");
            } else {
                std::printf("Starting\n");
                const QString binary = args.front();
                args.pop_front();
                ok = m_engine->startDebuggerWithExecutable(QString(), binary, args,
                                                           QStringList(), &errorMessage);
            }
        }
        break;
    case Execution_Go:
        std::printf("Go\n");
        ok = m_engine->setExecutionStatus(DEBUG_STATUS_GO, &errorMessage);
        break;
    }
    if (ok) {
        m_engine->startWatchTimer();
        m_stackTrace = QSharedPointer<CdbCore::StackTraceContext>();
    } else {
        std::fprintf(stderr, "%s\n", qPrintable(errorMessage));
    }
}

void CdbApplication::debugEvent()
{
    QString errorMessage;
    std::printf("Debug event\n");

    QVector<CdbCore::Thread> threads;
    QVector<CdbCore::StackFrame> threadFrames;
    ULONG currentThreadId;

    if (CdbCore::StackTraceContext::getThreadList(m_engine->interfaces(), &threads, &currentThreadId, &errorMessage)
        && CdbCore::StackTraceContext::getStoppedThreadFrames(m_engine->interfaces(), currentThreadId, threads, &threadFrames, &errorMessage)) {
        const int count = threadFrames.size();
        for (int i = 0; i  < count; i++) {
            printf("Thread #%02d ID %10d SYSID %10d %s\n", i,
                   threads.at(i).id, threads.at(i).systemId, qPrintable(threadFrames.at(i).toString()));
        }
    } else {
        std::fprintf(stderr, "%s\n", qPrintable(errorMessage));
    }

    CdbCore::StackTraceContext *trace =
        CdbCore::StackTraceContext::create(&m_engine->interfaces(),
                                           0xFFFF, &errorMessage);
    if (trace) {
        m_stackTrace = QSharedPointer<CdbCore::StackTraceContext>(trace);
        printf("%s\n", qPrintable(m_stackTrace->toString()));
    } else {
        std::fprintf(stderr, "%s\n", qPrintable(errorMessage));
    }
}

void CdbApplication::processAttached(void *handle)
{
    std::printf("### Process attached\n");
    m_processHandle = handle;
    QString errorMessage;
    // Commands
    foreach(const QString &qc, m_queuedCommands) {
        if (m_engine->executeDebuggerCommand(qc, &errorMessage)) {
            std::printf("'%s' [ok]\n", qPrintable(qc));
        } else {
            std::printf("%s\n", qPrintable(errorMessage));
        }
    }
    // Breakpoints
    foreach(const QString &bp, m_queuedBreakPoints) {
        if (const quint64 address = addQueuedBreakPoint(bp, &errorMessage)) {
            std::printf("'%s' 0x%lx [ok]\n", qPrintable(bp), address);
        } else {
            std::fprintf(stderr, "%s: %s\n",
                         qPrintable(bp),
                         qPrintable(errorMessage));
        }
    }
}
