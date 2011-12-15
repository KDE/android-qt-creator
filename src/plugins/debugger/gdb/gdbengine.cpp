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

#define QT_NO_CAST_FROM_ASCII

#include "gdbengine.h"

#include "debuggerstartparameters.h"
#include "debuggerinternalconstants.h"
#include "disassemblerlines.h"
#include "attachgdbadapter.h"
#include "coregdbadapter.h"
#include "localplaingdbadapter.h"
#include "termgdbadapter.h"
#include "remotegdbserveradapter.h"
#include "remoteplaingdbadapter.h"
#include "codagdbadapter.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerplugin.h"
#include "debuggerrunner.h"
#include "debuggerstringutils.h"
#include "debuggertooltipmanager.h"
#include "disassembleragent.h"
#include "gdbmi.h"
#include "gdboptionspage.h"
#include "memoryagent.h"
#include "watchutils.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "snapshothandler.h"
#include "sourcefileshandler.h"
#include "stackhandler.h"
#include "threadshandler.h"
#include "watchhandler.h"
#include "debuggersourcepathmappingwidget.h"
#include "hostutils.h"
#include "logwindow.h"

#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <texteditor/itexteditor.h>
#include <utils/qtcassert.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaObject>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTextStream>

#include <QtGui/QAction>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <dlfcn.h>
#endif
#include <ctype.h>

using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

class GdbToolTipContext : public DebuggerToolTipContext
{
public:
    GdbToolTipContext(const DebuggerToolTipContext &c) :
        DebuggerToolTipContext(c), editor(0) {}

    QPoint mousePosition;
    QString expression;
    Core::IEditor *editor;
};

enum { debugPending = 0 };

#define PENDING_DEBUG(s) do { if (debugPending) qDebug() << s; } while (0)

#define CB(callback) &GdbEngine::callback, STRINGIFY(callback)

QByteArray GdbEngine::tooltipIName(const QString &exp)
{
    return "tooltip." + exp.toLatin1().toHex();
}

static bool stateAcceptsGdbCommands(DebuggerState state)
{
    switch (state) {
    case EngineSetupRequested:
    case EngineSetupOk:
    case EngineSetupFailed:
    case InferiorUnrunnable:
    case InferiorSetupRequested:
    case InferiorSetupFailed:
    case EngineRunRequested:
    case InferiorRunRequested:
    case InferiorRunOk:
    case InferiorStopRequested:
    case InferiorStopOk:
    case InferiorShutdownRequested:
    case EngineShutdownRequested:
    case InferiorShutdownOk:
    case InferiorShutdownFailed:
        return true;
    case DebuggerNotReady:
    case InferiorStopFailed:
    case InferiorSetupOk:
    case EngineRunFailed:
    case InferiorExitOk:
    case InferiorRunFailed:
    case EngineShutdownOk:
    case EngineShutdownFailed:
    case DebuggerFinished:
        return false;
    }
    return false;
}

static int &currentToken()
{
    static int token = 0;
    return token;
}

static QByteArray parsePlainConsoleStream(const GdbResponse &response)
{
    QByteArray out = response.consoleStreamOutput;
    // FIXME: proper decoding needed
    if (out.endsWith("\\n"))
        out.chop(2);
    while (out.endsWith('\n') || out.endsWith(' '))
        out.chop(1);
    int pos = out.indexOf(" = ");
    return out.mid(pos + 3);
}

///////////////////////////////////////////////////////////////////////
//
// GdbEngine
//
///////////////////////////////////////////////////////////////////////

GdbEngine::GdbEngine(const DebuggerStartParameters &startParameters,
        DebuggerEngine *masterEngine)
  : DebuggerEngine(startParameters, CppLanguage, masterEngine)
{
    setObjectName(_("GdbEngine"));

    m_busy = false;
    m_gdbAdapter = 0;
    m_debuggingHelperState = DebuggingHelperUninitialized;
    m_gdbVersion = 100;
    m_gdbBuildVersion = -1;
    m_isMacGdb = false;
    m_hasBreakpointNotifications = false;
    m_hasPython = false;
    m_registerNamesListed = false;
    m_hasInferiorThreadList = false;
    m_sourcesListUpdating = false;
    m_oldestAcceptableToken = -1;
    m_nonDiscardableCount = 0;
    m_outputCodec = QTextCodec::codecForLocale();
    m_pendingWatchRequests = 0;
    m_pendingBreakpointRequests = 0;
    m_commandsDoneCallback = 0;
    m_stackNeeded = false;
    m_preparedForQmlBreak = false;
    m_disassembleUsesComma = false;
    m_actingOnExpectedStop = false;

    invalidateSourcesList();

    m_gdbAdapter = createAdapter();

    m_commandTimer.setSingleShot(true);
    connect(&m_commandTimer, SIGNAL(timeout()), SLOT(commandTimeout()));

    connect(debuggerCore()->action(AutoDerefPointers), SIGNAL(valueChanged(QVariant)),
            SLOT(reloadLocals()));
    connect(debuggerCore()->action(CreateFullBacktrace), SIGNAL(triggered()),
            SLOT(createFullBacktrace()));
}

DebuggerStartMode GdbEngine::startMode() const
{
    return startParameters().startMode;
}

AbstractGdbProcess *GdbEngine::gdbProc() const
{
    return m_gdbAdapter->gdbProc();
}

GdbEngine::~GdbEngine()
{
    // Prevent sending error messages afterwards.
    if (m_gdbAdapter)
        disconnect(gdbProc(), 0, this, 0);
    delete m_gdbAdapter;
    m_gdbAdapter = 0;
}

QString GdbEngine::errorMessage(QProcess::ProcessError error)
{
    switch (error) {
        case QProcess::FailedToStart:
            return tr("The gdb process failed to start. Either the "
                "invoked program '%1' is missing, or you may have insufficient "
                "permissions to invoke the program.\n%2")
                .arg(m_gdb, gdbProc()->errorString());
        case QProcess::Crashed:
            if (targetState() == DebuggerFinished)
                return tr("The gdb process crashed some time after starting "
                    "successfully.");
            else
                return tr("The gdb process was ended forcefully");
        case QProcess::Timedout:
            return tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
        case QProcess::WriteError:
            return tr("An error occurred when attempting to write "
                "to the gdb process. For example, the process may not be running, "
                "or it may have closed its input channel.");
        case QProcess::ReadError:
            return tr("An error occurred when attempting to read from "
                "the gdb process. For example, the process may not be running.");
        default:
            return tr("An unknown error in the gdb process occurred. ");
    }
}

#if 0
static void dump(const char *first, const char *middle, const QString & to)
{
    QByteArray ba(first, middle - first);
    Q_UNUSED(to)
    // note that qDebug cuts off output after a certain size... (bug?)
    qDebug("\n>>>>> %s\n%s\n====\n%s\n<<<<<\n",
        qPrintable(currentTime()),
        qPrintable(QString(ba).trimmed()),
        qPrintable(to.trimmed()));
    //qDebug() << "";
    //qDebug() << qPrintable(currentTime())
    //    << " Reading response:  " << QString(ba).trimmed() << "\n";
}
#endif

// Parse "~:gdb: unknown target exception 0xc0000139 at 0x77bef04e\n"
// and return an exception message
static inline QString msgWinException(const QByteArray &data)
{
    const int exCodePos = data.indexOf("0x");
    const int blankPos = exCodePos != -1 ? data.indexOf(' ', exCodePos + 1) : -1;
    const int addressPos = blankPos != -1 ? data.indexOf("0x", blankPos + 1) : -1;
    if (addressPos < 0)
        return GdbEngine::tr("An exception was triggered.");
    const unsigned exCode = data.mid(exCodePos, blankPos - exCodePos).toUInt(0, 0);
    const quint64 address = data.mid(addressPos).trimmed().toULongLong(0, 0);
    QString rc;
    QTextStream str(&rc);
    str << GdbEngine::tr("An exception was triggered: ");
#ifdef Q_OS_WIN
    formatWindowsException(exCode, address, 0, 0, 0, str);
#else
    Q_UNUSED(exCode)
    Q_UNUSED(address)
#endif
    str << '.';
    return rc;
}

void GdbEngine::readDebugeeOutput(const QByteArray &data)
{
    QString msg = m_outputCodec->toUnicode(data.constData(), data.length(),
        &m_outputCodecState);
    showMessage(msg, AppOutput);
}

static bool isNameChar(char c)
{
    // could be 'stopped' or 'shlibs-added'
    return (c >= 'a' && c <= 'z') || c == '-';
}

void GdbEngine::handleResponse(const QByteArray &buff)
{
    showMessage(QString::fromLocal8Bit(buff, buff.length()), LogOutput);

    if (buff.isEmpty() || buff == "(gdb) ")
        return;

    const char *from = buff.constData();
    const char *to = from + buff.size();
    const char *inner;

    int token = -1;
    // Token is a sequence of numbers.
    for (inner = from; inner != to; ++inner)
        if (*inner < '0' || *inner > '9')
            break;
    if (from != inner) {
        token = QByteArray(from, inner - from).toInt();
        from = inner;
    }

    // Next char decides kind of response.
    const char c = *from++;
    switch (c) {
        case '*':
        case '+':
        case '=': {
            QByteArray asyncClass;
            for (; from != to; ++from) {
                const char c = *from;
                if (!isNameChar(c))
                    break;
                asyncClass += *from;
            }

            GdbMi result;
            while (from != to) {
                GdbMi data;
                if (*from != ',') {
                    // happens on archer where we get
                    // 23^running <NL> *running,thread-id="all" <NL> (gdb)
                    result.m_type = GdbMi::Tuple;
                    break;
                }
                ++from; // skip ','
                data.parseResultOrValue(from, to);
                if (data.isValid()) {
                    //qDebug() << "parsed result:" << data.toString();
                    result.m_children += data;
                    result.m_type = GdbMi::Tuple;
                }
            }
            if (asyncClass == "stopped") {
                handleStopResponse(result);
                m_pendingLogStreamOutput.clear();
                m_pendingConsoleStreamOutput.clear();
            } else if (asyncClass == "running") {
                // Archer has 'thread-id="all"' here
            } else if (asyncClass == "library-loaded") {
                // Archer has 'id="/usr/lib/libdrm.so.2",
                // target-name="/usr/lib/libdrm.so.2",
                // host-name="/usr/lib/libdrm.so.2",
                // symbols-loaded="0"
                QByteArray id = result.findChild("id").data();
                if (!id.isEmpty())
                    showStatusMessage(tr("Library %1 loaded").arg(_(id)), 1000);
                progressPing();
                invalidateSourcesList();
            } else if (asyncClass == "library-unloaded") {
                // Archer has 'id="/usr/lib/libdrm.so.2",
                // target-name="/usr/lib/libdrm.so.2",
                // host-name="/usr/lib/libdrm.so.2"
                QByteArray id = result.findChild("id").data();
                progressPing();
                showStatusMessage(tr("Library %1 unloaded").arg(_(id)), 1000);
                invalidateSourcesList();
            } else if (asyncClass == "thread-group-added") {
                // 7.1-symbianelf has "{id="i1"}"
            } else if (asyncClass == "thread-group-created"
                    || asyncClass == "thread-group-started") {
                // Archer had only "{id="28902"}" at some point of 6.8.x.
                // *-started seems to be standard in 7.1, but in early
                // 7.0.x, there was a *-created instead.
                progressPing();
                // 7.1.50 has thread-group-started,id="i1",pid="3529"
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread group %1 created").arg(_(id)), 1000);
                int pid = id.toInt();
                if (!pid) {
                    id = result.findChild("pid").data();
                    pid = id.toInt();
                }
                if (pid)
                    notifyInferiorPid(pid);
            } else if (asyncClass == "thread-created") {
                //"{id="1",group-id="28902"}"
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread %1 created").arg(_(id)), 1000);
            } else if (asyncClass == "thread-group-exited") {
                // Archer has "{id="28902"}"
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread group %1 exited").arg(_(id)), 1000);
            } else if (asyncClass == "thread-exited") {
                //"{id="1",group-id="28902"}"
                QByteArray id = result.findChild("id").data();
                QByteArray groupid = result.findChild("group-id").data();
                showStatusMessage(tr("Thread %1 in group %2 exited")
                    .arg(_(id)).arg(_(groupid)), 1000);
            } else if (asyncClass == "thread-selected") {
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread %1 selected").arg(_(id)), 1000);
                //"{id="2"}"
            } else if (m_isMacGdb && asyncClass == "shlibs-updated") {
                // Apple's gdb announces updated libs.
                invalidateSourcesList();
            } else if (m_isMacGdb && asyncClass == "shlibs-added") {
                // Apple's gdb announces added libs.
                // {shlib-info={num="2", name="libmathCommon.A_debug.dylib",
                // kind="-", dyld-addr="0x7f000", reason="dyld", requested-state="Y",
                // state="Y", path="/usr/lib/system/libmathCommon.A_debug.dylib",
                // description="/usr/lib/system/libmathCommon.A_debug.dylib",
                // loaded_addr="0x7f000", slide="0x7f000", prefix=""}}
                invalidateSourcesList();
            } else if (m_isMacGdb && asyncClass == "resolve-pending-breakpoint") {
                // Apple's gdb announces resolved breakpoints.
                // new_bp="1",pended_bp="1",new_expr="\"gdbengine.cpp\":1584",
                // bkpt={number="1",type="breakpoint",disp="keep",enabled="y",
                // addr="0x0000000115cc3ddf",func="foo()",file="../foo.cpp",
                // line="1584",shlib="/../libFoo_debug.dylib",times="0"}
                const GdbMi bkpt = result.findChild("bkpt");
                const BreakpointResponseId rid(bkpt.findChild("number").data());
                if (!isQmlStepBreakpoint(rid)) {
                    BreakHandler *handler = breakHandler();
                    BreakpointModelId id = handler->findBreakpointByResponseId(rid);
                    BreakpointResponse br = handler->response(id);
                    updateResponse(br, bkpt);
                    handler->setResponse(id, br);
                    attemptAdjustBreakpointLocation(id);
                }
            } else if (asyncClass == "breakpoint-modified") {
                // New in FSF gdb since 2011-04-27.
                // "{bkpt={number="3",type="breakpoint",disp="keep",
                // enabled="y",addr="<MULTIPLE>",times="1",
                // original-location="\\",simple_gdbtest_app.cpp\\":135"},
                // {number="3.1",enabled="y",addr="0x0805ff68",
                // func="Vector<int>::Vector(int)",
                // file="simple_gdbtest_app.cpp",
                // fullname="/data/...line="135"},{number="3.2"...}}"

                // Note the leading comma in original-location. Filter it out.
                // We don't need the field anyway.
                QByteArray ba = result.toString();
                ba = '[' + ba.mid(6, ba.size() - 7) + ']';
                const int pos1 = ba.indexOf(",original-location");
                const int pos2 = ba.indexOf("\":", pos1 + 2);
                const int pos3 = ba.indexOf('"', pos2 + 2);
                ba.replace(pos1, pos3 - pos1 + 1, "");
                result = GdbMi();
                result.fromString(ba);
                BreakHandler *handler = breakHandler();
                BreakpointModelId id;
                BreakpointResponse br;
                foreach (const GdbMi &bkpt, result.children()) {
                    const QByteArray nr = bkpt.findChild("number").data();
                    BreakpointResponseId rid(nr);
                    if (!isHiddenBreakpoint(rid)) {
                        if (nr.contains('.')) {
                            // A sub-breakpoint.
                            BreakpointResponse sub;
                            updateResponse(sub, bkpt);
                            sub.id = rid;
                            sub.type = br.type;
                            handler->insertSubBreakpoint(id, sub);
                        } else {
                            // A primary breakpoint.
                            id = handler->findBreakpointByResponseId(rid);
                            //qDebug() << "NR: " << nr << "RID: " << rid
                            //    << "ID: " << id;
                            //BreakpointModelId id =
                            //    handler->findBreakpointByResponseId(rid);
                            br = handler->response(id);
                            updateResponse(br, bkpt);
                            handler->setResponse(id, br);
                        }
                    }
                }
                m_hasBreakpointNotifications = true;
            } else if (asyncClass == "breakpoint-created") {
                // "{bkpt={number="1",type="breakpoint",disp="del",enabled="y",
                //  addr="<PENDING>",pending="main",times="0",
                //original-location="main"}}"
                BreakHandler *handler = breakHandler();
                foreach (const GdbMi &bkpt, result.children()) {
                    BreakpointResponse br;
                    updateResponse(br, bkpt);
                    BreakpointModelId id = handler->findBreakpointByResponseId(br.id);
                    handler->handleAlienBreakpoint(id, br, this);
                }
            } else if (asyncClass == "breakpoint-deleted") {
                // "breakpoint-deleted" "{id="1"}"
                // New in FSF gdb since 2011-04-27.
                BreakHandler *handler = breakHandler();
                QByteArray nr = result.findChild("id").data();
                BreakpointResponseId rid(nr);
                BreakpointModelId id = handler->findBreakpointByResponseId(rid);
                if (id.isValid())
                    handler->removeAlienBreakpoint(id);
            } else {
                qDebug() << "IGNORED ASYNC OUTPUT"
                    << asyncClass << result.toString();
            }
            break;
        }

        case '~': {
            QByteArray data = GdbMi::parseCString(from, to);
            m_pendingConsoleStreamOutput += data;

            // Parse pid from noise.
            if (!inferiorPid()) {
                // Linux/Mac gdb: [New [Tt]hread 0x545 (LWP 4554)]
                static QRegExp re1(_("New .hread 0x[0-9a-f]+ \\(LWP ([0-9]*)\\)"));
                // MinGW 6.8: [New thread 2437.0x435345]
                static QRegExp re2(_("New .hread ([0-9]+)\\.0x[0-9a-f]*"));
                // Mac: [Switching to process 9294 local thread 0x2e03] or
                // [Switching to process 31773]
                static QRegExp re3(_("Switching to process ([0-9]+)"));
                QTC_ASSERT(re1.isValid() && re2.isValid(), return);
                if (re1.indexIn(_(data)) != -1)
                    maybeHandleInferiorPidChanged(re1.cap(1));
                else if (re2.indexIn(_(data)) != -1)
                    maybeHandleInferiorPidChanged(re2.cap(1));
                else if (re3.indexIn(_(data)) != -1)
                    maybeHandleInferiorPidChanged(re3.cap(1));
            }

            // Show some messages to give the impression something happens.
            if (data.startsWith("Reading symbols from ")) {
                showStatusMessage(tr("Reading %1...").arg(_(data.mid(21))), 1000);
                progressPing();
                invalidateSourcesList();
            } else if (data.startsWith("[New ") || data.startsWith("[Thread ")) {
                if (data.endsWith('\n'))
                    data.chop(1);
                progressPing();
                showStatusMessage(_(data), 1000);
            } else if (data.startsWith("gdb: unknown target exception 0x")) {
                // [Windows, most likely some DLL/Entry point not found]:
                // "gdb: unknown target exception 0xc0000139 at 0x77bef04e"
                // This may be fatal and cause the target to exit later
                m_lastWinException = msgWinException(data);
                showMessage(m_lastWinException, LogMisc);
            }

            if (data.startsWith("QMLBP:")) {
                int pos1 = 6;
                int pos2 = data.indexOf(' ', pos1);
                m_qmlBreakpointResponseId2 = BreakpointResponseId(data.mid(pos1, pos2 - pos1));
                //qDebug() << "FOUND QMLBP: " << m_qmlBreakpointNumbers[2];
                //qDebug() << "CURRENT: " << m_qmlBreakpointNumbers;
            }

            break;
        }

        case '@': {
            readDebugeeOutput(GdbMi::parseCString(from, to));
            break;
        }

        case '&': {
            QByteArray data = GdbMi::parseCString(from, to);
            m_pendingLogStreamOutput += data;
            // On Windows, the contents seem to depend on the debugger
            // version and/or OS version used.
            if (data.startsWith("warning:"))
                showMessage(_(data.mid(9)), AppStuff); // Cut "warning: "
            break;
        }

        case '^': {
            GdbResponse response;

            response.token = token;

            for (inner = from; inner != to; ++inner)
                if (*inner < 'a' || *inner > 'z')
                    break;

            QByteArray resultClass = QByteArray::fromRawData(from, inner - from);
            if (resultClass == "done") {
                response.resultClass = GdbResultDone;
            } else if (resultClass == "running") {
                response.resultClass = GdbResultRunning;
            } else if (resultClass == "connected") {
                response.resultClass = GdbResultConnected;
            } else if (resultClass == "error") {
                response.resultClass = GdbResultError;
            } else if (resultClass == "exit") {
                response.resultClass = GdbResultExit;
            } else {
                response.resultClass = GdbResultUnknown;
            }

            from = inner;
            if (from != to) {
                if (*from == ',') {
                    ++from;
                    response.data.parseTuple_helper(from, to);
                    response.data.m_type = GdbMi::Tuple;
                    response.data.m_name = "data";
                } else {
                    // Archer has this.
                    response.data.m_type = GdbMi::Tuple;
                    response.data.m_name = "data";
                }
            }

            //qDebug() << "\nLOG STREAM:" + m_pendingLogStreamOutput;
            //qDebug() << "\nCONSOLE STREAM:" + m_pendingConsoleStreamOutput;
            response.logStreamOutput = m_pendingLogStreamOutput;
            response.consoleStreamOutput =  m_pendingConsoleStreamOutput;
            m_pendingLogStreamOutput.clear();
            m_pendingConsoleStreamOutput.clear();

            handleResultRecord(&response);
            break;
        }
        default: {
            qDebug() << "UNKNOWN RESPONSE TYPE" << c;
            break;
        }
    }
}

void GdbEngine::readGdbStandardError()
{
    QByteArray err = gdbProc()->readAllStandardError();
    showMessage(_("UNEXPECTED GDB STDERR: " + err));
    if (err == "Undefined command: \"bb\".  Try \"help\".\n")
        return;
    if (err.startsWith("BFD: reopening"))
        return;
    qWarning() << "Unexpected GDB stderr:" << err;
}

void GdbEngine::readGdbStandardOutput()
{
    m_commandTimer.start(); // Restart timer.

    int newstart = 0;
    int scan = m_inbuffer.size();

    m_inbuffer.append(gdbProc()->readAllStandardOutput());

    // This can trigger when a dialog starts a nested event loop.
    if (m_busy)
        return;

    while (newstart < m_inbuffer.size()) {
        int start = newstart;
        int end = m_inbuffer.indexOf('\n', scan);
        if (end < 0) {
            m_inbuffer.remove(0, start);
            return;
        }
        newstart = end + 1;
        scan = newstart;
        if (end == start)
            continue;
        if (m_inbuffer.at(end - 1) == '\r') {
            --end;
            if (end == start)
                continue;
        }
        m_busy = true;
        handleResponse(QByteArray::fromRawData(m_inbuffer.constData() + start, end - start));
        m_busy = false;
    }
    m_inbuffer.clear();
}

void GdbEngine::interruptInferior()
{
    QTC_ASSERT(state() == InferiorStopRequested,
        qDebug() << "INTERRUPT INFERIOR: " << state(); return);

    if (0 && debuggerCore()->boolSetting(TargetAsync)) {
        postCommand("-exec-interrupt");
    } else {
        showStatusMessage(tr("Stop requested..."), 5000);
        showMessage(_("TRYING TO INTERRUPT INFERIOR"));
        m_gdbAdapter->interruptInferior();
    }
}

void GdbEngine::interruptInferiorTemporarily()
{
    foreach (const GdbCommand &cmd, m_commandsToRunOnTemporaryBreak) {
        if (cmd.flags & LosesChild) {
            notifyInferiorIll();
            return;
        }
    }
    requestInterruptInferior();
}

void GdbEngine::maybeHandleInferiorPidChanged(const QString &pid0)
{
    const qint64 pid = pid0.toLongLong();
    if (pid == 0) {
        showMessage(_("Cannot parse PID from %1").arg(pid0));
        return;
    }
    if (pid == inferiorPid())
        return;

    showMessage(_("FOUND PID %1").arg(pid));
    notifyInferiorPid(pid);
}

void GdbEngine::postCommand(const QByteArray &command, AdapterCallback callback,
                            const char *callbackName, const QVariant &cookie)
{
    postCommand(command, NoFlags, callback, callbackName, cookie);
}

void GdbEngine::postCommand(const QByteArray &command, GdbCommandFlags flags,
                            AdapterCallback callback,
                            const char *callbackName, const QVariant &cookie)
{
    GdbCommand cmd;
    cmd.command = command;
    cmd.flags = flags;
    cmd.adapterCallback = callback;
    cmd.callbackName = callbackName;
    cmd.cookie = cookie;
    postCommandHelper(cmd);
}

void GdbEngine::postCommand(const QByteArray &command, GdbCommandCallback callback,
                            const char *callbackName, const QVariant &cookie)
{
    postCommand(command, NoFlags, callback, callbackName, cookie);
}

void GdbEngine::postCommand(const QByteArray &command, GdbCommandFlags flags,
                            GdbCommandCallback callback, const char *callbackName,
                            const QVariant &cookie)
{
    GdbCommand cmd;
    cmd.command = command;
    cmd.flags = flags;
    cmd.callback = callback;
    cmd.callbackName = callbackName;
    cmd.cookie = cookie;
    postCommandHelper(cmd);
}

void GdbEngine::postCommandHelper(const GdbCommand &cmd)
{
    if (!stateAcceptsGdbCommands(state())) {
        PENDING_DEBUG(_("NO GDB PROCESS RUNNING, CMD IGNORED: " + cmd.command));
        showMessage(_("NO GDB PROCESS RUNNING, CMD IGNORED: %1 %2")
            .arg(_(cmd.command)).arg(state()));
        return;
    }

    if (cmd.flags & RebuildWatchModel) {
        ++m_pendingWatchRequests;
        PENDING_DEBUG("   WATCH MODEL:" << cmd.command << "=>" << cmd.callbackName
                      << "INCREMENTS PENDING TO" << m_pendingWatchRequests);
    } else if (cmd.flags & RebuildBreakpointModel) {
        ++m_pendingBreakpointRequests;
        PENDING_DEBUG("   BRWAKPOINT MODEL:" << cmd.command << "=>" << cmd.callbackName
                      << "INCREMENTS PENDING TO" << m_pendingBreakpointRequests);
    } else {
        PENDING_DEBUG("   OTHER (IN):" << cmd.command << "=>" << cmd.callbackName
                      << "LEAVES PENDING WATCH AT" << m_pendingWatchRequests
                      << "LEAVES PENDING BREAKPOINT AT" << m_pendingBreakpointRequests);
    }

    if (!(cmd.flags & Discardable))
        ++m_nonDiscardableCount;

    // FIXME: clean up logic below
    if (cmd.flags & Immediate) {
        // This should always be sent.
        flushCommand(cmd);
    } else if ((cmd.flags & NeedsStop)
            || !m_commandsToRunOnTemporaryBreak.isEmpty()) {
        if (state() == InferiorStopOk || state() == InferiorUnrunnable
            || state() == InferiorSetupRequested || state() == EngineSetupOk
            || state() == InferiorShutdownRequested) {
            // Can be safely sent now.
            flushCommand(cmd);
        } else {
            // Queue the commands that we cannot send at once.
            showMessage(_("QUEUING COMMAND " + cmd.command));
            m_commandsToRunOnTemporaryBreak.append(cmd);
            if (state() == InferiorStopRequested) {
                if (cmd.flags & LosesChild) {
                    notifyInferiorIll();
                }
                showMessage(_("CHILD ALREADY BEING INTERRUPTED. STILL HOPING."));
                // Calling shutdown() here breaks all situations where two
                // NeedsStop commands are issued in quick succession.
            } else if (state() == InferiorRunOk) {
                showStatusMessage(tr("Stopping temporarily"), 1000);
                interruptInferiorTemporarily();
            } else {
                qDebug() << "ATTEMPTING TO QUEUE COMMAND "
                    << cmd.command << "IN INAPPROPRIATE STATE" << state();
            }
        }
    } else if (!cmd.command.isEmpty()) {
        flushCommand(cmd);
    }
}

void GdbEngine::flushQueuedCommands()
{
    showStatusMessage(tr("Processing queued commands"), 1000);
    while (!m_commandsToRunOnTemporaryBreak.isEmpty()) {
        GdbCommand cmd = m_commandsToRunOnTemporaryBreak.takeFirst();
        showMessage(_("RUNNING QUEUED COMMAND " + cmd.command + ' '
            + (cmd.callbackName ? cmd.callbackName : "<unnamed callback>")));
        flushCommand(cmd);
    }
}

void GdbEngine::flushCommand(const GdbCommand &cmd0)
{
    if (!stateAcceptsGdbCommands(state())) {
        showMessage(_(cmd0.command), LogInput);
        showMessage(_("GDB PROCESS ACCEPTS NO CMD IN STATE %1 ").arg(state()));
        return;
    }

    QTC_ASSERT(gdbProc()->state() == QProcess::Running, return;)

    const int token = ++currentToken();

    GdbCommand cmd = cmd0;
    cmd.postTime = QTime::currentTime();
    m_cookieForToken[token] = cmd;
    if (cmd.flags & ConsoleCommand)
        cmd.command = "-interpreter-exec console \"" + cmd.command + '"';
    cmd.command = QByteArray::number(token) + cmd.command;
    showMessage(_(cmd.command), LogInput);

    if (m_scheduledTestResponses.contains(token)) {
        // Fake response for test cases.
        QByteArray buffer = m_scheduledTestResponses.value(token);
        buffer.replace("@TOKEN@", QByteArray::number(token));
        m_scheduledTestResponses.remove(token);
        showMessage(_("FAKING TEST RESPONSE (TOKEN: %2, RESPONSE: '%3')")
            .arg(token).arg(_(buffer)));
        QMetaObject::invokeMethod(this, "handleResponse",
            Q_ARG(QByteArray, buffer));
    } else {
        m_gdbAdapter->write(cmd.command + "\r\n");

        // Start Watchdog.
        if (m_commandTimer.interval() <= 20000)
            m_commandTimer.setInterval(commandTimeoutTime());
        // The process can die for external reason between the "-gdb-exit" was
        // sent and a response could be retrieved. We don't want the watchdog
        // to bark in that case since the only possible outcome is a dead
        // process anyway.
        if (!cmd.command.endsWith("-gdb-exit"))
            m_commandTimer.start();

        //if (cmd.flags & LosesChild)
        //    notifyInferiorIll();
    }
}

int GdbEngine::commandTimeoutTime() const
{
    int time = debuggerCore()->action(GdbWatchdogTimeout)->value().toInt();
    return 1000 * qMax(40, time);
}

void GdbEngine::commandTimeout()
{
    QList<int> keys = m_cookieForToken.keys();
    qSort(keys);
    bool killIt = false;
    foreach (int key, keys) {
        const GdbCommand &cmd = m_cookieForToken.value(key);
        if (!(cmd.flags & NonCriticalResponse))
            killIt = true;
        QByteArray msg = QByteArray::number(key);
        msg += ": " + cmd.command + " => ";
        msg += cmd.callbackName ? cmd.callbackName : "<unnamed callback>";
        showMessage(_(msg));
    }
    if (killIt) {
        showMessage(_("TIMED OUT WAITING FOR GDB REPLY. COMMANDS STILL IN PROGRESS:"));
        int timeOut = m_commandTimer.interval();
        //m_commandTimer.stop();
        const QString msg = tr("The gdb process has not responded "
            "to a command within %n second(s). This could mean it is stuck "
            "in an endless loop or taking longer than expected to perform "
            "the operation.\nYou can choose between waiting "
            "longer or abort debugging.", 0, timeOut / 1000);
        QMessageBox *mb = showMessageBox(QMessageBox::Critical,
            tr("GDB not responding"), msg,
            QMessageBox::Ok | QMessageBox::Cancel);
        mb->button(QMessageBox::Cancel)->setText(tr("Give GDB more time"));
        mb->button(QMessageBox::Ok)->setText(tr("Stop debugging"));
        if (mb->exec() == QMessageBox::Ok) {
            showMessage(_("KILLING DEBUGGER AS REQUESTED BY USER"));
            // This is an undefined state, so we just pull the emergency brake.
            watchHandler()->endCycle();
            gdbProc()->kill();
        } else {
            showMessage(_("CONTINUE DEBUGGER AS REQUESTED BY USER"));
        }
    } else {
        showMessage(_("\nNON-CRITICAL TIMEOUT\n"));
    }
}

void GdbEngine::handleResultRecord(GdbResponse *response)
{
    //qDebug() << "TOKEN:" << response->token
    //    << " ACCEPTABLE:" << m_oldestAcceptableToken;
    //qDebug() << "\nRESULT" << response->token << response->toString();

    int token = response->token;
    if (token == -1)
        return;

    if (!m_cookieForToken.contains(token)) {
        // In theory this should not happen (rather the error should be
        // reported in the "first" response to the command) in practice it
        // does. We try to handle a few situations we are aware of gracefully.
        // Ideally, this code should not be present at all.
        showMessage(_("COOKIE FOR TOKEN %1 ALREADY EATEN (%2). "
                      "TWO RESPONSES FOR ONE COMMAND?").arg(token).
                    arg(QString::fromAscii(stateName(state()))));
        if (response->resultClass == GdbResultError) {
            QByteArray msg = response->data.findChild("msg").data();
            if (msg == "Cannot find new threads: generic error") {
                // Handle a case known to occur on Linux/gdb 6.8 when debugging moc
                // with helpers enabled. In this case we get a second response with
                // msg="Cannot find new threads: generic error"
                showMessage(_("APPLYING WORKAROUND #1"));
                showMessageBox(QMessageBox::Critical,
                    tr("Executable failed"), QString::fromLocal8Bit(msg));
                showStatusMessage(tr("Process failed to start"));
                //shutdown();
                notifyInferiorIll();
            } else if (msg == "\"finish\" not meaningful in the outermost frame.") {
                // Handle a case known to appear on GDB 6.4 symbianelf when
                // the stack is cut due to access to protected memory.
                //showMessage(_("APPLYING WORKAROUND #2"));
                notifyInferiorStopOk();
            } else if (msg.startsWith("Cannot find bounds of current function")) {
                // Happens when running "-exec-next" in a function for which
                // there is no debug information. Divert to "-exec-next-step"
                showMessage(_("APPLYING WORKAROUND #3"));
                notifyInferiorStopOk();
                executeNextI();
            } else if (msg.startsWith("Couldn't get registers: No such process.")) {
                // Happens on archer-tromey-python 6.8.50.20090910-cvs
                // There might to be a race between a process shutting down
                // and library load messages.
                showMessage(_("APPLYING WORKAROUND #4"));
                notifyInferiorStopOk();
                //notifyInferiorIll();
                //showStatusMessage(tr("Executable failed: %1")
                //    .arg(QString::fromLocal8Bit(msg)));
                //shutdown();
                //showMessageBox(QMessageBox::Critical,
                //    tr("Executable failed"), QString::fromLocal8Bit(msg));
            } else if (msg.contains("Cannot insert breakpoint")) {
                // For breakpoints set by address to non-existent addresses we
                // might get something like "6^error,msg="Warning:\nCannot insert
                // breakpoint 3.\nError accessing memory address 0x34592327:
                // Input/output error.\nCannot insert breakpoint 4.\nError
                // accessing memory address 0x34592335: Input/output error.\n".
                // This should not stop us from proceeding.
                // Most notably, that happens after a "6^running" and "*running"
                // We are probably sitting at _start and can't proceed as
                // long as the breakpoints are enabled.
                // FIXME: Should we silently disable the offending breakpoints?
                showMessage(_("APPLYING WORKAROUND #5"));
                showMessageBox(QMessageBox::Critical,
                    tr("Setting breakpoints failed"), QString::fromLocal8Bit(msg));
                QTC_CHECK(state() == InferiorRunOk);
                notifyInferiorSpontaneousStop();
                notifyEngineIll();
            } else {
                // Windows: Some DLL or some function not found. Report
                // the exception now in a box.
                if (msg.startsWith("During startup program exited with"))
                    notifyInferiorExited();
                QString logMsg;
                if (!m_lastWinException.isEmpty())
                    logMsg = m_lastWinException + QLatin1Char('\n');
                logMsg += QString::fromLocal8Bit(msg);
                showMessageBox(QMessageBox::Critical, tr("Executable Failed"), logMsg);
                showStatusMessage(tr("Executable failed: %1").arg(logMsg));
            }
        }
        return;
    }

    GdbCommand cmd = m_cookieForToken.take(token);
    if (debuggerCore()->boolSetting(LogTimeStamps)) {
        showMessage(_("Response time: %1: %2 s")
            .arg(_(cmd.command))
            .arg(cmd.postTime.msecsTo(QTime::currentTime()) / 1000.),
            LogTime);
    }

    if (response->token < m_oldestAcceptableToken && (cmd.flags & Discardable)) {
        //showMessage(_("### SKIPPING OLD RESULT") + response.toString());
        return;
    }

    response->cookie = cmd.cookie;

    bool isExpectedResult =
           (response->resultClass == GdbResultError) // Can always happen.
        || (response->resultClass == GdbResultRunning && (cmd.flags & RunRequest))
        || (response->resultClass == GdbResultExit && (cmd.flags & ExitRequest))
        || (response->resultClass == GdbResultDone);
        // GdbResultDone can almost "always" happen. Known examples are:
        //  (response->resultClass == GdbResultDone && cmd.command == "continue")
        // Happens with some incarnations of gdb 6.8 for "jump to line"
        //  (response->resultClass == GdbResultDone && cmd.command.startsWith("jump"))
        //  (response->resultClass == GdbResultDone && cmd.command.startsWith("detach"))
        // Happens when stepping finishes very quickly and issues *stopped and ^done
        // instead of ^running and *stopped
        //  (response->resultClass == GdbResultDone && (cmd.flags & RunRequest));

    if (!isExpectedResult) {
        const DebuggerStartParameters &sp = startParameters();
        if (sp.toolChainAbi.os() == Abi::WindowsOS
            && cmd.command.startsWith("attach")
            && (sp.startMode == AttachExternal || sp.useTerminal))
        {
            // Ignore spurious 'running' responses to 'attach'.
        } else {
            QByteArray rsp = GdbResponse::stringFromResultClass(response->resultClass);
            rsp = "UNEXPECTED RESPONSE '" + rsp + "' TO COMMAND '" + cmd.command + "'";
            qWarning() << rsp << " AT " __FILE__ ":" STRINGIFY(__LINE__);
            showMessage(_(rsp));
        }
    }

    if (!(cmd.flags & Discardable))
        --m_nonDiscardableCount;

    if (cmd.callback)
        (this->*cmd.callback)(*response);
    else if (cmd.adapterCallback)
        (m_gdbAdapter->*cmd.adapterCallback)(*response);

    if (cmd.flags & RebuildWatchModel) {
        --m_pendingWatchRequests;
        PENDING_DEBUG("   WATCH" << cmd.command << "=>" << cmd.callbackName
                      << "DECREMENTS PENDING WATCH TO" << m_pendingWatchRequests);
        if (m_pendingWatchRequests <= 0) {
            PENDING_DEBUG("\n\n ... AND TRIGGERS WATCH MODEL UPDATE\n");
            rebuildWatchModel();
        }
    } else if (cmd.flags & RebuildBreakpointModel) {
        --m_pendingBreakpointRequests;
        PENDING_DEBUG("   BREAKPOINT" << cmd.command << "=>" << cmd.callbackName
                      << "DECREMENTS PENDING TO" << m_pendingWatchRequests);
        if (m_pendingBreakpointRequests <= 0) {
            PENDING_DEBUG("\n\n ... AND TRIGGERS BREAKPOINT MODEL UPDATE\n");
            attemptBreakpointSynchronization();
        }
    } else {
        PENDING_DEBUG("   OTHER (OUT):" << cmd.command << "=>" << cmd.callbackName
                      << "LEAVES PENDING WATCH AT" << m_pendingWatchRequests
                      << "LEAVES PENDING BREAKPOINT AT" << m_pendingBreakpointRequests);
    }

    // Commands were queued, but we were in RunningRequested state, so the interrupt
    // was postponed.
    // This is done after the command callbacks so the running-requesting commands
    // can assert on the right state.
    if (state() == InferiorRunOk && !m_commandsToRunOnTemporaryBreak.isEmpty())
        interruptInferiorTemporarily();

    // Continue only if there are no commands wire anymore, so this will
    // be fully synchronous.
    // This is somewhat inefficient, as it makes the last command synchronous.
    // An optimization would be requesting the continue immediately when the
    // event loop is entered, and let individual commands have a flag to suppress
    // that behavior.
    if (m_commandsDoneCallback && m_cookieForToken.isEmpty()) {
        showMessage(_("ALL COMMANDS DONE; INVOKING CALLBACK"));
        CommandsDoneCallback cont = m_commandsDoneCallback;
        m_commandsDoneCallback = 0;
        (this->*cont)();
    } else {
        PENDING_DEBUG("MISSING TOKENS: " << m_cookieForToken.keys());
    }

    if (m_cookieForToken.isEmpty())
        m_commandTimer.stop();
}

bool GdbEngine::acceptsDebuggerCommands() const
{
    return state() == InferiorStopOk
        || state() == InferiorUnrunnable;
}

void GdbEngine::executeDebuggerCommand(const QString &command)
{
    QTC_CHECK(acceptsDebuggerCommands());
    GdbCommand cmd;
    cmd.command = command.toLatin1();
    flushCommand(cmd);
}

// This is called from CoreAdapter and AttachAdapter.
void GdbEngine::updateAll()
{
    if (hasPython())
        updateAllPython();
    else
        updateAllClassic();
}

void GdbEngine::handleQuerySources(const GdbResponse &response)
{
    m_sourcesListUpdating = false;
    m_sourcesListOutdated = false;
    if (response.resultClass == GdbResultDone) {
        QMap<QString, QString> oldShortToFull = m_shortToFullName;
        m_shortToFullName.clear();
        m_fullToShortName.clear();
        // "^done,files=[{file="../../../../bin/dumper/dumper.cpp",
        // fullname="/data5/dev/ide/main/bin/dumper/dumper.cpp"},
        GdbMi files = response.data.findChild("files");
        foreach (const GdbMi &item, files.children()) {
            GdbMi fullName = item.findChild("fullname");
            GdbMi fileName = item.findChild("file");
            QString file = QString::fromLocal8Bit(fileName.data());
            if (fullName.isValid()) {
                QString full = cleanupFullName(QString::fromLocal8Bit(fullName.data()));
                m_shortToFullName[file] = full;
                m_fullToShortName[full] = file;
            } else if (fileName.isValid()) {
                m_shortToFullName[file] = tr("<unknown>");
            }
        }
        if (m_shortToFullName != oldShortToFull)
            sourceFilesHandler()->setSourceFiles(m_shortToFullName);
    }
}

void GdbEngine::handleExecuteJumpToLine(const GdbResponse &response)
{
    if (response.resultClass == GdbResultRunning) {
        doNotifyInferiorRunOk();
        // All is fine. Waiting for the temporary breakpoint to be hit.
    } else if (response.resultClass == GdbResultDone) {
        // This happens on old gdb. Trigger the effect of a '*stopped'.
        showStatusMessage(tr("Jumped. Stopped"));
        notifyInferiorSpontaneousStop();
        handleStop1(response);
    }
}

void GdbEngine::handleExecuteRunToLine(const GdbResponse &response)
{
    if (response.resultClass == GdbResultRunning) {
        doNotifyInferiorRunOk();
        // All is fine. Waiting for the temporary breakpoint to be hit.
    } else if (response.resultClass == GdbResultDone) {
        // This happens on old gdb (Mac). gdb is not stopped yet,
        // but merely accepted the continue.
        // >&"continue\n"
        // >~"Continuing.\n"
        //>~"testArray () at ../simple/app.cpp:241\n"
        //>~"241\t    s[1] = \"b\";\n"
        //>122^done
        showStatusMessage(tr("Target line hit. Stopped"));
        notifyInferiorRunOk();
    }
}

static bool isExitedReason(const QByteArray &reason)
{
    return reason == "exited-normally"   // inferior exited normally
        || reason == "exited-signalled"  // inferior exited because of a signal
        //|| reason == "signal-received" // inferior received signal
        || reason == "exited";           // inferior exited
}

#if 0
void GdbEngine::handleAqcuiredInferior()
{
    // Reverse debugging. FIXME: Should only be used when available.
    //if (debuggerCore()->boolSetting(EnableReverseDebugging))
    //    postCommand("target response");

    tryLoadDebuggingHelpers();

#    ifndef Q_OS_MAC
    // intentionally after tryLoadDebuggingHelpers(),
    // otherwise we'd interrupt solib loading.
    if (debuggerCore()->boolSetting(AllPluginBreakpoints)) {
        postCommand("set auto-solib-add on");
        postCommand("set stop-on-solib-events 0");
        postCommand("sharedlibrary .*");
    } else if (debuggerCore()->boolSetting(SelectedPluginBreakpoints)) {
        postCommand("set auto-solib-add on");
        postCommand("set stop-on-solib-events 1");
        postCommand("sharedlibrary "
          + theDebuggerStringSetting(SelectedPluginBreakpointsPattern));
    } else if (debuggerCore()->boolSetting(NoPluginBreakpoints)) {
        // should be like that already
        postCommand("set auto-solib-add off");
        postCommand("set stop-on-solib-events 0");
    }
#    endif

    // It's nicer to see a bit of the world we live in.
    reloadModulesInternal();
}
#endif

void GdbEngine::handleStopResponse(const GdbMi &data)
{
    // This is gdb 7+'s initial *stopped in response to attach.
    // For consistency, we just discard it.
    m_actingOnExpectedStop = false;
    if (state() == InferiorSetupRequested)
        return;

    if (isDying()) {
        notifyInferiorStopOk();
        return;
    }

    const QByteArray reason = data.findChild("reason").data();

    if (isExitedReason(reason)) {
        //   // The user triggered a stop, but meanwhile the app simply exited ...
        //    QTC_ASSERT(state() == InferiorStopRequested
        //            /*|| state() == InferiorStopRequested_Kill*/,
        //               qDebug() << state());
        QString msg;
        if (reason == "exited") {
            msg = tr("Application exited with exit code %1")
                .arg(_(data.findChild("exit-code").toString()));
        } else if (reason == "exited-signalled" || reason == "signal-received") {
            msg = tr("Application exited after receiving signal %1")
                .arg(_(data.findChild("signal-name").toString()));
        } else {
            msg = tr("Application exited normally");
        }
        showStatusMessage(msg);
        notifyInferiorExited();
        return;
    }

    BreakpointResponseId rid(data.findChild("bkptno").data());
    const GdbMi frame = data.findChild("frame");

    const int lineNumber = frame.findChild("line").data().toInt();
    QString fullName = QString::fromUtf8(frame.findChild("fullname").data());

    if (fullName.isEmpty())
        fullName = QString::fromUtf8(frame.findChild("file").data());

    if (rid.isValid() && frame.isValid()
            && !isQmlStepBreakpoint(rid)
            && !isQFatalBreakpoint(rid)) {
        // Use opportunity to update the breakpoint marker position.
        BreakHandler *handler = breakHandler();
        //qDebug() << " PROBLEM: " << m_qmlBreakpointNumbers << rid
        //    << isQmlStepBreakpoint1(rid)
        //    << isQmlStepBreakpoint2(rid)
        BreakpointModelId id = handler->findBreakpointByResponseId(rid);
        const BreakpointResponse &response = handler->response(id);
        QString fileName = response.fileName;
        if (fileName.isEmpty())
            fileName = handler->fileName(id);
        if (fileName.isEmpty())
            fileName = fullName;
        if (!fileName.isEmpty())
            handler->setMarkerFileAndLine(id, fileName, lineNumber);
    }

    //qDebug() << "BP " << rid << data.toString();
    // Quickly set the location marker.
    if (lineNumber && !debuggerCore()->boolSetting(OperateByInstruction)
            && QFileInfo(fullName).exists()
            && !isQmlStepBreakpoint(rid)
            && !isQFatalBreakpoint(rid))
        gotoLocation(Location(fullName, lineNumber));

    if (!m_commandsToRunOnTemporaryBreak.isEmpty()) {
        QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state())
        m_actingOnExpectedStop = true;
        notifyInferiorStopOk();
        flushQueuedCommands();
        if (state() == InferiorStopOk) {
            QTC_CHECK(m_commandsDoneCallback == 0);
            m_commandsDoneCallback = &GdbEngine::autoContinueInferior;
        } else {
            QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state())
        }
        return;
    }

    if (state() == InferiorRunOk) {
        // Stop triggered by a breakpoint or otherwise not directly
        // initiated by the user.
        notifyInferiorSpontaneousStop();
    } else if (state() == InferiorRunRequested) {
        // Stop triggered by something like "-exec-step\n"
        //  "&"Cannot access memory at address 0xbfffedd4\n"
        // or, on S40,
        //  "*running,thread-id="30""
        //  "&"Warning:\n""
        //  "&"Cannot insert breakpoint -33.\n"
        //  "&"Error accessing memory address 0x11673fc: Input/output error.\n""
        // In this case a proper response 94^error,msg="" will follow and
        // be handled in the result handler.
        // -- or --
        // *stopped arriving earlier than ^done response to an -exec-step
        doNotifyInferiorRunOk();
        notifyInferiorSpontaneousStop();
    } else if (state() == InferiorStopOk && isQmlStepBreakpoint2(rid)) {
        // That's expected.
    } else {
        QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state());
        m_actingOnExpectedStop = true;
        notifyInferiorStopOk();
    }

    if (isQmlStepBreakpoint1(rid))
        return;

    handleStop0(data);
}

static QByteArray stopSignal(Abi abi)
{
    return (abi.os() == Abi::WindowsOS) ? QByteArray("SIGTRAP") : QByteArray("SIGINT");
}

void GdbEngine::handleStop0(const GdbMi &data)
{
#if 0 // See http://vladimir_prus.blogspot.com/2007/12/debugger-stories-pending-breakpoints.html
    // Due to LD_PRELOADing the dumpers, these events can occur even before
    // reaching the entry point. So handle it before the entry point hacks below.
    if (reason.isEmpty() && m_gdbVersion < 70000 && !m_isMacGdb) {
        // On Linux it reports "Stopped due to shared library event\n", but
        // on Windows it simply forgets about it. Thus, we identify the response
        // based on it having no frame information.
        if (!data.findChild("frame").isValid()) {
            invalidateSourcesList();
            // Each stop causes a roundtrip and button flicker, so prevent
            // a flood of useless stops. Will be automatically re-enabled.
            postCommand("set stop-on-solib-events 0");
#if 0
            // The related code (handleAqcuiredInferior()) is disabled as well.
            if (debuggerCore()->boolSetting(SelectedPluginBreakpoints)) {
                QString dataStr = _(data.toString());
                showMessage(_("SHARED LIBRARY EVENT: ") + dataStr);
                QString pat = theDebuggerStringSetting(SelectedPluginBreakpointsPattern);
                showMessage(_("PATTERN: ") + pat);
                postCommand("sharedlibrary " + pat.toLocal8Bit());
                showStatusMessage(tr("Loading %1...").arg(dataStr));
            }
#endif
            continueInferiorInternal();
            return;
        }
    }
#endif

    const GdbMi frame = data.findChild("frame");
    const QByteArray reason = data.findChild("reason").data();

    // This was seen on XP after removing a breakpoint while running
    //  >945*stopped,reason="signal-received",signal-name="SIGTRAP",
    //  signal-meaning="Trace/breakpoint trap",thread-id="2",
    //  frame={addr="0x7c91120f",func="ntdll!DbgUiConnectToDbg",
    //  args=[],from="C:\\WINDOWS\\system32\\ntdll.dll"}
    // also seen on gdb 6.8-symbianelf without qXfer:libraries:read+;
    // FIXME: remote.c parses "loaded" reply. It should be turning
    // that into a TARGET_WAITKIND_LOADED. Does it?
    // The bandaid here has the problem that it breaks for 'next' over a
    // statement that indirectly loads shared libraries
    // 6.1.2010: Breaks interrupting inferiors, disabled:
    // if (reason == "signal-received"
    //      && data.findChild("signal-name").data() == "SIGTRAP") {
    //    continueInferiorInternal();
    //    return;
    // }

    // Jump over well-known frames.
    static int stepCounter = 0;
    if (debuggerCore()->boolSetting(SkipKnownFrames)) {
        if (reason == "end-stepping-range" || reason == "function-finished") {
            //showMessage(frame.toString());
            QString funcName = _(frame.findChild("func").data());
            QString fileName = QString::fromLocal8Bit(frame.findChild("file").data());
            if (isLeavableFunction(funcName, fileName)) {
                //showMessage(_("LEAVING ") + funcName);
                ++stepCounter;
                executeStepOut();
                return;
            }
            if (isSkippableFunction(funcName, fileName)) {
                //showMessage(_("SKIPPING ") + funcName);
                ++stepCounter;
                executeStep();
                return;
            }
            //if (stepCounter)
            //    qDebug() << "STEPCOUNTER:" << stepCounter;
            stepCounter = 0;
        }
    }

    // Show return value if possible, usually with reason "function-finished".
    // *stopped,reason="function-finished",frame={addr="0x080556da",
    // func="testReturnValue",args=[],file="/../app.cpp",
    // fullname="/../app.cpp",line="1611"},gdb-result-var="$1",
    // return-value="{d = 0x808d998}",thread-id="1",stopped-threads="all",
    // core="1"
    GdbMi resultVar = data.findChild("gdb-result-var");
    if (resultVar.isValid())
        m_resultVarName = resultVar.data();
    else
        m_resultVarName.clear();

    bool initHelpers = m_debuggingHelperState == DebuggingHelperUninitialized
                       || m_debuggingHelperState == DebuggingHelperLoadTried;
    // Don't load helpers on stops triggered by signals unless it's
    // an intentional trap.
    if (initHelpers
            && m_gdbAdapter->dumperHandling() != AbstractGdbAdapter::DumperLoadedByGdbPreload
            && reason == "signal-received") {
        const QByteArray name = data.findChild("signal-name").data();
        const DebuggerStartParameters &sp = startParameters();
        if (name != stopSignal(sp.toolChainAbi))
            initHelpers = false;
    }
    if (isSynchronous())
        initHelpers = false;
    if (initHelpers) {
        tryLoadDebuggingHelpersClassic();
        QVariant var = QVariant::fromValue<GdbMi>(data);
        postCommand("p 4", CB(handleStop1), var);  // dummy
    } else {
        handleStop1(data);
    }
    // Dumper loading is sequenced, as otherwise the display functions
    // will start requesting data without knowing that dumpers are available.
}

void GdbEngine::handleStop1(const GdbResponse &response)
{
    handleStop1(response.cookie.value<GdbMi>());
}

void GdbEngine::handleStop1(const GdbMi &data)
{
    if (isDying()) {
        qDebug() << "HANDLING STOP WHILE DYING";
        notifyInferiorStopOk();
        return;
    }

    // A user initiated stop looks like the following. Note that there is
    // this extra "stopper thread" created and "properly" reported by gdb.
    //
    // dNOTE: INFERIOR RUN OK
    // dState changed from InferiorRunRequested(10) to InferiorRunOk(11).
    // >*running,thread-id="all"
    // >=thread-exited,id="11",group-id="i1"
    // sThread 11 in group i1 exited
    // dState changed from InferiorRunOk(11) to InferiorStopRequested(13).
    // dCALL: INTERRUPT INFERIOR
    // sStop requested...
    // dTRYING TO INTERRUPT INFERIOR
    // >=thread-created,id="12",group-id="i1"
    // sThread 12 created
    // >~"[New Thread 8576.0x1154]\n"
    // s[New Thread 8576.0x1154]
    // >*running,thread-id="all"
    // >~"[Switching to Thread 8576.0x1154]\n"
    // >*stopped,reason="signal-received",signal-name="SIGTRAP",
    // signal-meaning="Trace/breakpointtrap",frame={addr="0x7c90120f",func=
    // "ntdll!DbgUiConnectToDbg",args=[],from="C:\\WINDOWS\\system32\\ntdll.dll"},
    // thread-id="12",stopped-threads="all"
    // dNOTE: INFERIOR STOP OK
    // dState changed from InferiorStopRequested(13) to InferiorStopOk(14).

    const QByteArray reason = data.findChild("reason").data();
    const DebuggerStartParameters &sp = startParameters();
    const Abi abi = sp.toolChainAbi;

    bool isStopperThread = false;

    if (abi.os() == Abi::WindowsOS
            && sp.useTerminal
            && reason == "signal-received"
            && data.findChild("signal-name").data() == "SIGTRAP")
    {
        if (!m_actingOnExpectedStop) {
            // Ignore signals from command line start up traps.
            showMessage(_("INTERNAL CONTINUE"), LogMisc);
            continueInferiorInternal();
            return;
        }
        // This is the stopper thread. That also means that the
        // reported thread is not the one we'd like to expose
        // to the user.
        isStopperThread = true;
    }

    // This is for display only.
    if (m_modulesListOutdated)
        reloadModulesInternal();

    if (m_breakListOutdated) {
        reloadBreakListInternal();
    } else {
        // Older gdb versions do not produce "library loaded" messages
        // so the breakpoint update is not triggered.
        if (m_gdbVersion < 70000 && !m_isMacGdb && breakHandler()->size() > 0)
            reloadBreakListInternal();
    }

    if (reason == "watchpoint-trigger") {
        // *stopped,reason="watchpoint-trigger",wpt={number="2",exp="*0xbfffed40"},
        // value={old="1",new="0"},frame={addr="0x00451e1b",
        // func="QScopedPointer",args=[{name="this",value="0xbfffed40"},
        // {name="p",value="0x0"}],file="x.h",fullname="/home/.../x.h",line="95"},
        // thread-id="1",stopped-threads="all",core="2"
        const GdbMi wpt = data.findChild("wpt");
        const BreakpointResponseId rid(wpt.findChild("number").data());
        const BreakpointModelId id = breakHandler()->findBreakpointByResponseId(rid);
        const quint64 bpAddress = wpt.findChild("exp").data().mid(1).toULongLong(0, 0);
        QString msg;
        if (id && breakHandler()->type(id) == WatchpointAtExpression)
            msg = msgWatchpointByExpressionTriggered(id, rid.majorPart(),
                breakHandler()->expression(id));
        if (id && breakHandler()->type(id) == WatchpointAtAddress)
            msg = msgWatchpointByAddressTriggered(id, rid.majorPart(), bpAddress);
        GdbMi value = data.findChild("value");
        GdbMi oldValue = value.findChild("old");
        GdbMi newValue = value.findChild("new");
        if (oldValue.isValid() && newValue.isValid()) {
            msg += QLatin1Char(' ');
            msg += tr("Value changed from %1 to %2.")
                .arg(_(oldValue.data())).arg(_(newValue.data()));
        }
        showStatusMessage(msg);
    } else if (reason == "breakpoint-hit") {
        GdbMi gNumber = data.findChild("bkptno"); // 'number' or 'bkptno'?
        if (!gNumber.isValid())
            gNumber = data.findChild("number");
        const BreakpointResponseId rid(gNumber.data());
        const QByteArray threadId = data.findChild("thread-id").data();
        const BreakpointModelId id = breakHandler()->findBreakpointByResponseId(rid);
        showStatusMessage(msgBreakpointTriggered(id, rid.majorPart(), _(threadId)));
        m_currentThread = threadId;
    } else {
        QString reasontr = msgStopped(_(reason));
        if (reason == "signal-received") {
            QByteArray name = data.findChild("signal-name").data();
            QByteArray meaning = data.findChild("signal-meaning").data();
            // Ignore these as they are showing up regularly when
            // stopping debugging.
            if (name == stopSignal(sp.toolChainAbi)) {
                showMessage(_(name + " CONSIDERED HARMLESS. CONTINUING."));
            } else {
                showMessage(_("HANDLING SIGNAL " + name));
                if (debuggerCore()->boolSetting(UseMessageBoxForSignals)
                        && !isStopperThread)
                    showStoppedBySignalMessageBox(_(meaning), _(name));
                if (!name.isEmpty() && !meaning.isEmpty())
                    reasontr = msgStoppedBySignal(_(meaning), _(name));
            }
        }

        if (reason.isEmpty())
            showStatusMessage(msgStopped());
        else
            showStatusMessage(reasontr);
    }

    // Let the event loop run before deciding whether to update the stack.
    m_stackNeeded = true; // setTokenBarrier() might reset this.
    if (isStopperThread)
        m_currentThreadId = 0;
    else
        m_currentThreadId = data.findChild("thread-id").data().toInt();
    QTimer::singleShot(0, this, SLOT(handleStop2()));
}

void GdbEngine::handleStop2()
{
    // We are already continuing.
    if (!m_stackNeeded)
        return;

    reloadStack(false); // Will trigger register reload.

    if (supportsThreads()) {
        if (m_gdbAdapter->isCodaAdapter()) {
            m_gdbAdapter->codaReloadThreads();
        } else if (m_isMacGdb) {
            postCommand("-thread-list-ids", Discardable, CB(handleThreadListIds));
        } else {
            // This is only available in gdb 7.1+.
            postCommand("-thread-info", Discardable, CB(handleThreadInfo));
        }
    }

}

void GdbEngine::handleInfoProc(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        static QRegExp re(_("\\bprocess ([0-9]+)\n"));
        QTC_ASSERT(re.isValid(), return);
        if (re.indexIn(_(response.consoleStreamOutput)) != -1)
            maybeHandleInferiorPidChanged(re.cap(1));
    }
}

void GdbEngine::handleShowVersion(const GdbResponse &response)
{
    showMessage(_("PARSING VERSION: " + response.toString()));
    if (response.resultClass == GdbResultDone) {
        m_gdbVersion = 100;
        m_gdbBuildVersion = -1;
        m_isMacGdb = false;
        QString msg = QString::fromLocal8Bit(response.consoleStreamOutput);
        extractGdbVersion(msg,
              &m_gdbVersion, &m_gdbBuildVersion, &m_isMacGdb);
        if (m_gdbVersion > 60500 && m_gdbVersion < 200000)
            showMessage(_("SUPPORTED GDB VERSION ") + msg);
        else
            showMessage(_("UNSUPPORTED GDB VERSION ") + msg);

        showMessage(_("USING GDB VERSION: %1, BUILD: %2%3").arg(m_gdbVersion)
            .arg(m_gdbBuildVersion).arg(_(m_isMacGdb ? " (APPLE)" : "")));

        if (m_gdbVersion > 70300)
            m_hasBreakpointNotifications = true;
    }
}

void GdbEngine::handleListFeatures(const GdbResponse &response)
{
    showMessage(_("FEATURES: " + response.toString()));
}

void GdbEngine::handleHasPython(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        m_hasPython = true;
        GdbMi data;
        data.fromStringMultiple(response.consoleStreamOutput);
        const GdbMi dumpers = data.findChild("dumpers");
        foreach (const GdbMi &dumper, dumpers.children()) {
            QByteArray type = dumper.findChild("type").data();
            QStringList formats(tr("Raw structure"));
            foreach (const QByteArray &format,
                     dumper.findChild("formats").data().split(',')) {
                if (format == "Normal")
                    formats.append(tr("Normal"));
                else if (format == "Displayed")
                    formats.append(tr("Displayed"));
                else if (!format.isEmpty())
                    formats.append(_(format));
            }
            watchHandler()->addTypeFormats(type, formats);
        }
        const GdbMi hasInferiorThreadList = data.findChild("hasInferiorThreadList");
        m_hasInferiorThreadList = (hasInferiorThreadList.data().toInt() != 0);
    } else {
        pythonDumpersFailed();
    }
}

void GdbEngine::pythonDumpersFailed()
{
    m_hasPython = false;
    const DebuggerStartParameters &sp = startParameters();
    if (m_gdbAdapter->dumperHandling() == AbstractGdbAdapter::DumperLoadedByGdbPreload
            && checkDebuggingHelpersClassic()) {
        QByteArray cmd = "set environment ";
        if (sp.toolChainAbi.os() == Abi::MacOS)
            cmd += "DYLD_INSERT_LIBRARIES";
        else
            cmd += "LD_PRELOAD";
        cmd += ' ';
        if (sp.startMode == StartRemoteGdb)
            cmd += sp.remoteDumperLib;
        else
            cmd += qtDumperLibraryName().toLocal8Bit();
        postCommand(cmd);
        m_debuggingHelperState = DebuggingHelperLoadTried;
    }
}

void GdbEngine::showExecutionError(const QString &message)
{
    showMessageBox(QMessageBox::Critical, tr("Execution Error"),
       tr("Cannot continue debugged process:\n") + message);
}

void GdbEngine::handleExecuteContinue(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorRunRequested, qDebug() << state());
    if (response.resultClass == GdbResultRunning) {
        doNotifyInferiorRunOk();
        return;
    }
    QByteArray msg = response.data.findChild("msg").data();
    if (msg.startsWith("Cannot find bounds of current function")) {
        notifyInferiorRunFailed();
        if (isDying())
            return;
        if (!m_commandsToRunOnTemporaryBreak.isEmpty())
            flushQueuedCommands();
        QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
        showStatusMessage(tr("Stopped."), 5000);
        reloadStack(true);
    } else if (msg.startsWith("Cannot access memory at address")) {
        // Happens on single step on ARM prolog and epilogs.
    } else if (msg.startsWith("\"finish\" not meaningful in the outermost frame")) {
        notifyInferiorRunFailed();
        if (isDying())
            return;
        QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
        // FIXME: Fix translation in master.
        showStatusMessage(QString::fromLocal8Bit(msg), 5000);
        gotoLocation(stackHandler()->currentFrame());
    } else {
        showExecutionError(QString::fromLocal8Bit(msg));
        notifyInferiorIll();
    }
}

QString GdbEngine::fullName(const QString &fileName)
{
    if (fileName.isEmpty())
        return QString();
    //QTC_ASSERT(!m_sourcesListOutdated, /* */)
    QTC_ASSERT(!m_sourcesListUpdating, /* */)
    return m_shortToFullName.value(fileName, QString());
}

QString GdbEngine::cleanupFullName(const QString &fileName)
{
    QString cleanFilePath = fileName;

    // Gdb running on windows often delivers "fullnames" which
    // (a) have no drive letter and (b) are not normalized.
    if (Abi::hostAbi().os() == Abi::WindowsOS) {
        QTC_ASSERT(!fileName.isEmpty(), return QString())
        QFileInfo fi(fileName);
        if (fi.isReadable())
            cleanFilePath = QDir::cleanPath(fi.absoluteFilePath());
    }
    if (startMode() == StartRemoteGdb) {
        cleanFilePath.replace(0, startParameters().remoteMountPoint.length(),
            startParameters().localMountDir);
    }
    return cleanFilePath;
}

void GdbEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    m_commandsToRunOnTemporaryBreak.clear();
    m_gdbAdapter->shutdownInferior();
}

void GdbEngine::defaultInferiorShutdown(const char *cmd)
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    postCommand(cmd, NeedsStop | LosesChild, CB(handleInferiorShutdown));
}

void GdbEngine::handleInferiorShutdown(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        notifyInferiorShutdownOk();
        return;
    }
    QByteArray ba = response.data.findChild("msg").data();
    if (ba.contains(": No such file or directory.")) {
        // This happens when someone removed the binary behind our back.
        // It is not really an error from a user's point of view.
        showMessage(_("NOTE: " + ba));
        notifyInferiorShutdownOk();
        return;
    }
    showMessageBox(QMessageBox::Critical,
        tr("Failed to shut down application"),
        AbstractGdbAdapter::msgInferiorStopFailed(QString::fromLocal8Bit(ba)));
    notifyInferiorShutdownFailed();
}

void GdbEngine::shutdownEngine()
{
    m_gdbAdapter->shutdownAdapter();
}

void GdbEngine::notifyAdapterShutdownFailed()
{
    showMessage(_("ADAPTER SHUTDOWN FAILED"));
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    notifyEngineShutdownFailed();
}

void GdbEngine::notifyAdapterShutdownOk()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    showMessage(_("INITIATE GDBENGINE SHUTDOWN IN STATE %1, PROC: %2")
        .arg(lastGoodState()).arg(gdbProc()->state()));
    m_commandsDoneCallback = 0;
    switch (gdbProc()->state()) {
    case QProcess::Running:
        postCommand("-gdb-exit", GdbEngine::ExitRequest, CB(handleGdbExit));
        break;
    case QProcess::NotRunning:
        // Cannot find executable.
        notifyEngineShutdownOk();
        break;
    case QProcess::Starting:
        showMessage(_("GDB NOT REALLY RUNNING; KILLING IT"));
        gdbProc()->kill();
        notifyEngineShutdownFailed();
        break;
    }
}

void GdbEngine::handleGdbExit(const GdbResponse &response)
{
    if (response.resultClass == GdbResultExit) {
        showMessage(_("GDB CLAIMS EXIT; WAITING"));
        // Don't set state here, this will be handled in handleGdbFinished()
        //notifyEngineShutdownOk();
    } else {
        QString msg = m_gdbAdapter->msgGdbStopFailed(
            QString::fromLocal8Bit(response.data.findChild("msg").data()));
        qDebug() << (_("GDB WON'T EXIT (%1); KILLING IT").arg(msg));
        showMessage(_("GDB WON'T EXIT (%1); KILLING IT").arg(msg));
        gdbProc()->kill();
    }
}

void GdbEngine::detachDebugger()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    QTC_ASSERT(startMode() != AttachCore, qDebug() << startMode());
    postCommand("detach", GdbEngine::ExitRequest, CB(handleDetach));
}

void GdbEngine::handleDetach(const GdbResponse &response)
{
    Q_UNUSED(response);
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    notifyInferiorExited();
}

int GdbEngine::currentFrame() const
{
    return stackHandler()->currentIndex();
}

QString msgNoGdbBinaryForToolChain(const Abi &tc)
{
    return GdbEngine::tr("There is no gdb binary available for binaries in format '%1'")
        .arg(tc.toString());
}

AbstractGdbAdapter *GdbEngine::createAdapter()
{
    const DebuggerStartParameters &sp = startParameters();
    if (sp.toolChainAbi.os() == Abi::SymbianOS) {
        // FIXME: 1 of 3 testing hacks.
        if (sp.debugClient == DebuggerStartParameters::SymbianDebugClientCoda)
            return new CodaGdbAdapter(this);
        return 0;
    }

    switch (sp.startMode) {
    case AttachCore:
        return new CoreGdbAdapter(this);
    case AttachToRemoteServer:
        return new RemoteGdbServerAdapter(this);
    case StartRemoteGdb:
        return new RemotePlainGdbAdapter(this);
    case AttachExternal:
        return new AttachGdbAdapter(this);
    default:
        if (sp.useTerminal)
            return new TermGdbAdapter(this);
        return new LocalPlainGdbAdapter(this);
    }
}

void GdbEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    QTC_CHECK(m_debuggingHelperState == DebuggingHelperUninitialized);

    if (m_gdbAdapter->dumperHandling() != AbstractGdbAdapter::DumperNotAvailable) {
        connect(debuggerCore()->action(UseDebuggingHelpers),
            SIGNAL(valueChanged(QVariant)),
            SLOT(setUseDebuggingHelpers(QVariant)));
    }

    QTC_CHECK(state() == EngineSetupRequested);
    m_gdbAdapter->startAdapter();
}

unsigned GdbEngine::debuggerCapabilities() const
{
    unsigned caps = ReverseSteppingCapability
        | AutoDerefPointersCapability
        | DisassemblerCapability
        | RegisterCapability
        | ShowMemoryCapability
        | JumpToLineCapability
        | ReloadModuleCapability
        | ReloadModuleSymbolsCapability
        | BreakOnThrowAndCatchCapability
        | BreakConditionCapability
        | TracePointCapability
        | ReturnFromFunctionCapability
        | CreateFullBacktraceCapability
        | WatchpointByAddressCapability
        | WatchpointByExpressionCapability
        | AddWatcherCapability
        | WatchWidgetsCapability
        | ShowModuleSymbolsCapability
        | CatchCapability
        | OperateByInstructionCapability
        | RunToLineCapability
        | MemoryAddressCapability;

    if (startParameters().startMode == AttachCore)
        return caps;

    // FIXME: Remove in case we have gdb 7.x on Mac.
    if (startParameters().toolChainAbi.os() == Abi::MacOS)
        return caps;

    return caps | SnapshotCapability;
}


void GdbEngine::continueInferiorInternal()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    notifyInferiorRunRequested();
    showStatusMessage(tr("Running requested..."), 5000);
    QTC_ASSERT(state() == InferiorRunRequested, qDebug() << state());
    postCommand("-exec-continue", RunRequest, CB(handleExecuteContinue));
}

void GdbEngine::doNotifyInferiorRunOk()
{
    clearToolTip();
    notifyInferiorRunOk();
}

void GdbEngine::autoContinueInferior()
{
    resetLocation();
    continueInferiorInternal();
    showStatusMessage(tr("Continuing after temporary stop..."), 1000);
}

void GdbEngine::continueInferior()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    continueInferiorInternal();
}

void GdbEngine::executeStep()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Step requested..."), 5000);
    if (m_gdbAdapter->isCodaAdapter() && stackHandler()->stackSize() > 0)
        postCommand("sal step,0x" + QByteArray::number(stackHandler()->topAddress(), 16));
    if (isReverseDebugging()) {
        postCommand("reverse-step", RunRequest, CB(handleExecuteStep));
    } else {
        postCommand("-exec-step", RunRequest, CB(handleExecuteStep));
    }
}

void GdbEngine::handleExecuteStep(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        // Step was finishing too quick, and a '*stopped' messages should
        // have preceded it, so just ignore this result.
        QTC_CHECK(state() == InferiorStopOk);
        return;
    }
    QTC_ASSERT(state() == InferiorRunRequested, qDebug() << state());
    if (response.resultClass == GdbResultRunning) {
        doNotifyInferiorRunOk();
        return;
    }
    QByteArray msg = response.data.findChild("msg").data();
    if (msg.startsWith("Cannot find bounds of current function")
            || msg.contains("Error accessing memory address")
            || msg.startsWith("Cannot access memory at address")) {
        // On S40: "40^error,msg="Warning:\nCannot insert breakpoint -39.\n"
        //" Error accessing memory address 0x11673fc: Input/output error.\n"
        notifyInferiorRunFailed();
        if (isDying())
            return;
        if (!m_commandsToRunOnTemporaryBreak.isEmpty())
            flushQueuedCommands();
        executeStepI(); // Fall back to instruction-wise stepping.
    } else {
        showExecutionError(QString::fromLocal8Bit(msg));
        notifyInferiorIll();
    }
}

void GdbEngine::executeStepI()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Step by instruction requested..."), 5000);
    if (isReverseDebugging())
        postCommand("reverse-stepi", RunRequest, CB(handleExecuteContinue));
    else
        postCommand("-exec-step-instruction", RunRequest, CB(handleExecuteContinue));
}

void GdbEngine::executeStepOut()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    postCommand("-stack-select-frame 0");
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Finish function requested..."), 5000);
    postCommand("-exec-finish", RunRequest, CB(handleExecuteContinue));
}

void GdbEngine::executeNext()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Step next requested..."), 5000);
    if (m_gdbAdapter->isCodaAdapter() && stackHandler()->stackSize() > 0)
        postCommand("sal next,0x" + QByteArray::number(stackHandler()->topAddress(), 16));
    if (isReverseDebugging()) {
        postCommand("reverse-next", RunRequest, CB(handleExecuteNext));
    } else {
        scheduleTestResponse(GdbTestNoBoundsOfCurrentFunction,
            "@TOKEN@^error,msg=\"Warning:\\nCannot insert breakpoint -39.\\n"
            " Error accessing memory address 0x11673fc: Input/output error.\\n\"");
        postCommand("-exec-next", RunRequest, CB(handleExecuteNext));
    }
}

void GdbEngine::handleExecuteNext(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        // Step was finishing too quick, and a '*stopped' messages should
        // have preceded it, so just ignore this result.
        QTC_CHECK(state() == InferiorStopOk);
        return;
    }
    QTC_ASSERT(state() == InferiorRunRequested, qDebug() << state());
    if (response.resultClass == GdbResultRunning) {
        doNotifyInferiorRunOk();
        return;
    }
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    QByteArray msg = response.data.findChild("msg").data();
    if (msg.startsWith("Cannot find bounds of current function")
            || msg.contains("Error accessing memory address ")) {
        if (!m_commandsToRunOnTemporaryBreak.isEmpty())
            flushQueuedCommands();
        notifyInferiorRunFailed();
        if (!isDying())
            executeNextI(); // Fall back to instruction-wise stepping.
    } else {
        showMessageBox(QMessageBox::Critical, tr("Execution Error"),
           tr("Cannot continue debugged process:\n") + QString::fromLocal8Bit(msg));
        notifyInferiorIll();
    }
}

void GdbEngine::executeNextI()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Step next instruction requested..."), 5000);
    if (isReverseDebugging())
        postCommand("reverse-nexti", RunRequest, CB(handleExecuteContinue));
    else
        postCommand("-exec-next-instruction", RunRequest, CB(handleExecuteContinue));
}

static QByteArray addressSpec(quint64 address)
{
    return "*0x" + QByteArray::number(address, 16);
}

void GdbEngine::executeRunToLine(const ContextData &data)
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    resetLocation();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Run to line %1 requested...").arg(data.lineNumber), 5000);
#if 1
    QByteArray loc;
    if (m_isMacGdb) {
        if (data.address)
            loc = addressSpec(data.address);
        else
            loc = "\"\\\"" + breakLocation(data.fileName).toLocal8Bit() + "\\\":"
                + QByteArray::number(data.lineNumber) + '"';
        // "tbreak/continue" does _not_ work on Mac. See #4619
        postCommand("-break-insert -t -l -1 -f " + loc);
        postCommand("-exec-continue", RunRequest, CB(handleExecuteRunToLine));
    } else {
        if (data.address)
            loc = addressSpec(data.address);
        else
            loc = '"' + breakLocation(data.fileName).toLocal8Bit() + '"' + ':'
               + QByteArray::number(data.lineNumber);
        postCommand("tbreak " + loc);
        postCommand("continue", RunRequest, CB(handleExecuteRunToLine));
    }
#else
    // Seems to jump to unpredicatable places. Observed in the manual
    // tests in the Foo::Foo() constructor with both gdb 6.8 and 7.1.
    QByteArray args = '"' + breakLocation(fileName).toLocal8Bit() + '"' + ':'
        + QByteArray::number(lineNumber);
    postCommand("-exec-until " + args, RunRequest, CB(handleExecuteContinue));
#endif
}

void GdbEngine::executeRunToFunction(const QString &functionName)
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    resetLocation();
    postCommand("-break-insert -t " + functionName.toLatin1());
    showStatusMessage(tr("Run to function %1 requested...").arg(functionName), 5000);
    continueInferiorInternal();
}

void GdbEngine::executeJumpToLine(const ContextData &data)
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    QByteArray loc;
    if (data.address)
        loc = addressSpec(data.address);
    else
        loc = '"' + breakLocation(data.fileName).toLocal8Bit() + '"' + ':'
        + QByteArray::number(data.lineNumber);
    postCommand("tbreak " + loc);
    notifyInferiorRunRequested();
    postCommand("jump " + loc, RunRequest, CB(handleExecuteJumpToLine));
    // will produce something like
    //  &"jump \"/home/apoenitz/dev/work/test1/test1.cpp\":242"
    //  ~"Continuing at 0x4058f3."
    //  ~"run1 (argc=1, argv=0x7fffbf1f5538) at test1.cpp:242"
    //  ~"242\t x *= 2;"
    //  23^done"
}

void GdbEngine::executeReturn()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Immediate return from function requested..."), 5000);
    postCommand("-exec-finish", RunRequest, CB(handleExecuteReturn));
}

void GdbEngine::handleExecuteReturn(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        notifyInferiorStopOk();
        updateAll();
        return;
    }
    notifyInferiorRunFailed();
}

/*!
    \fn void Debugger::Internal::GdbEngine::setTokenBarrier()
    \brief Discard the results of all pending watch-updating commands.

    This method is called at the beginning of all step/next/finish etc.
    debugger functions.
    If non-watch-updating commands with call-backs are still in the pipe,
    it will complain.
*/

void GdbEngine::setTokenBarrier()
{
    QTC_ASSERT(m_nonDiscardableCount == 0, /**/);
    bool good = true;
    QHashIterator<int, GdbCommand> it(m_cookieForToken);
    while (it.hasNext()) {
        it.next();
        if (!(it.value().flags & Discardable)) {
            qDebug() << "TOKEN: " << it.key()
                << "CMD:" << it.value().command
                << " FLAGS:" << it.value().flags
                << " CALLBACK:" << it.value().callbackName;
            good = false;
        }
    }
    QTC_ASSERT(good, return);
    PENDING_DEBUG("\n--- token barrier ---\n");
    showMessage(_("--- token barrier ---"), LogMiscInput);
    if (debuggerCore()->boolSetting(LogTimeStamps))
        showMessage(LogWindow::logTimeStamp(), LogMiscInput);
    m_oldestAcceptableToken = currentToken();
    m_stackNeeded = false;
}


//////////////////////////////////////////////////////////////////////
//
// Breakpoint specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::updateResponse(BreakpointResponse &response, const GdbMi &bkpt)
{
    QTC_ASSERT(bkpt.isValid(), return);

    QByteArray originalLocation;

    response.multiple = false;
    response.enabled = true;
    response.pending = false;
    response.condition.clear();
    QByteArray file, fullName;
    foreach (const GdbMi &child, bkpt.children()) {
        if (child.hasName("number")) {
            response.id = BreakpointResponseId(child.data());
        } else if (child.hasName("func")) {
            response.functionName = _(child.data());
        } else if (child.hasName("addr")) {
            // <MULTIPLE> happens in constructors, inline functions, and
            // at other places like 'foreach' lines. In this case there are
            // fields named "addr" in the response and/or the address
            // is called <MULTIPLE>.
            //qDebug() << "ADDR: " << child.data() << (child.data() == "<MULTIPLE>");
            if (child.data() == "<MULTIPLE>")
                response.multiple = true;
            if (child.data().startsWith("0x"))
                response.address = child.toAddress();
        } else if (child.hasName("file")) {
            file = child.data();
        } else if (child.hasName("fullname")) {
            fullName = child.data();
        } else if (child.hasName("line")) {
            // The line numbers here are the uncorrected ones. So don't
            // change it if we know better already.
            if (response.correctedLineNumber == 0)
                response.lineNumber = child.data().toInt();
        } else if (child.hasName("cond")) {
            // gdb 6.3 likes to "rewrite" conditions. Just accept that fact.
            response.condition = child.data();
        } else if (child.hasName("enabled")) {
            response.enabled = (child.data() == "y");
        } else if (child.hasName("pending")) {
            // Any content here would be interesting only if we did accept
            // spontaneously appearing breakpoints (user using gdb commands).
            response.pending = true;
        } else if (child.hasName("at")) {
            // Happens with gdb 6.4 symbianelf.
            QByteArray ba = child.data();
            if (ba.startsWith('<') && ba.endsWith('>'))
                ba = ba.mid(1, ba.size() - 2);
            response.functionName = _(ba);
        } else if (child.hasName("thread")) {
            response.threadSpec = child.data().toInt();
        } else if (child.hasName("type")) {
            // "breakpoint", "hw breakpoint", "tracepoint"
            if (child.data().contains("tracepoint"))
                response.tracepoint = true;
            else if (!child.data().contains("reakpoint"))
                response.type = WatchpointAtAddress;
        } else if (child.hasName("original-location")) {
            originalLocation = child.data();
        }
        // This field is not present.  Contents needs to be parsed from
        // the plain "ignore" response.
        //else if (child.hasName("ignore"))
        //    response.ignoreCount = child.data();
    }

    QString name;
    if (!fullName.isEmpty()) {
        name = cleanupFullName(QFile::decodeName(fullName));
        response.fileName = name;
        //if (data->markerFileName().isEmpty())
        //    data->setMarkerFileName(name);
    } else {
        name = QFile::decodeName(file);
        // Use fullName() once we have a mapping which is more complete than
        // gdb's own. No point in assigning markerFileName for now.
    }
    if (!name.isEmpty())
        response.fileName = name;

    if (response.fileName.isEmpty())
        response.updateLocation(originalLocation);
}

QString GdbEngine::breakLocation(const QString &file) const
{
    //QTC_ASSERT(!m_breakListOutdated, /* */)
    QString where = m_fullToShortName.value(file);
    if (where.isEmpty())
        return QFileInfo(file).fileName();
    return where;
}

QByteArray GdbEngine::breakpointLocation(BreakpointModelId id)
{
    BreakHandler *handler = breakHandler();
    const BreakpointParameters &data = handler->breakpointData(id);
    QTC_ASSERT(data.type != UnknownType, return QByteArray());
    // FIXME: Non-GCC-runtime
    if (data.type == BreakpointAtThrow)
        return "__cxa_throw";
    if (data.type == BreakpointAtCatch)
        return "__cxa_begin_catch";
    if (data.type == BreakpointAtMain) {
        const Abi abi = startParameters().toolChainAbi;
        return (abi.os() == Abi::WindowsOS) ? "qMain" : "main";
    }
    if (data.type == BreakpointByFunction)
        return data.functionName.toUtf8();
    if (data.type == BreakpointByAddress)
        return addressSpec(data.address);

    const QString fileName = data.pathUsage == BreakpointUseFullPath
        ? data.fileName : breakLocation(data.fileName);
    // The argument is simply a C-quoted version of the argument to the
    // non-MI "break" command, including the "original" quoting it wants.
    return "\"\\\"" + GdbMi::escapeCString(fileName).toLocal8Bit() + "\\\":"
        + QByteArray::number(data.lineNumber) + '"';
}

QByteArray GdbEngine::breakpointLocation2(BreakpointModelId id)
{
    BreakHandler *handler = breakHandler();
    const BreakpointParameters &data = handler->breakpointData(id);
    const QString fileName = data.pathUsage == BreakpointUseFullPath
        ? data.fileName : breakLocation(data.fileName);
    return  GdbMi::escapeCString(fileName).toLocal8Bit() + ':'
        + QByteArray::number(data.lineNumber);
}

void GdbEngine::handleWatchInsert(const GdbResponse &response)
{
    BreakpointModelId id = response.cookie.value<BreakpointModelId>();
    if (response.resultClass == GdbResultDone) {
        BreakHandler *handler = breakHandler();
        BreakpointResponse br = handler->response(id);
        // "Hardware watchpoint 2: *0xbfffed40\n"
        QByteArray ba = response.consoleStreamOutput;
        GdbMi wpt = response.data.findChild("wpt");
        if (wpt.isValid()) {
            // Mac yields:
            //>32^done,wpt={number="4",exp="*4355182176"}
            br.id = BreakpointResponseId(wpt.findChild("number").data());
            QByteArray exp = wpt.findChild("exp").data();
            if (exp.startsWith('*'))
                br.address = exp.mid(1).toULongLong(0, 0);
            handler->setResponse(id, br);
            QTC_CHECK(!handler->needsChange(id));
            handler->notifyBreakpointInsertOk(id);
        } else if (ba.startsWith("Hardware watchpoint ")
                || ba.startsWith("Watchpoint ")) {
            // Non-Mac: "Hardware watchpoint 2: *0xbfffed40\n"
            const int end = ba.indexOf(':');
            const int begin = ba.lastIndexOf(' ', end) + 1;
            const QByteArray address = ba.mid(end + 2).trimmed();
            br.id = BreakpointResponseId(ba.mid(begin, end - begin));
            if (address.startsWith('*'))
                br.address = address.mid(1).toULongLong(0, 0);
            handler->setResponse(id, br);
            QTC_CHECK(!handler->needsChange(id));
            handler->notifyBreakpointInsertOk(id);
        } else {
            showMessage(_("CANNOT PARSE WATCHPOINT FROM " + ba));
        }
    }
}

void GdbEngine::attemptAdjustBreakpointLocation(BreakpointModelId id)
{
    if (m_hasBreakpointNotifications)
        return;
    if (!debuggerCore()->boolSetting(AdjustBreakpointLocations))
        return;
    BreakpointResponse response = breakHandler()->response(id);
    if (response.address == 0 || response.correctedLineNumber != 0)
        return;
    // Prevent endless loop.
    response.correctedLineNumber = -1;
    breakHandler()->setResponse(id, response);
    postCommand("info line *0x" + QByteArray::number(response.address, 16),
        NeedsStop | RebuildBreakpointModel,
        CB(handleInfoLine), QVariant::fromValue(id));
}

void GdbEngine::handleCatchInsert(const GdbResponse &response)
{
    BreakHandler *handler = breakHandler();
    BreakpointModelId id = response.cookie.value<BreakpointModelId>();
    if (response.resultClass == GdbResultDone) {
        handler->notifyBreakpointInsertOk(id);
        attemptAdjustBreakpointLocation(id);
    }
}

void GdbEngine::handleBreakInsert1(const GdbResponse &response)
{
    BreakHandler *handler = breakHandler();
    BreakpointModelId id = response.cookie.value<BreakpointModelId>();
    if (handler->state(id) == BreakpointRemoveRequested) {
        if (response.resultClass == GdbResultDone) {
            // This delete was defered. Act now.
            const GdbMi mainbkpt = response.data.findChild("bkpt");
            handler->notifyBreakpointRemoveProceeding(id);
            QByteArray nr = mainbkpt.findChild("number").data();
            postCommand("-break-delete " + nr,
                NeedsStop | RebuildBreakpointModel);
            handler->notifyBreakpointRemoveOk(id);
            return;
        }
    }
    if (response.resultClass == GdbResultDone) {
        // The result is a list with the first entry marked "bkpt"
        // and "unmarked" rest. The "bkpt" one seems to always be
        // the "main" entry. Use the "main" entry to retrieve the
        // already known data from the BreakpointManager, and then
        // iterate over all items to update main- and sub-data.
        const GdbMi mainbkpt = response.data.findChild("bkpt");
        QByteArray nr = mainbkpt.findChild("number").data();
        BreakpointResponseId rid(nr);
        if (!isHiddenBreakpoint(rid)) {
            BreakpointResponse br = handler->response(id);
            foreach (const GdbMi bkpt, response.data.children()) {
                nr = bkpt.findChild("number").data();
                rid = BreakpointResponseId(nr);
                QTC_ASSERT(rid.isValid(), continue);
                if (nr.contains('.')) {
                    // A sub-breakpoint.
                    BreakpointResponse sub;
                    updateResponse(sub, bkpt);
                    sub.id = rid;
                    sub.type = br.type;
                    handler->insertSubBreakpoint(id, sub);
                } else {
                    // A (the?) primary breakpoint.
                    updateResponse(br, bkpt);
                    br.id = rid;
                    handler->setResponse(id, br);
                }
            }
            if (handler->needsChange(id)) {
                handler->notifyBreakpointChangeAfterInsertNeeded(id);
                changeBreakpoint(id);
            } else {
                handler->notifyBreakpointInsertOk(id);
            }
            br = handler->response(id);
            attemptAdjustBreakpointLocation(id);
            // Remove if we only support 7.4 or later.
            if (br.multiple && !m_hasBreakpointNotifications)
                postCommand("info break " + QByteArray::number(br.id.majorPart()),
                    NeedsStop, CB(handleBreakListMultiple),
                    QVariant::fromValue(id));
        }
    } else if (response.data.findChild("msg").data().contains("Unknown option")) {
        // Older version of gdb don't know the -a option to set tracepoints
        // ^error,msg="mi_cmd_break_insert: Unknown option ``a''"
        const QString fileName = handler->fileName(id);
        const int lineNumber = handler->lineNumber(id);
        QByteArray cmd = "trace "
            "\"" + GdbMi::escapeCString(fileName).toLocal8Bit() + "\":"
            + QByteArray::number(lineNumber);
        QVariant vid = QVariant::fromValue(id);
        postCommand(cmd, NeedsStop | RebuildBreakpointModel,
            CB(handleTraceInsert2), vid);
    } else {
        // Some versions of gdb like "GNU gdb (GDB) SUSE (6.8.91.20090930-2.4)"
        // know how to do pending breakpoints using CLI but not MI. So try
        // again with MI.
        QByteArray cmd = "break " + breakpointLocation2(id);
        QVariant vid = QVariant::fromValue(id);
        postCommand(cmd, NeedsStop | RebuildBreakpointModel,
            CB(handleBreakInsert2), vid);
    }
}

void GdbEngine::handleBreakInsert2(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        BreakpointModelId id = response.cookie.value<BreakpointModelId>();
        attemptAdjustBreakpointLocation(id);
        breakHandler()->notifyBreakpointInsertOk(id);
    } else {
        // Note: gdb < 60800  doesn't "do" pending breakpoints.
        // Not much we can do about it except implementing the
        // logic on top of shared library events, and that's not
        // worth the effort.
    }
}

void GdbEngine::handleTraceInsert2(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone)
        reloadBreakListInternal();
}

void GdbEngine::reloadBreakListInternal()
{
    if (m_hasBreakpointNotifications) {
        // Assume this properly handles breakpoint notifications.
        return;
    }
    postCommand("-break-list", NeedsStop | RebuildBreakpointModel,
        CB(handleBreakList));
}

void GdbEngine::handleBreakList(const GdbResponse &response)
{
    // 45^done,BreakpointTable={nr_rows="2",nr_cols="6",hdr=[
    // {width="3",alignment="-1",col_name="number",colhdr="Num"}, ...
    // body=[bkpt={number="1",type="breakpoint",disp="keep",enabled="y",
    //  addr="0x000000000040109e",func="main",file="app.cpp",
    //  fullname="/home/apoenitz/dev/work/plugintest/app/app.cpp",
    //  line="11",times="1"},
    //  bkpt={number="2",type="breakpoint",disp="keep",enabled="y",
    //  addr="<PENDING>",pending="plugin.cpp:7",times="0"}] ... }

    if (response.resultClass == GdbResultDone) {
        GdbMi table = response.data.findChild("BreakpointTable");
        if (table.isValid())
            handleBreakList(table);
    }
}

void GdbEngine::handleBreakList(const GdbMi &table)
{
    const GdbMi body = table.findChild("body");
    QList<GdbMi> bkpts;
    if (body.isValid()) {
        // Non-Mac
        bkpts = body.children();
    } else {
        // Mac
        bkpts = table.children();
        // Remove the 'hdr' and artificial items.
        for (int i = bkpts.size(); --i >= 0; ) {
            int num = bkpts.at(i).findChild("number").data().toInt();
            if (num <= 0)
                bkpts.removeAt(i);
        }
    }

    BreakHandler *handler = breakHandler();
    foreach (const GdbMi &bkpt, bkpts) {
        BreakpointResponse needle;
        needle.id = BreakpointResponseId(bkpt.findChild("number").data());
        if (isQmlStepBreakpoint2(needle.id))
            continue;
        if (isQFatalBreakpoint(needle.id))
            continue;
        BreakpointModelId id = handler->findSimilarBreakpoint(needle);
        if (id.isValid()) {
            BreakpointResponse response = handler->response(id);
            updateResponse(response, bkpt);
            handler->setResponse(id, response);
            attemptAdjustBreakpointLocation(id);
            response = handler->response(id);
            if (response.multiple)
                postCommand("info break " + response.id.toString().toLatin1(),
                    NeedsStop, CB(handleBreakListMultiple),
                    QVariant::fromValue(id));
        } else {
            qDebug() << "  NOTHING SUITABLE FOUND";
            showMessage(_("CANNOT FIND BP: " + bkpt.toString()));
        }
    }

    m_breakListOutdated = false;
}

void GdbEngine::handleBreakListMultiple(const GdbResponse &response)
{
    QTC_CHECK(response.resultClass == GdbResultDone)
    const BreakpointModelId id = response.cookie.value<BreakpointModelId>();
    const QString str = QString::fromLocal8Bit(response.consoleStreamOutput);
    extractDataFromInfoBreak(str, id);
}

void GdbEngine::handleBreakDisable(const GdbResponse &response)
{
    QTC_CHECK(response.resultClass == GdbResultDone)
    const BreakpointModelId id = response.cookie.value<BreakpointModelId>();
    BreakHandler *handler = breakHandler();
    // This should only be the requested state.
    QTC_ASSERT(!handler->isEnabled(id), /* Prevent later recursion */);
    BreakpointResponse br = handler->response(id);
    br.enabled = false;
    handler->setResponse(id, br);
    changeBreakpoint(id); // Maybe there's more to do.
}

void GdbEngine::handleBreakEnable(const GdbResponse &response)
{
    QTC_CHECK(response.resultClass == GdbResultDone)
    const BreakpointModelId id = response.cookie.value<BreakpointModelId>();
    BreakHandler *handler = breakHandler();
    // This should only be the requested state.
    QTC_ASSERT(handler->isEnabled(id), /* Prevent later recursion */);
    BreakpointResponse br = handler->response(id);
    br.enabled = true;
    handler->setResponse(id, br);
    changeBreakpoint(id); // Maybe there's more to do.
}

void GdbEngine::handleBreakThreadSpec(const GdbResponse &response)
{
    QTC_CHECK(response.resultClass == GdbResultDone)
    const BreakpointModelId id = response.cookie.value<BreakpointModelId>();
    BreakHandler *handler = breakHandler();
    BreakpointResponse br = handler->response(id);
    br.threadSpec = handler->threadSpec(id);
    handler->setResponse(id, br);
    handler->notifyBreakpointNeedsReinsertion(id);
    insertBreakpoint(id);
}

void GdbEngine::handleBreakLineNumber(const GdbResponse &response)
{
    QTC_CHECK(response.resultClass == GdbResultDone)
    const BreakpointModelId id = response.cookie.value<BreakpointModelId>();
    BreakHandler *handler = breakHandler();
    BreakpointResponse br = handler->response(id);
    br.lineNumber = handler->lineNumber(id);
    handler->setResponse(id, br);
    handler->notifyBreakpointNeedsReinsertion(id);
    insertBreakpoint(id);
}

void GdbEngine::handleBreakIgnore(const GdbResponse &response)
{
    // gdb 6.8:
    // ignore 2 0:
    // ~"Will stop next time breakpoint 2 is reached.\n"
    // 28^done
    // ignore 2 12:
    // &"ignore 2 12\n"
    // ~"Will ignore next 12 crossings of breakpoint 2.\n"
    // 29^done
    //
    // gdb 6.3 does not produce any console output
    QTC_CHECK(response.resultClass == GdbResultDone)
    //QString msg = _(response.consoleStreamOutput);
    BreakpointModelId id = response.cookie.value<BreakpointModelId>();
    BreakHandler *handler = breakHandler();
    BreakpointResponse br = handler->response(id);
    //if (msg.contains(__("Will stop next time breakpoint")))
    //    response.ignoreCount = _("0");
    //else if (msg.contains(__("Will ignore next")))
    //    response.ignoreCount = data->ignoreCount;
    // FIXME: this assumes it is doing the right thing...
    const BreakpointParameters &parameters = handler->breakpointData(id);
    br.ignoreCount = parameters.ignoreCount;
    br.command = parameters.command;
    handler->setResponse(id, br);
    changeBreakpoint(id); // Maybe there's more to do.
}

void GdbEngine::handleBreakCondition(const GdbResponse &response)
{
    // Can happen at invalid condition strings.
    //QTC_CHECK(response.resultClass == GdbResultDone)
    const BreakpointModelId id = response.cookie.value<BreakpointModelId>();
    BreakHandler *handler = breakHandler();
    // We just assume it was successful. Otherwise we had to parse
    // the output stream data.
    // The following happens on Mac:
    //   QByteArray msg = response.data.findChild("msg").data();
    //   if (1 || msg.startsWith("Error parsing breakpoint condition. "
    //            " Will try again when we hit the breakpoint.")) {
    BreakpointResponse br = handler->response(id);
    br.condition = handler->condition(id);
    handler->setResponse(id, br);
    changeBreakpoint(id); // Maybe there's more to do.
}

void GdbEngine::extractDataFromInfoBreak(const QString &output, BreakpointModelId id)
{
    //qDebug() << output;
    if (output.isEmpty())
        return;
    // "Num     Type           Disp Enb Address            What
    // 4       breakpoint     keep y   <MULTIPLE>         0x00000000004066ad
    // 4.1                         y     0x00000000004066ad in CTorTester
    //  at /data5/dev/ide/main/tests/manual/gdbdebugger/simple/app.cpp:124
    // - or -
    // everything on a single line on Windows for constructors of classes
    // within namespaces.
    // Sometimes the path is relative too.

    // 2    breakpoint     keep y   <MULTIPLE> 0x0040168e
    // 2.1    y     0x0040168e in MainWindow::MainWindow(QWidget*) at mainwindow.cpp:7
    // 2.2    y     0x00401792 in MainWindow::MainWindow(QWidget*) at mainwindow.cpp:7

    // "Num     Type           Disp Enb Address    What
    // 3       breakpoint     keep y   <MULTIPLE> \n"
    // 3.1           y   0x0806094e in Vector<int>::Vector(int) at simple.cpp:141
    // 3.2           y   0x08060994 in Vector<float>::Vector(int) at simple.cpp:141
    // 3.3           y   0x080609da in Vector<double>::Vector(int) at simple.cpp:141
    // 3.4           y   0x08060a1d in Vector<char>::Vector(int) at simple.cpp:141

    BreakHandler *handler = breakHandler();
    BreakpointResponse response = handler->response(id);
    int posMultiple = output.indexOf(_("<MULTIPLE>"));
    if (posMultiple != -1) {
        QByteArray data = output.toUtf8();
        data.replace('\n', ' ');
        data.replace("  ", " ");
        data.replace("  ", " ");
        data.replace("  ", " ");
        int majorPart = 0;
        int minorPart = 0;
        int hitCount = 0;
        bool hitCountComing = false;
        bool functionComing = false;
        bool locationComing = false;
        QByteArray location;
        QByteArray function;
        qulonglong address = 0;
        foreach (const QByteArray &part, data.split(' ')) {
            if (part.isEmpty())
                continue;
            //qDebug() << "PART: " << part;
            if (majorPart == 0) {
                majorPart = part.toInt();
                if (majorPart > 0)
                    continue;
            }
            if (part == "hit") {
                hitCountComing = true;
                continue;
            }
            if (hitCountComing) {
                hitCountComing = false;
                hitCount = part.toInt();
                continue;
            }
            if (part == "at") {
                locationComing = true;
                continue;
            }
            if (locationComing) {
                locationComing = false;
                location = part;
                continue;
            }
            if (part == "in") {
                functionComing = true;
                continue;
            }
            if (functionComing) {
                functionComing = false;
                function = part;
                continue;
            }
            if (part.startsWith("0x")) {
                address = part.toInt(0, 0);
                continue;
            }
            if (part.size() >= 3 && part.count('.') == 1) {
                BreakpointResponseId subId(part);
                int tmpMajor = subId.majorPart();
                int tmpMinor = subId.minorPart();
                if (tmpMajor == majorPart) {
                    if (minorPart) {
                        // Commit what we had before.
                        BreakpointResponse sub;
                        sub.address = address;
                        sub.functionName = QString::fromUtf8(function);
                        sub.updateLocation(location);
                        sub.id = BreakpointResponseId(majorPart, minorPart);
                        sub.type = response.type;
                        sub.address = address;
                        sub.hitCount = hitCount;
                        handler->insertSubBreakpoint(id, sub);
                        location.clear();
                        function.clear();
                        address = 0;
                    }

                    // Now start new.
                    minorPart = tmpMinor;
                    continue;
                }
            }
        }
        if (minorPart) {
            // Commit last chunk.
            BreakpointResponse sub;
            sub.address = address;
            sub.functionName = QString::fromUtf8(function);
            sub.updateLocation(location);
            sub.id = BreakpointResponseId(majorPart, minorPart);
            sub.type = response.type;
            sub.hitCount = hitCount;
            handler->insertSubBreakpoint(id, sub);
            location.clear();
            function.clear();
            address = 0;
        }
    } else {
        qDebug() << "COULD NOT MATCH" << output;
        response.id = BreakpointResponseId(); // Unavailable.
    }
    //handler->setResponse(id, response);
}

void GdbEngine::handleInfoLine(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        // Old-style output: "Line 1102 of \"simple/app.cpp\" starts
        // at address 0x80526aa <_Z10...+131> and ends at 0x80526b5
        // <_Z10testQStackv+142>.\n"
        QByteArray ba = response.consoleStreamOutput;
        const BreakpointModelId id = response.cookie.value<BreakpointModelId>();
        const int pos = ba.indexOf(' ', 5);
        if (ba.startsWith("Line ") && pos != -1) {
            const int line = ba.mid(5, pos - 5).toInt();
            BreakpointResponse br = breakHandler()->response(id);
            br.lineNumber = line;
            br.correctedLineNumber = line;
            breakHandler()->setResponse(id, br);
        }
    }
}

bool GdbEngine::stateAcceptsBreakpointChanges() const
{
    switch (state()) {
    case InferiorSetupRequested:
    case InferiorRunRequested:
    case InferiorRunOk:
    case InferiorStopRequested:
    case InferiorStopOk:
        return true;
    default:
        return false;
    }
}

bool GdbEngine::acceptsBreakpoint(BreakpointModelId id) const
{
    return DebuggerEngine::isCppBreakpoint(breakHandler()->breakpointData(id))
        && startParameters().startMode != AttachCore;
}

void GdbEngine::insertBreakpoint(BreakpointModelId id)
{
    // Set up fallback in case of pending breakpoints which aren't handled
    // by the MI interface.
    BreakHandler *handler = breakHandler();
    QTC_CHECK(handler->state(id) == BreakpointInsertRequested);
    handler->notifyBreakpointInsertProceeding(id);
    BreakpointType type = handler->type(id);
    QVariant vid = QVariant::fromValue(id);
    if (type == WatchpointAtAddress) {
        postCommand("watch " + addressSpec(handler->address(id)),
            NeedsStop | RebuildBreakpointModel,
            CB(handleWatchInsert), vid);
        return;
    }
    if (type == WatchpointAtExpression) {
        postCommand("watch " + handler->expression(id).toLocal8Bit(),
            NeedsStop | RebuildBreakpointModel,
            CB(handleWatchInsert), vid);
        return;
    }
    if (type == BreakpointAtFork) {
        postCommand("catch fork", NeedsStop | RebuildBreakpointModel,
            CB(handleCatchInsert), vid);
        postCommand("catch vfork", NeedsStop | RebuildBreakpointModel,
            CB(handleCatchInsert), vid);
        return;
    }
    //if (type == BreakpointAtVFork) {
    //    postCommand("catch vfork", NeedsStop | RebuildBreakpointModel,
    //        CB(handleCatchInsert), vid);
    //    return;
    //}
    if (type == BreakpointAtExec) {
        postCommand("catch exec", NeedsStop | RebuildBreakpointModel,
            CB(handleCatchInsert), vid);
        return;
    }
    if (type == BreakpointAtSysCall) {
        postCommand("catch syscall", NeedsStop | RebuildBreakpointModel,
            CB(handleCatchInsert), vid);
        return;
    }

    QByteArray cmd = "xxx";
    if (handler->isTracepoint(id)) {
        cmd = "-break-insert -a -f ";
    } else if (m_isMacGdb) {
        cmd = "-break-insert -l -1 -f ";
    } else if (m_gdbAdapter->isCodaAdapter()) {
        cmd = "-break-insert -h -f ";
    } else if (m_gdbVersion >= 70000) {
        int spec = handler->threadSpec(id);
        cmd = "-break-insert ";
        if (spec >= 0)
            cmd += "-p " + QByteArray::number(spec);
        cmd += " -f ";
    } else if (m_gdbVersion >= 60800) {
        // Probably some earlier version would work as well.
        cmd = "-break-insert -f ";
    } else {
        cmd = "-break-insert ";
    }
    //if (!data->condition.isEmpty())
    //    cmd += "-c " + data->condition + ' ';
    cmd += breakpointLocation(id);
    postCommand(cmd, NeedsStop | RebuildBreakpointModel,
        CB(handleBreakInsert1), vid);
}

void GdbEngine::changeBreakpoint(BreakpointModelId id)
{
    BreakHandler *handler = breakHandler();
    const BreakpointParameters &data = handler->breakpointData(id);
    QTC_ASSERT(data.type != UnknownType, return);
    const BreakpointResponse &response = handler->response(id);
    QTC_ASSERT(response.id.isValid(), return);
    const QByteArray bpnr = response.id.toByteArray();
    const BreakpointState state = handler->state(id);
    if (state == BreakpointChangeRequested)
        handler->notifyBreakpointChangeProceeding(id);
    const BreakpointState state2 = handler->state(id);
    QTC_ASSERT(state2 == BreakpointChangeProceeding, qDebug() << state2);
    QVariant vid = QVariant::fromValue(id);

    if (data.threadSpec != response.threadSpec) {
        // The only way to change this seems to be to re-set the bp completely.
        postCommand("-break-delete " + bpnr,
            NeedsStop | RebuildBreakpointModel,
            CB(handleBreakThreadSpec), vid);
        return;
    }
    if (data.lineNumber != response.lineNumber) {
        // The only way to change this seems to be to re-set the bp completely.
        postCommand("-break-delete " + bpnr,
            NeedsStop | RebuildBreakpointModel,
            CB(handleBreakLineNumber), vid);
        return;
    }
    if (data.command != response.command) {
        QByteArray breakCommand = "-break-commands " + bpnr;
        foreach (const QString &command, data.command.split(QLatin1String("\\n"))) {
            if (!command.isEmpty()) {
                breakCommand.append(" \"");
                breakCommand.append(command.toLatin1());
                breakCommand.append('"');
            }
        }
        postCommand(breakCommand, NeedsStop | RebuildBreakpointModel,
                    CB(handleBreakIgnore), vid);
        return;
    }
    if (!data.conditionsMatch(response.condition)) {
        postCommand("condition " + bpnr + ' '  + data.condition,
            NeedsStop | RebuildBreakpointModel,
            CB(handleBreakCondition), vid);
        return;
    }
    if (data.ignoreCount != response.ignoreCount) {
        postCommand("ignore " + bpnr + ' ' + QByteArray::number(data.ignoreCount),
            NeedsStop | RebuildBreakpointModel,
            CB(handleBreakIgnore), vid);
        return;
    }
    if (!data.enabled && response.enabled) {
        postCommand("-break-disable " + bpnr,
            NeedsStop | RebuildBreakpointModel,
            CB(handleBreakDisable), vid);
        return;
    }
    if (data.enabled && !response.enabled) {
        postCommand("-break-enable " + bpnr,
            NeedsStop | RebuildBreakpointModel,
            CB(handleBreakEnable), vid);
        return;
    }
    handler->notifyBreakpointChangeOk(id);
    attemptAdjustBreakpointLocation(id);
}

void GdbEngine::removeBreakpoint(BreakpointModelId id)
{
    BreakHandler *handler = breakHandler();
    QTC_CHECK(handler->state(id) == BreakpointRemoveRequested);
    BreakpointResponse br = handler->response(id);
    if (br.id.isValid()) {
        // We already have a fully inserted breakpoint.
        handler->notifyBreakpointRemoveProceeding(id);
        showMessage(_("DELETING BP %1 IN %2").arg(br.id.toString())
            .arg(handler->fileName(id)));
        postCommand("-break-delete " + br.id.toByteArray(),
            NeedsStop | RebuildBreakpointModel);
        // Pretend it succeeds without waiting for response. Feels better.
        // FIXME: Really?
        handler->notifyBreakpointRemoveOk(id);
    } else {
        // Breakpoint was scheduled to be inserted, but we haven't had
        // an answer so far. Postpone activity by doing nothing.
    }
}


//////////////////////////////////////////////////////////////////////
//
// Modules specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::loadSymbols(const QString &moduleName)
{
    // FIXME: gdb does not understand quoted names here (tested with 6.8)
    postCommand("sharedlibrary " + dotEscape(moduleName.toLocal8Bit()));
    reloadModulesInternal();
    reloadBreakListInternal();
    reloadStack(true);
    updateLocals();
}

void GdbEngine::loadAllSymbols()
{
    postCommand("sharedlibrary .*");
    reloadModulesInternal();
    reloadBreakListInternal();
    reloadStack(true);
    updateLocals();
}

void GdbEngine::loadSymbolsForStack()
{
    bool needUpdate = false;
    const Modules &modules = modulesHandler()->modules();
    foreach (const StackFrame &frame, stackHandler()->frames()) {
        if (frame.function == _("??")) {
            //qDebug() << "LOAD FOR " << frame.address;
            foreach (const Module &module, modules) {
                if (module.startAddress <= frame.address
                        && frame.address < module.endAddress) {
                    postCommand("sharedlibrary "
                        + dotEscape(module.moduleName.toLocal8Bit()));
                    needUpdate = true;
                }
            }
        }
    }
    if (needUpdate) {
        reloadModulesInternal();
        reloadBreakListInternal();
        reloadStack(true);
        updateLocals();
    }
}

void GdbEngine::requestModuleSymbols(const QString &moduleName)
{
    QTemporaryFile tf(QDir::tempPath() + _("/gdbsymbols"));
    if (!tf.open())
        return;
    QString fileName = tf.fileName();
    tf.close();
    postCommand("maint print msymbols " + fileName.toLocal8Bit()
            + ' ' + moduleName.toLocal8Bit(),
        NeedsStop, CB(handleShowModuleSymbols),
        QVariant(moduleName + QLatin1Char('@') +  fileName));
}

void GdbEngine::handleShowModuleSymbols(const GdbResponse &response)
{
    const QString cookie = response.cookie.toString();
    const QString moduleName = cookie.section(QLatin1Char('@'), 0, 0);
    const QString fileName = cookie.section(QLatin1Char('@'), 1, 1);
    if (response.resultClass == GdbResultDone) {
        Symbols rc;
        QFile file(fileName);
        file.open(QIODevice::ReadOnly);
        // Object file /opt/dev/qt/lib/libQtNetworkMyns.so.4:
        // [ 0] A 0x16bd64 _DYNAMIC  moc_qudpsocket.cpp
        // [12] S 0xe94680 _ZN4myns5QFileC1Ev section .plt  myns::QFile::QFile()
        foreach (const QByteArray &line, file.readAll().split('\n')) {
            if (line.isEmpty())
                continue;
            if (line.at(0) != '[')
                continue;
            int posCode = line.indexOf(']') + 2;
            int posAddress = line.indexOf("0x", posCode);
            if (posAddress == -1)
                continue;
            int posName = line.indexOf(" ", posAddress);
            int lenAddress = posName - posAddress - 1;
            int posSection = line.indexOf(" section ");
            int lenName = 0;
            int lenSection = 0;
            int posDemangled = 0;
            if (posSection == -1) {
                lenName = line.size() - posName;
                posDemangled = posName;
            } else {
                lenName = posSection - posName;
                posSection += 10;
                posDemangled = line.indexOf(' ', posSection + 1);
                if (posDemangled == -1) {
                    lenSection = line.size() - posSection;
                } else {
                    lenSection = posDemangled - posSection;
                    posDemangled += 1;
                }
            }
            int lenDemangled = 0;
            if (posDemangled != -1)
                lenDemangled = line.size() - posDemangled;
            Symbol symbol;
            symbol.state = _(line.mid(posCode, 1));
            symbol.address = _(line.mid(posAddress, lenAddress));
            symbol.name = _(line.mid(posName, lenName));
            symbol.section = _(line.mid(posSection, lenSection));
            symbol.demangled = _(line.mid(posDemangled, lenDemangled));
            rc.push_back(symbol);
        }
        file.close();
        file.remove();
        debuggerCore()->showModuleSymbols(moduleName, rc);
    } else {
        showMessageBox(QMessageBox::Critical, tr("Cannot Read Symbols"),
            tr("Cannot read symbols for module \"%1\".").arg(fileName));
    }
}

void GdbEngine::reloadModules()
{
    if (state() == InferiorRunOk || state() == InferiorStopOk)
        reloadModulesInternal();
}

void GdbEngine::reloadModulesInternal()
{
    m_modulesListOutdated = false;
    postCommand("info shared", NeedsStop, CB(handleModulesList));
}

void GdbEngine::handleModulesList(const GdbResponse &response)
{
    Modules modules;
    if (response.resultClass == GdbResultDone) {
        // That's console-based output, likely Linux or Windows,
        // but we can avoid the target dependency here.
        QString data = QString::fromLocal8Bit(response.consoleStreamOutput);
        QTextStream ts(&data, QIODevice::ReadOnly);
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            Module module;
            QString symbolsRead;
            QTextStream ts(&line, QIODevice::ReadOnly);
            if (line.startsWith(__("0x"))) {
                ts >> module.startAddress >> module.endAddress >> symbolsRead;
                module.moduleName = ts.readLine().trimmed();
                module.symbolsRead =
                    (symbolsRead == __("Yes") ? Module::ReadOk : Module::ReadFailed);
                modules.append(module);
            } else if (line.trimmed().startsWith(__("No"))) {
                // gdb 6.4 symbianelf
                ts >> symbolsRead;
                QTC_ASSERT(symbolsRead == __("No"), continue);
                module.startAddress = 0;
                module.endAddress = 0;
                module.moduleName = ts.readLine().trimmed();
                modules.append(module);
            }
        }
        if (modules.isEmpty()) {
            // Mac has^done,shlib-info={num="1",name="dyld",kind="-",
            // dyld-addr="0x8fe00000",reason="dyld",requested-state="Y",
            // state="Y",path="/usr/lib/dyld",description="/usr/lib/dyld",
            // loaded_addr="0x8fe00000",slide="0x0",prefix="__dyld_"},
            // shlib-info={...}...
            foreach (const GdbMi &item, response.data.children()) {
                Module module;
                module.moduleName =
                    QString::fromLocal8Bit(item.findChild("path").data());
                module.symbolsRead = (item.findChild("state").data() == "Y")
                        ? Module::ReadOk : Module::ReadFailed;
                module.startAddress =
                    item.findChild("loaded_addr").data().toULongLong(0, 0);
                module.endAddress = 0; // FIXME: End address not easily available.
                modules.append(module);
            }
        }
    }
    modulesHandler()->setModules(modules);
}


void GdbEngine::examineModules()
{
    foreach (Module module, modulesHandler()->modules()) {
        if (module.symbolsType == Module::UnknownType) {
            QProcess proc;
            qDebug() << _("objdump -h \"%1\"").arg(module.moduleName);
            proc.start(_("objdump -h \"%1\"").arg(module.moduleName));
            if (!proc.waitForStarted())
                continue;
            if (!proc.waitForFinished())
                continue;
            QByteArray ba = proc.readAllStandardOutput();
            if (ba.contains(".gdb_index"))
                module.symbolsType = Module::FastSymbols;
            else
                module.symbolsType = Module::PlainSymbols;
            modulesHandler()->updateModule(module.moduleName, module);
        }
    }
}

//////////////////////////////////////////////////////////////////////
//
// Source files specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::invalidateSourcesList()
{
    m_modulesListOutdated = true;
    m_sourcesListOutdated = true;
    m_breakListOutdated = true;
}

void GdbEngine::reloadSourceFiles()
{
    if ((state() == InferiorRunOk || state() == InferiorStopOk)
        && !m_sourcesListUpdating)
        reloadSourceFilesInternal();
}

void GdbEngine::reloadSourceFilesInternal()
{
    QTC_CHECK(!m_sourcesListUpdating);
    m_sourcesListUpdating = true;
    postCommand("-file-list-exec-source-files", NeedsStop, CB(handleQuerySources));
#if 0
    if (m_gdbVersion < 70000 && !m_isMacGdb)
        postCommand("set stop-on-solib-events 1");
#endif
}


//////////////////////////////////////////////////////////////////////
//
// Stack specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::selectThread(int index)
{
    threadsHandler()->setCurrentThread(index);
    Threads threads = threadsHandler()->threads();
    QTC_ASSERT(index < threads.size(), return);
    const int id = threads.at(index).id;
    showStatusMessage(tr("Retrieving data for stack view thread 0x%1...")
        .arg(id, 0, 16), 10000);
    postCommand("-thread-select " + QByteArray::number(id), Discardable,
        CB(handleStackSelectThread));
}

void GdbEngine::handleStackSelectThread(const GdbResponse &)
{
    QTC_CHECK(state() == InferiorUnrunnable || state() == InferiorStopOk);
    showStatusMessage(tr("Retrieving data for stack view..."), 3000);
    reloadStack(true); // Will reload registers.
    updateLocals();
}

void GdbEngine::reloadFullStack()
{
    PENDING_DEBUG("RELOAD FULL STACK");
    postCommand("-stack-list-frames", Discardable, CB(handleStackListFrames),
        QVariant::fromValue<StackCookie>(StackCookie(true, true)));
}

void GdbEngine::reloadStack(bool forceGotoLocation)
{
    PENDING_DEBUG("RELOAD STACK");
    QByteArray cmd = "-stack-list-frames";
    int stackDepth = debuggerCore()->action(MaximalStackDepth)->value().toInt();
    if (stackDepth && !m_gdbAdapter->isCodaAdapter())
        cmd += " 0 " + QByteArray::number(stackDepth);
    // FIXME: gdb 6.4 symbianelf likes to be asked twice. The first time it
    // returns with "^error,msg="Previous frame identical to this frame
    // (corrupt stack?)". Might be related to the fact that we can't
    // access the memory belonging to the lower frames. But as we know
    // this sometimes happens, ask the second time immediately instead
    // of waiting for the first request to fail.
    // FIXME: Seems to work with 6.8.
    if (m_gdbAdapter->isCodaAdapter() && m_gdbVersion < 6.8)
        postCommand(cmd);
    postCommand(cmd, Discardable, CB(handleStackListFrames),
        QVariant::fromValue<StackCookie>(StackCookie(false, forceGotoLocation)));
}

StackFrame GdbEngine::parseStackFrame(const GdbMi &frameMi, int level)
{
    //qDebug() << "HANDLING FRAME:" << frameMi.toString();
    StackFrame frame;
    frame.level = level;
    GdbMi fullName = frameMi.findChild("fullname");
    if (fullName.isValid())
        frame.file = cleanupFullName(QFile::decodeName(fullName.data()));
    else
        frame.file = QFile::decodeName(frameMi.findChild("file").data());
    frame.function = _(frameMi.findChild("func").data());
    frame.from = _(frameMi.findChild("from").data());
    frame.line = frameMi.findChild("line").data().toInt();
    frame.address = frameMi.findChild("addr").toAddress();
    frame.usable = QFileInfo(frame.file).isReadable();
    return frame;
}

void GdbEngine::handleStackListFrames(const GdbResponse &response)
{
    bool handleIt = (m_isMacGdb || response.resultClass == GdbResultDone);
    if (!handleIt) {
        // That always happens on symbian gdb with
        // ^error,data={msg="Previous frame identical to this frame (corrupt stack?)"
        // logStreamOutput: "Previous frame identical to this frame (corrupt stack?)\n"
        //qDebug() << "LISTING STACK FAILED: " << response.toString();
        reloadRegisters();
        return;
    }

    StackCookie cookie = response.cookie.value<StackCookie>();
    QList<StackFrame> stackFrames;

    GdbMi stack = response.data.findChild("stack");
    if (!stack.isValid()) {
        qDebug() << "FIXME: stack:" << stack.toString();
        return;
    }

    int targetFrame = -1;

    int n = stack.childCount();
    for (int i = 0; i != n; ++i) {
        stackFrames.append(parseStackFrame(stack.childAt(i), i));
        const StackFrame &frame = stackFrames.back();

        // Initialize top frame to the first valid frame.
        const bool isValid = frame.isUsable() && !frame.function.isEmpty();
        if (isValid && targetFrame == -1)
            targetFrame = i;
    }

    bool canExpand = !cookie.isFull
        && (n >= debuggerCore()->action(MaximalStackDepth)->value().toInt());
    debuggerCore()->action(ExpandStack)->setEnabled(canExpand);
    stackHandler()->setFrames(stackFrames, canExpand);

    // We can't jump to any file if we don't have any frames.
    if (stackFrames.isEmpty())
        return;

    // targetFrame contains the top most frame for which we have source
    // information. That's typically the frame we'd like to jump to, with
    // a few exceptions:

    // Always jump to frame #0 when stepping by instruction.
    if (debuggerCore()->boolSetting(OperateByInstruction))
        targetFrame = 0;

    // If there is no frame with source, jump to frame #0.
    if (targetFrame == -1)
        targetFrame = 0;

    stackHandler()->setCurrentIndex(targetFrame);
    activateFrame(targetFrame);
}

void GdbEngine::activateFrame(int frameIndex)
{
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    StackHandler *handler = stackHandler();

    if (frameIndex == handler->stackSize()) {
        reloadFullStack();
        return;
    }

    QTC_ASSERT(frameIndex < handler->stackSize(), return);

    // Assuming the command always succeeds this saves a roundtrip.
    // Otherwise the lines below would need to get triggered
    // after a response to this -stack-select-frame here.
    handler->setCurrentIndex(frameIndex);
    QByteArray cmd = "-stack-select-frame";
    //if (!m_currentThread.isEmpty())
    //    cmd += " --thread " + m_currentThread;
    cmd += ' ';
    cmd += QByteArray::number(frameIndex);
    postCommand(cmd, Discardable, CB(handleStackSelectFrame));
    gotoLocation(stackHandler()->currentFrame());
    updateLocals();
    reloadRegisters();
}

void GdbEngine::handleStackSelectFrame(const GdbResponse &response)
{
    Q_UNUSED(response);
}

void GdbEngine::handleThreadInfo(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        int currentThreadId;
        const Threads threads =
            ThreadsHandler::parseGdbmiThreads(response.data, &currentThreadId);
        threadsHandler()->setThreads(threads);
        threadsHandler()->setCurrentThreadId(currentThreadId);
        updateViews(); // Adjust Threads combobox.
        if (m_hasInferiorThreadList && debuggerCore()->boolSetting(ShowThreadNames)) {
            postCommand("threadnames " +
                debuggerCore()->action(MaximalStackDepth)->value().toByteArray(),
                Discardable, CB(handleThreadNames));
        }
    } else {
        // Fall back for older versions: Try to get at least a list
        // of running threads.
        postCommand("-thread-list-ids", Discardable, CB(handleThreadListIds));
    }
}

void GdbEngine::handleThreadListIds(const GdbResponse &response)
{
    // "72^done,{thread-ids={thread-id="2",thread-id="1"},number-of-threads="2"}
    // In gdb 7.1+ additionally: current-thread-id="1"
    const QList<GdbMi> items = response.data.findChild("thread-ids").children();
    Threads threads;
    for (int index = 0, n = items.size(); index != n; ++index) {
        ThreadData thread;
        thread.id = items.at(index).data().toInt();
        threads.append(thread);
    }
    threadsHandler()->setThreads(threads);
    threadsHandler()->setCurrentThreadId(m_currentThreadId);
}

void GdbEngine::handleThreadNames(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        GdbMi names;
        names.fromString(response.consoleStreamOutput);

        Threads threads = threadsHandler()->threads();

        foreach (const GdbMi &name, names.children()) {
            int id = name.findChild("id").data().toInt();
            for (int index = 0, n = threads.size(); index != n; ++index) {
                ThreadData &thread = threads[index];
                if (thread.id == quint64(id)) {
                    thread.name = decodeData(name.findChild("value").data(),
                        name.findChild("valueencoded").data().toInt());
                    break;
                }
            }
        }
        threadsHandler()->setThreads(threads);
        updateViews();
    }
}


//////////////////////////////////////////////////////////////////////
//
// Snapshot specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::createSnapshot()
{
    QString fileName;
    QTemporaryFile tf(QDir::tempPath() + _("/gdbsnapshot"));
    if (tf.open()) {
        fileName = tf.fileName();
        tf.close();
        postCommand("gcore " + fileName.toLocal8Bit(),
            NeedsStop|ConsoleCommand, CB(handleMakeSnapshot), fileName);
    } else {
        showMessageBox(QMessageBox::Critical, tr("Snapshot Creation Error"),
            tr("Cannot create snapshot file."));
    }
}

void GdbEngine::handleMakeSnapshot(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        DebuggerStartParameters sp = startParameters();
        sp.startMode = AttachCore;
        sp.coreFile = response.cookie.toString();
        //snapshot.setDate(QDateTime::currentDateTime());
        StackFrames frames = stackHandler()->frames();
        QString function = _("<unknown>");
        if (!frames.isEmpty()) {
            const StackFrame &frame = frames.at(0);
            function = frame.function + _(":") + QString::number(frame.line);
        }
        sp.displayName = function + _(": ") + QDateTime::currentDateTime().toString();
        sp.isSnapshot = true;
        DebuggerRunControl *rc = DebuggerPlugin::createDebugger(sp);
        DebuggerPlugin::startDebugger(rc);
    } else {
        QByteArray msg = response.data.findChild("msg").data();
        showMessageBox(QMessageBox::Critical, tr("Snapshot Creation Error"),
            tr("Cannot create snapshot:\n") + QString::fromLocal8Bit(msg));
    }
}


//////////////////////////////////////////////////////////////////////
//
// Register specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::reloadRegisters()
{
    if (!debuggerCore()->isDockVisible(_(Constants::DOCKWIDGET_REGISTER)))
        return;

    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;
    if (!m_registerNamesListed) {
        postCommand("-data-list-register-names", CB(handleRegisterListNames));
        m_registerNamesListed = true;
        // FIXME: Maybe better completely re-do this logic in CODA adapter.
        if (m_gdbAdapter->isCodaAdapter())
            return;
    }

    if (m_gdbAdapter->isCodaAdapter()) {
        m_gdbAdapter->codaReloadRegisters();
    } else {
        postCommand("-data-list-register-values r",
                    Discardable, CB(handleRegisterListValues));
    }
}

void GdbEngine::setRegisterValue(int nr, const QString &value)
{
    Register reg = registerHandler()->registers().at(nr);
    postCommand("set $" + reg.name  + "=" + value.toLatin1());
    reloadRegisters();
}

void GdbEngine::handleRegisterListNames(const GdbResponse &response)
{
    if (response.resultClass != GdbResultDone) {
        m_registerNamesListed = false;
        return;
    }

    Registers registers;
    foreach (const GdbMi &item, response.data.findChild("register-names").children())
        if (!item.data().isEmpty())
            registers.append(Register(item.data()));

    registerHandler()->setRegisters(registers);

    if (m_gdbAdapter->isCodaAdapter())
        m_gdbAdapter->codaReloadRegisters();
}

void GdbEngine::handleRegisterListValues(const GdbResponse &response)
{
    if (response.resultClass != GdbResultDone)
        return;

    Registers registers = registerHandler()->registers();
    const int registerCount = registers.size();

    // 24^done,register-values=[{number="0",value="0xf423f"},...]
    const GdbMi values = response.data.findChild("register-values");
    QTC_ASSERT(registerCount == values.children().size(), return);
    foreach (const GdbMi &item, values.children()) {
        const int number = item.findChild("number").data().toInt();
        if (number >= 0 && number < registerCount)
            registers[number].value = item.findChild("value").data();
    }
    registerHandler()->setAndMarkRegisters(registers);
}


//////////////////////////////////////////////////////////////////////
//
// Thread specific stuff
//
//////////////////////////////////////////////////////////////////////

bool GdbEngine::supportsThreads() const
{
    // FSF gdb 6.3 crashes happily on -thread-list-ids. So don't use it.
    // The test below is a semi-random pick, 6.8 works fine
    return m_isMacGdb || m_gdbVersion > 60500;
}


//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////

bool GdbEngine::showToolTip()
{
    if (m_toolTipContext.isNull())
        return false;
    const QString expression = m_toolTipContext->expression;
    const QByteArray iname = tooltipIName(m_toolTipContext->expression);
    if (DebuggerToolTipManager::debug())
        qDebug() << "GdbEngine::showToolTip " << expression << iname << (*m_toolTipContext);

    if (!debuggerCore()->boolSetting(UseToolTipsInMainEditor)) {
        watchHandler()->removeData(iname);
        return true;
    }

    const QModelIndex index = watchHandler()->itemIndex(iname);
    if (!index.isValid()) {
        watchHandler()->removeData(iname);
        return false;
    }
    DebuggerToolTipWidget *tw = new DebuggerToolTipWidget;
    tw->setExpression(expression);
    tw->setContext(*m_toolTipContext);
    tw->acquireEngine(this);
    DebuggerToolTipManager::instance()->showToolTip(m_toolTipContext->mousePosition,
                                                    m_toolTipContext->editor, tw);
    // Prevent tooltip from re-occurring (classic GDB, QTCREATORBUG-4711).
    m_toolTipContext.reset();
    return true;
}

QString GdbEngine::tooltipExpression() const
{
    return m_toolTipContext.isNull() ? QString() : m_toolTipContext->expression;
}

void GdbEngine::clearToolTip()
{
    m_toolTipContext.reset();
}

bool GdbEngine::setToolTipExpression(const QPoint &mousePos,
    TextEditor::ITextEditor *editor, const DebuggerToolTipContext &contextIn)
{
    if (state() != InferiorStopOk || !isCppEditor(editor)) {
        //qDebug() << "SUPPRESSING DEBUGGER TOOLTIP, INFERIOR NOT STOPPED "
        // " OR NOT A CPPEDITOR";
        return false;
    }

    DebuggerToolTipContext context = contextIn;
    int line, column;
    QString exp = cppExpressionAt(editor, context.position, &line, &column, &context.function);
    if (DebuggerToolTipManager::debug())
        qDebug() << "GdbEngine::setToolTipExpression1 " << exp << context;
    if (exp.isEmpty())
        return false;

    // Extract the first identifier, everything else is considered
    // too dangerous.
    int pos1 = 0, pos2 = exp.size();
    bool inId = false;
    for (int i = 0; i != exp.size(); ++i) {
        const QChar c = exp.at(i);
        const bool isIdChar = c.isLetterOrNumber() || c.unicode() == '_';
        if (inId && !isIdChar) {
            pos2 = i;
            break;
        }
        if (!inId && isIdChar) {
            inId = true;
            pos1 = i;
        }
    }
    exp = exp.mid(pos1, pos2 - pos1);

    if (exp.isEmpty() || exp.startsWith(_c('#')) || !hasLetterOrNumber(exp) || isKeyWord(exp))
        return false;

    if (exp.startsWith(_c('"')) && exp.endsWith(_c('"')))
        return false;

    if (exp.startsWith(__("++")) || exp.startsWith(__("--")))
        exp = exp.mid(2);

    if (exp.endsWith(__("++")) || exp.endsWith(__("--")))
        exp = exp.mid(2);

    if (exp.startsWith(_c('<')) || exp.startsWith(_c('[')))
        return false;

    if (hasSideEffects(exp) || exp.isEmpty())
        return false;

    if (!m_toolTipContext.isNull() && m_toolTipContext->expression == exp) {
        showToolTip();
        return true;
    }

    m_toolTipContext.reset(new GdbToolTipContext(context));
    m_toolTipContext->mousePosition = mousePos;
    m_toolTipContext->expression = exp;
    m_toolTipContext->editor = editor;
    if (DebuggerToolTipManager::debug())
        qDebug() << "GdbEngine::setToolTipExpression2 " << exp << (*m_toolTipContext);

    if (isSynchronous()) {
        UpdateParameters params;
        params.tryPartial = true;
        params.tooltipOnly = true;
        params.varList = tooltipIName(exp);
        updateLocalsPython(params);
    } else {
        WatchData toolTip;
        toolTip.exp = exp.toLatin1();
        toolTip.name = exp;
        toolTip.iname = tooltipIName(exp);
        watchHandler()->removeData(toolTip.iname);
        watchHandler()->insertData(toolTip);
    }
    return true;
}


//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::reloadLocals()
{
    setTokenBarrier();
    updateLocals();
}

bool GdbEngine::hasDebuggingHelperForType(const QByteArray &type) const
{
    if (!debuggerCore()->boolSetting(UseDebuggingHelpers))
        return false;

    if (m_gdbAdapter->dumperHandling() == AbstractGdbAdapter::DumperNotAvailable) {
        // Inferior calls are not possible in gdb when looking at core files.
        return type == "QString" || type.endsWith("::QString")
            || type == "QStringList" || type.endsWith("::QStringList");
    }

    if (m_debuggingHelperState != DebuggingHelperAvailable)
        return false;

    // Simple types.
    return m_dumperHelper.type(type) != DumperHelper::UnknownType;
}


void GdbEngine::updateWatchData(const WatchData &data, const WatchUpdateFlags &flags)
{
    if (isSynchronous()) {
        // This should only be called for fresh expanded items, not for
        // items that had their children retrieved earlier.
        //qDebug() << "\nUPDATE WATCH DATA: " << data.toString() << "\n";
#if 0
        WatchData data1 = data;
        data1.setAllUnneeded();
        insertData(data1);
        rebuildModel();
#else
        if (data.iname.endsWith("."))
            return;

        // Avoid endless loops created by faulty dumpers.
        QByteArray processedName = "1-" + data.iname;
        //qDebug() << "PROCESSED NAMES: " << processedName << m_processedNames;
        if (m_processedNames.contains(processedName)) {
            WatchData data1 = data;
            showMessage(_("<Breaking endless loop for " + data.iname + '>'),
                LogMiscInput);
            data1.setAllUnneeded();
            data1.setValue(_("<unavailable>"));
            data1.setHasChildren(false);
            insertData(data1);
            return;
        }
        m_processedNames.insert(processedName);

        // FIXME: Is this sufficient when "external" changes are
        // triggered e.g. by manually entered command in the gdb console?
        //qDebug() << "TRY PARTIAL: " << flags.tryIncremental
        //        << hasPython()
        //        << (m_pendingWatchRequests == 0)
        //        << (m_pendingBreakpointRequests == 0);

        UpdateParameters params;
        params.tooltipOnly = data.iname.startsWith("tooltip");
        params.tryPartial = flags.tryIncremental
                && hasPython()
                && m_pendingWatchRequests == 0
                && m_pendingBreakpointRequests == 0;
        params.varList = data.iname;

        updateLocalsPython(params);
#endif
    } else {
        // Bump requests to avoid model rebuilding during the nested
        // updateWatchModel runs.
        ++m_pendingWatchRequests;
        PENDING_DEBUG("UPDATE WATCH BUMPS PENDING UP TO " << m_pendingWatchRequests);
#if 1
        QMetaObject::invokeMethod(this, "updateWatchDataHelper",
            Qt::QueuedConnection, Q_ARG(WatchData, data));
#else
        updateWatchDataHelper(data);
#endif
    }
}

void GdbEngine::updateWatchDataHelper(const WatchData &data)
{
    //m_pendingRequests = 0;
    PENDING_DEBUG("UPDATE WATCH DATA");
#    if DEBUG_PENDING
    //qDebug() << "##############################################";
    qDebug() << "UPDATE MODEL, FOUND INCOMPLETE:";
    //qDebug() << data.toString();
#    endif

    updateSubItemClassic(data);
    //PENDING_DEBUG("INTERNAL TRIGGERING UPDATE WATCH MODEL");
    --m_pendingWatchRequests;
    PENDING_DEBUG("UPDATE WATCH DONE BUMPS PENDING DOWN TO " << m_pendingWatchRequests);
    if (m_pendingWatchRequests <= 0)
        rebuildWatchModel();
}

void GdbEngine::rebuildWatchModel()
{
    static int count = 0;
    ++count;
    if (!isSynchronous())
        m_processedNames.clear();
    PENDING_DEBUG("REBUILDING MODEL" << count);
    if (debuggerCore()->boolSetting(LogTimeStamps))
        showMessage(LogWindow::logTimeStamp(), LogMiscInput);
    showMessage(_("<Rebuild Watchmodel %1>").arg(count), LogMiscInput);
    showStatusMessage(tr("Finished retrieving data"), 400);
    watchHandler()->endCycle();
    showToolTip();
    handleAutoTests();
}

static QByteArray arrayFillCommand(const char *array, const QByteArray &params)
{
    QString buf;
    buf.sprintf("set {char[%d]} &%s = {", params.size(), array);
    QByteArray encoded;
    encoded.append(buf.toLocal8Bit());
    const int size = params.size();
    for (int i = 0; i != size; ++i) {
        buf.sprintf("%d,", int(params[i]));
        encoded.append(buf.toLocal8Bit());
    }
    encoded[encoded.size() - 1] = '}';
    return encoded;
}

void GdbEngine::sendWatchParameters(const QByteArray &params0)
{
    QByteArray params = params0;
    params.append('\0');
    const QByteArray inBufferCmd = arrayFillCommand("qDumpInBuffer", params);

    params.replace('\0','!');
    showMessage(QString::fromUtf8(params), LogMiscInput);

    params.clear();
    params.append('\0');
    const QByteArray outBufferCmd = arrayFillCommand("qDumpOutBuffer", params);

    postCommand(inBufferCmd);
    postCommand(outBufferCmd);
}

void GdbEngine::handleVarAssign(const GdbResponse &)
{
    // Everything might have changed, force re-evaluation.
    setTokenBarrier();
    updateLocals();
}

void GdbEngine::handleVarCreate(const GdbResponse &response)
{
    WatchData data = response.cookie.value<WatchData>();
    // Happens e.g. when we already issued a var-evaluate command.
    if (!data.isValid())
        return;
    //qDebug() << "HANDLE VARIABLE CREATION:" << data.toString();
    if (response.resultClass == GdbResultDone) {
        data.variable = data.iname;
        setWatchDataType(data, response.data.findChild("type"));
        if (watchHandler()->isExpandedIName(data.iname)
                && !response.data.findChild("children").isValid())
            data.setChildrenNeeded();
        else
            data.setChildrenUnneeded();
        setWatchDataChildCount(data, response.data.findChild("numchild"));
        insertData(data);
    } else {
        data.setError(QString::fromLocal8Bit(response.data.findChild("msg").data()));
        if (data.isWatcher()) {
            data.value = WatchData::msgNotInScope();
            data.type = " ";
            data.setAllUnneeded();
            data.setHasChildren(false);
            data.valueEnabled = false;
            data.valueEditable = false;
            insertData(data);
        }
    }
}

void GdbEngine::handleDebuggingHelperSetup(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
    } else {
        QString msg = QString::fromLocal8Bit(response.data.findChild("msg").data());
        showStatusMessage(tr("Custom dumper setup: %1").arg(msg), 10000);
    }
}

void GdbEngine::updateLocals()
{
    if (hasPython())
        updateLocalsPython(UpdateParameters());
    else
        updateLocalsClassic();
}

// Parse a local variable from GdbMi.
WatchData GdbEngine::localVariable(const GdbMi &item,
                                   const QStringList &uninitializedVariables,
                                   QMap<QByteArray, int> *seen)
{
    // Local variables of inlined code are reported as
    // 26^done,locals={varobj={exp="this",value="",name="var4",exp="this",
    // numchild="1",type="const QtSharedPointer::Basic<CPlusPlus::..."}}
    // We do not want these at all. Current hypotheses is that those
    // "spurious" locals have _two_ "exp" field. Try to filter them:
    QByteArray name;
    if (m_isMacGdb) {
        int numExps = 0;
        foreach (const GdbMi &child, item.children())
            numExps += int(child.name() == "exp");
        if (numExps > 1)
            return WatchData();
        name = item.findChild("exp").data();
    } else {
        name = item.findChild("name").data();
    }
    const QMap<QByteArray, int>::iterator it  = seen->find(name);
    if (it != seen->end()) {
        const int n = it.value();
        ++(it.value());
        WatchData data;
        QString nam = _(name);
        data.iname = "local." + name + QByteArray::number(n + 1);
        data.name = WatchData::shadowedName(nam, n);
        if (uninitializedVariables.contains(data.name)) {
            data.setError(WatchData::msgNotInScope());
            return data;
        }
        setWatchDataValue(data, item);
        //: Type of local variable or parameter shadowed by another
        //: variable of the same name in a nested block.
        data.setType(GdbEngine::tr("<shadowed>").toUtf8());
        data.setHasChildren(false);
        return data;
    }
    seen->insert(name, 1);
    WatchData data;
    QString nam = _(name);
    data.iname = "local." + name;
    data.name = nam;
    data.exp = name;
    setWatchDataType(data, item.findChild("type"));
    if (uninitializedVariables.contains(data.name)) {
        data.setError(WatchData::msgNotInScope());
        return data;
    }
    if (isSynchronous()) {
        setWatchDataValue(data, item);
        // We know that the complete list of children is
        // somewhere in the response.
        data.setChildrenUnneeded();
    } else {
        // Set value only directly if it is simple enough, otherwise
        // pass through the insertData() machinery.
        if (isIntOrFloatType(data.type) || isPointerType(data.type))
            setWatchDataValue(data, item);
        if (isSymbianIntType(data.type)) {
            setWatchDataValue(data, item);
            data.setHasChildren(false);
        }
    }

    if (!watchHandler()->isExpandedIName(data.iname))
        data.setChildrenUnneeded();

    GdbMi t = item.findChild("numchild");
    if (t.isValid())
        data.setHasChildren(t.data().toInt() > 0);
    else if (isPointerType(data.type) || data.name == __("this"))
        data.setHasChildren(true);
    return data;
}

void GdbEngine::insertData(const WatchData &data0)
{
    PENDING_DEBUG("INSERT DATA" << data0.toString());
    WatchData data = data0;
    if (data.value.startsWith(__("mi_cmd_var_create:"))) {
        qDebug() << "BOGUS VALUE:" << data.toString();
        return;
    }
    watchHandler()->insertData(data);
}

void GdbEngine::assignValueInDebugger(const WatchData *data,
    const QString &expression, const QVariant &value)
{
    if (hasPython() && !isIntOrFloatType(data->type)) {
        QByteArray cmd = "bbedit "
            + data->type.toHex() + ','
            + expression.toUtf8().toHex() + ','
            + value.toString().toUtf8().toHex();
        postCommand(cmd, Discardable, CB(handleVarAssign));
    } else {
        postCommand("-var-delete assign");
        postCommand("-var-create assign * " + expression.toLatin1());
        postCommand("-var-assign assign " +
                GdbMi::escapeCString(value.toString().toLatin1()),
            Discardable, CB(handleVarAssign));
    }
}

void GdbEngine::watchPoint(const QPoint &pnt)
{
    QByteArray x = QByteArray::number(pnt.x());
    QByteArray y = QByteArray::number(pnt.y());
    postCommand("print '" + qtNamespace() + "QApplication::widgetAt'("
            + x + ',' + y + ')',
        NeedsStop, CB(handleWatchPoint));
}

void GdbEngine::handleWatchPoint(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        // "$5 = (void *) 0xbfa7ebfc\n"
        const QByteArray ba = parsePlainConsoleStream(response);
        //qDebug() << "BA: " << ba;
        const int posWidget = ba.indexOf("QWidget");
        const int pos0x = ba.indexOf("0x", posWidget + 7);
        if (posWidget == -1 || pos0x == -1) {
            showStatusMessage(tr("Cannot read widget data: %1").arg(_(ba)));
        } else {
            const QByteArray addr = ba.mid(pos0x);
            if (addr.toULongLong(0, 0)) { // Non-null pointer
                const QByteArray ns = qtNamespace();
                const QByteArray type = ns.isEmpty() ? "QWidget*"
                    : QByteArray("'" + ns + "QWidget'*");
                const QString exp = _("(*(struct %1)%2)").arg(_(type)).arg(_(addr));
                // qDebug() << posNs << posWidget << pos0x << addr << ns << type;
                watchHandler()->watchExpression(exp);
            } else {
                showStatusMessage(tr("Could not find a widget."));
            }
        }
    }
}

class MemoryAgentCookie
{
public:
    MemoryAgentCookie() : agent(0), token(0), address(0) {}

    MemoryAgentCookie(MemoryAgent *agent_, QObject *token_, quint64 address_)
        : agent(agent_), token(token_), address(address_)
    {}

public:
    QPointer<MemoryAgent> agent;
    QPointer<QObject> token;
    quint64 address;
};

void GdbEngine::changeMemory(MemoryAgent *agent, QObject *token,
        quint64 addr, const QByteArray &data)
{
    QByteArray cmd = "-data-write-memory " + QByteArray::number(addr) + " d 1";
    foreach (unsigned char c, data) {
        cmd.append(' ');
        cmd.append(QByteArray::number(uint(c)));
    }
    postCommand(cmd, NeedsStop, CB(handleChangeMemory),
        QVariant::fromValue(MemoryAgentCookie(agent, token, addr)));
}

void GdbEngine::handleChangeMemory(const GdbResponse &response)
{
    Q_UNUSED(response);
}

void GdbEngine::fetchMemory(MemoryAgent *agent, QObject *token, quint64 addr,
                            quint64 length)
{
    postCommand("-data-read-memory " + QByteArray::number(addr) + " x 1 1 "
            + QByteArray::number(length),
        NeedsStop, CB(handleFetchMemory),
        QVariant::fromValue(MemoryAgentCookie(agent, token, addr)));
}

void GdbEngine::handleFetchMemory(const GdbResponse &response)
{
    // ^done,addr="0x08910c88",nr-bytes="16",total-bytes="16",
    // next-row="0x08910c98",prev-row="0x08910c78",next-page="0x08910c98",
    // prev-page="0x08910c78",memory=[{addr="0x08910c88",
    // data=["1","0","0","0","5","0","0","0","0","0","0","0","0","0","0","0"]}]
    MemoryAgentCookie ac = response.cookie.value<MemoryAgentCookie>();
    QTC_ASSERT(ac.agent, return);
    QByteArray ba;
    GdbMi memory = response.data.findChild("memory");
    QTC_ASSERT(memory.children().size() <= 1, return);
    if (memory.children().isEmpty())
        return;
    GdbMi memory0 = memory.children().at(0); // we asked for only one 'row'
    GdbMi data = memory0.findChild("data");
    foreach (const GdbMi &child, data.children()) {
        bool ok = true;
        unsigned char c = '?';
        c = child.data().toUInt(&ok, 0);
        QTC_ASSERT(ok, return);
        ba.append(c);
    }
    ac.agent->addLazyData(ac.token, ac.address, ba);
}

class DisassemblerAgentCookie
{
public:
    DisassemblerAgentCookie() : agent(0) {}
    DisassemblerAgentCookie(DisassemblerAgent *agent_) : agent(agent_) {}

public:
    QPointer<DisassemblerAgent> agent;
};

void GdbEngine::fetchDisassembler(DisassemblerAgent *agent)
{
    // As of 7.2 the MI output is often less informative then the CLI version.
    // So globally fall back to CLI.
    if (agent->isMixed())
        fetchDisassemblerByCliPointMixed(agent);
    else
        fetchDisassemblerByCliPointPlain(agent);
#if 0
    if (agent->isMixed())
        fetchDisassemblerByMiRangeMixed(agent)
    else
        fetchDisassemblerByMiRangePlain(agent);
#endif
}

#if 0
void GdbEngine::fetchDisassemblerByMiRangePlain(const DisassemblerAgentCookie &ac0)
{
    // Disassemble full function:
    const StackFrame &frame = agent->frame();
    DisassemblerAgentCookie ac = ac0;
    QTC_ASSERT(ac.agent, return);
    const quint64 address = ac.agent->address();
    QByteArray cmd = "-data-disassemble"
        " -f " + frame.file.toLocal8Bit() +
        " -l " + QByteArray::number(frame.line) + " -n -1 -- 1";
    postCommand(cmd, Discardable, CB(handleFetchDisassemblerByMiPointMixed),
        QVariant::fromValue(DisassemblerAgentCookie(agent)));
}
#endif

#if 0
void GdbEngine::fetchDisassemblerByMiRangeMixed(const DisassemblerAgentCookie &ac0)
{
    DisassemblerAgentCookie ac = ac0;
    QTC_ASSERT(ac.agent, return);
    const quint64 address = ac.agent->address();
    QByteArray start = QByteArray::number(address - 20, 16);
    QByteArray end = QByteArray::number(address + 100, 16);
    // -data-disassemble [ -s start-addr -e end-addr ]
    //  | [ -f filename -l linenum [ -n lines ] ] -- mode
    postCommand("-data-disassemble -s 0x" + start + " -e 0x" + end + " -- 1",
        Discardable, CB(handleFetchDisassemblerByMiRangeMixed),
        QVariant::fromValue(ac));
}
#endif

#if 0
void GdbEngine::fetchDisassemblerByMiRangePlain(const DisassemblerAgentCookie &ac0)
{
    DisassemblerAgentCookie ac = ac0;
    QTC_ASSERT(ac.agent, return);
    const quint64 address = ac.agent->address();
    QByteArray start = QByteArray::number(address - 20, 16);
    QByteArray end = QByteArray::number(address + 100, 16);
    // -data-disassemble [ -s start-addr -e end-addr ]
    //  | [ -f filename -l linenum [ -n lines ] ] -- mode
    postCommand("-data-disassemble -s 0x" + start + " -e 0x" + end + " -- 0",
        Discardable, CB(handleFetchDisassemblerByMiRangePlain),
        QVariant::fromValue(ac));
}
#endif

void GdbEngine::fetchDisassemblerByCliPointMixed(const DisassemblerAgentCookie &ac0)
{
    DisassemblerAgentCookie ac = ac0;
    QTC_ASSERT(ac.agent, return);
    const quint64 address = ac.agent->address();
    QByteArray cmd = "disassemble /m 0x" + QByteArray::number(address, 16);
    postCommand(cmd, Discardable, CB(handleFetchDisassemblerByCliPointMixed),
        QVariant::fromValue(ac));
}

void GdbEngine::fetchDisassemblerByCliPointPlain(const DisassemblerAgentCookie &ac0)
{
    DisassemblerAgentCookie ac = ac0;
    QTC_ASSERT(ac.agent, return);
    const quint64 address = ac.agent->address();
    QByteArray cmd = "disassemble 0x" + QByteArray::number(address, 16);
    postCommand(cmd, Discardable, CB(handleFetchDisassemblerByCliPointPlain),
        QVariant::fromValue(ac));
}

void GdbEngine::fetchDisassemblerByCliRangeMixed(const DisassemblerAgentCookie &ac0)
{
    DisassemblerAgentCookie ac = ac0;
    QTC_ASSERT(ac.agent, return);
    const quint64 address = ac.agent->address();
    QByteArray start = QByteArray::number(address - 20, 16);
    QByteArray end = QByteArray::number(address + 100, 16);
    const char sep = m_disassembleUsesComma ? ',' : ' ';
    QByteArray cmd = "disassemble /m 0x" + start + sep + "0x" + end;
    postCommand(cmd, Discardable, CB(handleFetchDisassemblerByCliRangeMixed),
        QVariant::fromValue(ac));
}

void GdbEngine::fetchDisassemblerByCliRangePlain(const DisassemblerAgentCookie &ac0)
{
    DisassemblerAgentCookie ac = ac0;
    QTC_ASSERT(ac.agent, return);
    const quint64 address = ac.agent->address();
    QByteArray start = QByteArray::number(address - 20, 16);
    QByteArray end = QByteArray::number(address + 100, 16);
    const char sep = m_disassembleUsesComma ? ',' : ' ';
    QByteArray cmd = "disassemble 0x" + start + sep + "0x" + end;
    postCommand(cmd, Discardable, CB(handleFetchDisassemblerByCliRangePlain),
        QVariant::fromValue(ac));
}

static DisassemblerLine parseLine(const GdbMi &line)
{
    DisassemblerLine dl;
    QByteArray address = line.findChild("address").data();
    dl.address = address.toULongLong(0, 0);
    dl.data = _(line.findChild("inst").data());
    dl.function = _(line.findChild("func-name").data());
    dl.offset = line.findChild("offset").data().toUInt();
    return dl;
}

DisassemblerLines GdbEngine::parseMiDisassembler(const GdbMi &lines)
{
    // ^done,data={asm_insns=[src_and_asm_line={line="1243",file=".../app.cpp",
    // line_asm_insn=[{address="0x08054857",func-name="main",offset="27",
    // inst="call 0x80545b0 <_Z13testQFileInfov>"}]},
    // src_and_asm_line={line="1244",file=".../app.cpp",
    // line_asm_insn=[{address="0x0805485c",func-name="main",offset="32",
    //inst="call 0x804cba1 <_Z11testObject1v>"}]}]}
    // - or - (non-Mac)
    // ^done,asm_insns=[
    // {address="0x0805acf8",func-name="...",offset="25",inst="and $0xe8,%al"},
    // {address="0x0805acfa",func-name="...",offset="27",inst="pop %esp"}, ..]
    // - or - (MAC)
    // ^done,asm_insns={
    // {address="0x0d8f69e0",func-name="...",offset="1952",inst="add $0x0,%al"},..}

    DisassemblerLines result;

    // FIXME: Performance?
    foreach (const GdbMi &child, lines.children()) {
        if (child.hasName("src_and_asm_line")) {
            const QString fileName = QFile::decodeName(child.findChild("file").data());
            const uint line = child.findChild("line").data().toUInt();
            result.appendSourceLine(fileName, line);
            GdbMi insn = child.findChild("line_asm_insn");
            foreach (const GdbMi &item, insn.children())
                result.appendLine(parseLine(item));
        } else {
            // The non-mixed version.
            result.appendLine(parseLine(child));
        }
    }
    return result;
}

DisassemblerLines GdbEngine::parseCliDisassembler(const QByteArray &output)
{
    // First line is something like
    // "Dump of assembler code from 0xb7ff598f to 0xb7ff5a07:"
    DisassemblerLines dlines;
    foreach (const QByteArray &line, output.split('\n'))
        dlines.appendUnparsed(_(line));
    return dlines;
}

DisassemblerLines GdbEngine::parseDisassembler(const GdbResponse &response)
{
    // Apple's gdb produces MI output even for CLI commands.
    // FIXME: Check whether wrapping this into -interpreter-exec console
    // (i.e. usgind the 'ConsoleCommand' GdbCommandFlag makes a
    // difference.
    GdbMi lines = response.data.findChild("asm_insns");
    if (lines.isValid())
        return parseMiDisassembler(lines);
    return parseCliDisassembler(response.consoleStreamOutput);
}

void GdbEngine::handleDisassemblerCheck(const GdbResponse &response)
{
    m_disassembleUsesComma = response.resultClass != GdbResultDone;
}

void GdbEngine::handleFetchDisassemblerByCliPointMixed(const GdbResponse &response)
{
    DisassemblerAgentCookie ac = response.cookie.value<DisassemblerAgentCookie>();
    QTC_ASSERT(ac.agent, return);

    if (response.resultClass == GdbResultDone) {
        DisassemblerLines dlines = parseDisassembler(response);
        if (dlines.coversAddress(ac.agent->address())) {
            ac.agent->setContents(dlines);
            return;
        }
    }
    fetchDisassemblerByCliPointPlain(ac);
}

void GdbEngine::handleFetchDisassemblerByCliPointPlain(const GdbResponse &response)
{
    DisassemblerAgentCookie ac = response.cookie.value<DisassemblerAgentCookie>();
    QTC_ASSERT(ac.agent, return);

    if (response.resultClass == GdbResultDone) {
        DisassemblerLines dlines = parseDisassembler(response);
        if (dlines.coversAddress(ac.agent->address())) {
            ac.agent->setContents(dlines);
            return;
        }
    }
    if (ac.agent->isMixed())
        fetchDisassemblerByCliRangeMixed(ac);
    else
        fetchDisassemblerByCliRangePlain(ac);
}

void GdbEngine::handleFetchDisassemblerByCliRangeMixed(const GdbResponse &response)
{
    DisassemblerAgentCookie ac = response.cookie.value<DisassemblerAgentCookie>();
    QTC_ASSERT(ac.agent, return);

    if (response.resultClass == GdbResultDone) {
        DisassemblerLines dlines = parseDisassembler(response);
        if (dlines.coversAddress(ac.agent->address())) {
            ac.agent->setContents(dlines);
            return;
        }
    }
    fetchDisassemblerByCliRangePlain(ac);
}

void GdbEngine::handleFetchDisassemblerByCliRangePlain(const GdbResponse &response)
{
    DisassemblerAgentCookie ac = response.cookie.value<DisassemblerAgentCookie>();
    QTC_ASSERT(ac.agent, return);

    if (response.resultClass == GdbResultDone) {
        DisassemblerLines dlines = parseDisassembler(response);
        if (dlines.size()) {
            ac.agent->setContents(dlines);
            return;
        }
    }

    // Finally, give up.
    //76^error,msg="No function contains program counter for selected..."
    //76^error,msg="No function contains specified address."
    //>568^error,msg="Line number 0 out of range;
    QByteArray msg = response.data.findChild("msg").data();
    showStatusMessage(tr("Disassembler failed: %1")
        .arg(QString::fromLocal8Bit(msg)), 5000);
}

// Binary/configuration check logic.

static QString gdbBinary(const DebuggerStartParameters &sp)
{
    // 1) Environment.
    const QByteArray envBinary = qgetenv("QTC_DEBUGGER_PATH");
    if (!envBinary.isEmpty())
        return QString::fromLocal8Bit(envBinary);
    // 2) Command explicitly specified.
    if (!sp.debuggerCommand.isEmpty()) {
        // Do not use a CDB binary if we got started for a project with MSVC runtime.
        const Abi abi = sp.toolChainAbi;
        if (abi.os() != Abi::WindowsOS || abi.osFlavor() == Abi::WindowsMSysFlavor)
            return sp.debuggerCommand;
    }
    // 3) Find one from tool chains.
    return debuggerCore()->debuggerForAbi(sp.toolChainAbi, GdbEngineType);
}

bool checkGdbConfiguration(const DebuggerStartParameters &sp, ConfigurationCheck *check)
{
    const QString binary = gdbBinary(sp);
    if (gdbBinary(sp).isEmpty()) {
        check->errorDetails.push_back(msgNoGdbBinaryForToolChain(sp.toolChainAbi));
        check->settingsCategory = _(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
        check->settingsPage = _(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
        return false;
    }
    if (sp.toolChainAbi.os() == Abi::WindowsOS &&  !QFileInfo(binary).isAbsolute()) {
    // See initialization below, we need an absolute path to be able to locate Python on Windows.
        check->errorDetails.push_back(GdbEngine::tr("The gdb location must be given as an "
                "absolute path in the debugger settings (%1).").arg(binary));
        check->settingsCategory = _(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
        check->settingsPage = _(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
        return false;
    }
    return true;
}

//
// Starting up & shutting down
//

bool GdbEngine::startGdb(const QStringList &args, const QString &settingsIdHint)
{
    const QByteArray tests = qgetenv("QTC_DEBUGGER_TESTS");
    foreach (const QByteArray &test, tests.split(','))
        m_testCases.insert(test.toInt());
    foreach (int test, m_testCases)
        showMessage(_("ENABLING TEST CASE: " + QByteArray::number(test)));

    gdbProc()->disconnect(); // From any previous runs

    const DebuggerStartParameters &sp = startParameters();
    m_gdb = gdbBinary(sp);
    if (m_gdb.isEmpty()) {
        handleAdapterStartFailed(
            msgNoGdbBinaryForToolChain(sp.toolChainAbi),
            _(Constants::DEBUGGER_COMMON_SETTINGS_ID));
        return false;
    }
    QStringList gdbArgs;
    gdbArgs << _("-i");
    gdbArgs << _("mi");
    if (!debuggerCore()->boolSetting(LoadGdbInit))
        gdbArgs << _("-n");
    gdbArgs += args;

    connect(gdbProc(), SIGNAL(error(QProcess::ProcessError)),
        SLOT(handleGdbError(QProcess::ProcessError)));
    connect(gdbProc(), SIGNAL(finished(int, QProcess::ExitStatus)),
        SLOT(handleGdbFinished(int, QProcess::ExitStatus)));
    connect(gdbProc(), SIGNAL(readyReadStandardOutput()),
        SLOT(readGdbStandardOutput()));
    connect(gdbProc(), SIGNAL(readyReadStandardError()),
        SLOT(readGdbStandardError()));

    showMessage(_("STARTING ") + m_gdb + _(" ") + gdbArgs.join(_(" ")));
    gdbProc()->start(m_gdb, gdbArgs);

    if (!gdbProc()->waitForStarted()) {
        const QString msg = errorMessage(QProcess::FailedToStart);
        handleAdapterStartFailed(msg, settingsIdHint);
        return false;
    }

    showMessage(_("GDB STARTED, INITIALIZING IT"));
    postCommand("show version", CB(handleShowVersion));
    postCommand("-list-features", CB(handleListFeatures));

    //postCommand("-enable-timings");
    //postCommand("set print static-members off"); // Seemingly doesn't work.
    //postCommand("set debug infrun 1");
    //postCommand("define hook-stop\n-thread-list-ids\n-stack-list-frames\nend");
    //postCommand("define hook-stop\nprint 4\nend");
    //postCommand("define hookpost-stop\nprint 5\nend");
    //postCommand("define hook-call\nprint 6\nend");
    //postCommand("define hookpost-call\nprint 7\nend");
    postCommand("set print object on");
    //postCommand("set step-mode on");  // we can't work with that yes
    //postCommand("set exec-done-display on");
    //postCommand("set print pretty on");
    //postCommand("set confirm off");
    //postCommand("set pagination off");

    // The following does not work with 6.3.50-20050815 (Apple version gdb-1344)
    // (Mac OS 10.6), but does so for gdb-966 (10.5):
    //postCommand("set print inferior-events 1");

    postCommand("set breakpoint pending on");
    postCommand("set print elements 10000");
    // Produces a few messages during symtab loading
    //postCommand("set verbose on");

    //postCommand("set substitute-path /var/tmp/qt-x11-src-4.5.0 "
    //    "/home/sandbox/qtsdk-2009.01/qt");

    // one of the following is needed to prevent crashes in gdb on code like:
    //  template <class T> T foo() { return T(0); }
    //  int main() { return foo<int>(); }
    //  (gdb) call 'int foo<int>'()
    //  /build/buildd/gdb-6.8/gdb/valops.c:2069: internal-error:
    postCommand("set overload-resolution off");
    //postCommand(_("set demangle-style none"));
    // From the docs:
    //  Stop means reenter debugger if this signal happens (implies print).
    //  Print means print a message if this signal happens.
    //  Pass means let program see this signal;
    //  otherwise program doesn't know.
    //  Pass and Stop may be combined.
    // We need "print" as otherwise we will get no feedback whatsoever
    // when Custom DebuggingHelper crash (which happen regularly when accessing
    // uninitialized variables).
    postCommand("handle SIGSEGV nopass stop print");

    // This is useful to kill the inferior whenever gdb dies.
    //postCommand(_("handle SIGTERM pass nostop print"));

    postCommand("set unwindonsignal on");
    postCommand("pwd");
    postCommand("set width 0");
    postCommand("set height 0");
    postCommand("set auto-solib-add on");

    if (0 && debuggerCore()->boolSetting(TargetAsync)) {
        postCommand("set target-async on");
        postCommand("set non-stop on");
    }

    // Work around https://bugreports.qt.nokia.com/browse/QTCREATORBUG-2004
    postCommand("maintenance set internal-warning quit no", ConsoleCommand);
    postCommand("maintenance set internal-error quit no", ConsoleCommand);

    postCommand("disassemble 0 0", ConsoleCommand, CB(handleDisassemblerCheck));

    if (sp.breakOnMain) {
        QByteArray cmd = "tbreak ";
        cmd += sp.toolChainAbi.os() == Abi::WindowsOS ? "qMain" : "main";
        postCommand(cmd);
    }

    loadPythonDumpers();

    return true;
}

void GdbEngine::loadInitScript()
{
    const QString script = startParameters().overrideStartScript;
    if (!script.isEmpty()) {
        if (QFileInfo(script).isReadable()) {
            postCommand("source " + script.toLocal8Bit());
        } else {
            showMessageBox(QMessageBox::Warning,
            tr("Cannot find debugger initialization script"),
            tr("The debugger settings point to a script file at '%1' "
               "which is not accessible. If a script file is not needed, "
               "consider clearing that entry to avoid this warning. "
              ).arg(script));
        }
    } else {
        const QString commands = debuggerCore()->stringSetting(GdbStartupCommands);
        if (!commands.isEmpty())
            postCommand(commands.toLocal8Bit());
    }
}

void GdbEngine::loadPythonDumpers()
{
    const QByteArray dumperSourcePath =
        Core::ICore::instance()->resourcePath().toLocal8Bit() + "/dumper/";

    postCommand("python execfile('" + dumperSourcePath + "bridge.py')",
        ConsoleCommand|NonCriticalResponse);
    postCommand("python execfile('" + dumperSourcePath + "dumper.py')",
        ConsoleCommand|NonCriticalResponse);
    postCommand("python execfile('" + dumperSourcePath + "qttypes.py')",
        ConsoleCommand|NonCriticalResponse);

    loadInitScript();

    postCommand("bbsetup",
        ConsoleCommand, CB(handleHasPython));
}

void GdbEngine::handleGdbError(QProcess::ProcessError error)
{
    const QString msg = errorMessage(error);
    showMessage(_("HANDLE GDB ERROR: ") + msg);
    // Show a message box for asynchronously reported issues.
    switch (error) {
    case QProcess::FailedToStart:
        // This should be handled by the code trying to start the process.
        break;
    case QProcess::Crashed:
        // This will get a processExited() as well.
        break;
    case QProcess::ReadError:
    case QProcess::WriteError:
    case QProcess::Timedout:
    default:
        //gdbProc()->kill();
        //notifyEngineIll();
        showMessageBox(QMessageBox::Critical, tr("GDB I/O Error"), msg);
        break;
    }
}

void GdbEngine::handleGdbFinished(int code, QProcess::ExitStatus type)
{
    if (m_commandTimer.isActive())
        m_commandTimer.stop();

    showMessage(_("GDB PROCESS FINISHED, status %1, code %2").arg(type).arg(code));

    switch (state()) {
    case EngineShutdownRequested:
        notifyEngineShutdownOk();
        break;
    case InferiorRunOk:
        // This could either be a real gdb crash or a quickly exited inferior
        // in the terminal adapter. In this case the stub proc will die soon,
        // too, so there's no need to act here.
        showMessage(_("The gdb process exited somewhat unexpectedly."));
        notifyEngineSpontaneousShutdown();
        break;
    default: {
        notifyEngineIll(); // Initiate shutdown sequence
        const QString msg = type == QProcess::CrashExit ?
                    tr("The gdb process crashed.") :
                    tr("The gdb process exited unexpectedly (code %1)").arg(code);
        showMessageBox(QMessageBox::Critical, tr("Unexpected GDB Exit"), msg);
        break;
    }
    }
}

void GdbEngine::abortDebugger()
{
    if (targetState() == DebuggerFinished) {
        // We already tried. Try harder.
        showMessage(_("ABORTING DEBUGGER. SECOND TIME."));
        QTC_ASSERT(m_gdbAdapter, return);
        QTC_ASSERT(m_gdbAdapter->gdbProc(), return);
        m_gdbAdapter->gdbProc()->kill();
    } else {
        // Be friendly the first time. This will change targetState().
        showMessage(_("ABORTING DEBUGGER. FIRST TIME."));
        quitDebugger();
    }
}

void GdbEngine::handleAdapterStartFailed(const QString &msg,
    const QString &settingsIdHint)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("ADAPTER START FAILED"));
    if (!msg.isEmpty()) {
        const QString title = tr("Adapter start failed");
        if (settingsIdHint.isEmpty()) {
            Core::ICore::instance()->showWarningWithOptions(title, msg);
        } else {
            Core::ICore::instance()->showWarningWithOptions(title, msg, QString(),
                _(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY), settingsIdHint);
        }
    }
    notifyEngineSetupFailed();
}

void GdbEngine::handleAdapterStarted()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("ADAPTER SUCCESSFULLY STARTED"));
    notifyEngineSetupOk();
}

void GdbEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    typedef GlobalDebuggerOptions::SourcePathMap SourcePathMap;
    typedef SourcePathMap::const_iterator SourcePathMapIterator;

    showStatusMessage(tr("Setting up inferior..."));
    const DebuggerStartParameters &sp = startParameters();

    // Apply source path mappings from global options.
    const SourcePathMap sourcePathMap =
        DebuggerSourcePathMappingWidget::mergePlatformQtPath(sp.qtInstallPath,
                debuggerCore()->globalDebuggerOptions()->sourcePathMap);
    const SourcePathMapIterator cend = sourcePathMap.constEnd();
    SourcePathMapIterator it = sourcePathMap.constBegin();
    for ( ; it != cend; ++it)
        postCommand("set substitute-path " + it.key().toLocal8Bit()
            + " " + it.value().toLocal8Bit());

    const QByteArray debugInfoLocation = sp.debugInfoLocation.toLocal8Bit();
    if (!debugInfoLocation.isEmpty())
        postCommand("set debug-file-directory " + debugInfoLocation);

    // Spaces just will not work.
    foreach (const QString &src, sp.debugSourceLocation)
        postCommand("directory " + src.toLocal8Bit());

    //QByteArray ba = QFileInfo(sp.dumperLibrary).path().toLocal8Bit();
    //if (!ba.isEmpty())
    //    postCommand("set solib-search-path " + ba);

    m_gdbAdapter->setupInferior();
}

void GdbEngine::notifyInferiorSetupFailed()
{
    // FIXME: that's not enough to stop gdb from getting confused
    // by a timeout of the adapter.
    //resetCommandQueue();
    DebuggerEngine::notifyInferiorSetupFailed();
}

void GdbEngine::handleInferiorPrepared()
{
    const DebuggerStartParameters &sp = startParameters();

    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());

    // Initial attempt to set breakpoints.
    if (sp.startMode != AttachCore) {
        showStatusMessage(tr("Setting breakpoints..."));
        showMessage(tr("Setting breakpoints..."));
        attemptBreakpointSynchronization();
    }

    if (m_cookieForToken.isEmpty()) {
        finishInferiorSetup();
    } else {
        QTC_CHECK(m_commandsDoneCallback == 0);
        m_commandsDoneCallback = &GdbEngine::finishInferiorSetup;
    }
}

void GdbEngine::finishInferiorSetup()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    // Extract Qt namespace.
    QString fileName;
    {
        QTemporaryFile symbols(QDir::tempPath() + _("/gdb_ns_"));
        symbols.open();
        fileName = symbols.fileName();
    }
    postCommand("maint print msymbols " + fileName.toLocal8Bit(),
        CB(handleNamespaceExtraction), fileName);
}

void GdbEngine::handleNamespaceExtraction(const GdbResponse &response)
{
    QFile file(response.cookie.toString());
    file.open(QIODevice::ReadOnly);
    QByteArray ba = file.readAll();
    file.close();
    file.remove();
    int pos = ba.indexOf("7QString9fromAscii");
    int pos1 = pos - 1;
    while (pos1 > 0 && ba.at(pos1) != 'N' && ba.at(pos1) > '@')
        --pos1;
    ++pos1;
    const QByteArray ns = ba.mid(pos1, pos - pos1);
    if (ns.isEmpty()) {
        showMessage(_("FOUND NON-NAMESPACED QT"));
    } else {
        showMessage(_("FOUND NAMESPACED QT: " + ns));
        setQtNamespace(ns + "::");
    }

    if (startParameters().startMode == AttachCore) {
        notifyInferiorSetupOk(); // No breakpoints in core files.
    } else {
        if (debuggerCore()->boolSetting(BreakOnWarning))
            postCommand("-break-insert -f '" + qtNamespace() + "qWarning'");
        if (debuggerCore()->boolSetting(BreakOnFatal))
            postCommand("-break-insert -f '" + qtNamespace() + "qFatal'",
                CB(handleBreakOnQFatal));
        else
            notifyInferiorSetupOk();
    }
}

void GdbEngine::handleBreakOnQFatal(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        GdbMi bkpt = response.data.findChild("bkpt");
        GdbMi number = bkpt.findChild("number");
        BreakpointResponseId rid(number.data());
        if (rid.isValid()) {
            m_qFatalBreakpointResponseId = rid;
            postCommand("-break-commands " + number.data() + " return");
        }
    }

    // Continue setup.
    notifyInferiorSetupOk();
}

void GdbEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    m_gdbAdapter->runEngine();
}

void GdbEngine::notifyInferiorSetupFailed(const QString &msg)
{
    showStatusMessage(tr("Failed to start application: ") + msg);
    if (state() == EngineSetupFailed) {
        showMessage(_("INFERIOR START FAILED, BUT ADAPTER DIED ALREADY"));
        return; // Adapter crashed meanwhile, so this notification is meaningless.
    }
    showMessage(_("INFERIOR START FAILED"));
    showMessageBox(QMessageBox::Critical, tr("Failed to start application"), msg);
    DebuggerEngine::notifyInferiorSetupFailed();
}

void GdbEngine::handleAdapterCrashed(const QString &msg)
{
    showMessage(_("ADAPTER CRASHED"));

    // The adapter is expected to have cleaned up after itself when we get here,
    // so the effect is about the same as AdapterStartFailed => use it.
    // Don't bother with state transitions - this can happen in any state and
    // the end result is always the same, so it makes little sense to find a
    // "path" which does not assert.
    if (state() == EngineSetupRequested)
        notifyEngineSetupFailed();
    else
        notifyEngineIll();

    // No point in being friendly here ...
    gdbProc()->kill();

    if (!msg.isEmpty())
        showMessageBox(QMessageBox::Critical, tr("Adapter crashed"), msg);
}

void GdbEngine::setUseDebuggingHelpers(const QVariant &)
{
    setTokenBarrier();
    updateLocals();
}

bool GdbEngine::hasPython() const
{
    return m_hasPython;
}

void GdbEngine::createFullBacktrace()
{
    postCommand("thread apply all bt full",
        NeedsStop|ConsoleCommand, CB(handleCreateFullBacktrace));
}

void GdbEngine::handleCreateFullBacktrace(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        debuggerCore()->openTextEditor(_("Backtrace $"),
            _(response.consoleStreamOutput + response.logStreamOutput));
    }
}

void GdbEngine::resetCommandQueue()
{
    m_commandTimer.stop();
    if (!m_cookieForToken.isEmpty()) {
        QString msg;
        QTextStream ts(&msg);
        ts << "RESETING COMMAND QUEUE. LEFT OVER TOKENS: ";
        foreach (const GdbCommand &cookie, m_cookieForToken)
            ts << "CMD:" << cookie.command << cookie.callbackName;
        m_cookieForToken.clear();
        showMessage(msg);
    }
}

void GdbEngine::handleRemoteSetupDone(int gdbServerPort, int qmlPort)
{
    m_gdbAdapter->handleRemoteSetupDone(gdbServerPort, qmlPort);
}

void GdbEngine::handleRemoteSetupFailed(const QString &message)
{
    m_gdbAdapter->handleRemoteSetupFailed(message);
}

bool GdbEngine::setupQmlStep(bool on)
{
    QTC_ASSERT(isSlaveEngine(), return false);
    m_qmlBreakpointResponseId1 = BreakpointResponseId();
    m_qmlBreakpointResponseId2 = BreakpointResponseId();
    //qDebug() << "CLEAR: " << m_qmlBreakpointNumbers;
    postCommand("tbreak '" + qtNamespace() + "QScript::FunctionWrapper::proxyCall'\n"
        "commands\n"
        "set $d=(void*)((FunctionWrapper*)callee)->data->function\n"
        "tbreak *$d\nprintf \"QMLBP:%d \\n\",$bpnum\ncontinue\nend",
        NeedsStop, CB(handleSetQmlStepBreakpoint));
    m_preparedForQmlBreak = on;
    return true;
}

void GdbEngine::handleSetQmlStepBreakpoint(const GdbResponse &response)
{
    //QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        // logStreamOutput: "tbreak 'myns::QScript::FunctionWrapper::proxyCall'\n"
        // consoleStreamOutput: "Temporary breakpoint 1 at 0xf166e7:
        // file bridge/qscriptfunction.cpp, line 75.\n"}
        QByteArray ba = parsePlainConsoleStream(response);
        const int pos2 = ba.indexOf(" at 0x");
        const int pos1 = ba.lastIndexOf(" ", pos2 - 1) + 1;
        QByteArray mid = ba.mid(pos1, pos2 - pos1);
        m_qmlBreakpointResponseId1 = BreakpointResponseId(mid);
        //qDebug() << "SET: " << m_qmlBreakpointResponseId1;
    }
    QTC_ASSERT(masterEngine(), return);
    masterEngine()->readyToExecuteQmlStep();
}

bool GdbEngine::isQmlStepBreakpoint(const BreakpointResponseId &id) const
{
    return isQmlStepBreakpoint1(id) || isQmlStepBreakpoint2(id);
}

bool GdbEngine::isQmlStepBreakpoint1(const BreakpointResponseId &id) const
{
    return id.isValid() && m_qmlBreakpointResponseId1 == id;
}

bool GdbEngine::isQmlStepBreakpoint2(const BreakpointResponseId &id) const
{
    return id.isValid() && m_qmlBreakpointResponseId2 == id;
}

bool GdbEngine::isQFatalBreakpoint(const BreakpointResponseId &id) const
{
    return id.isValid() && m_qFatalBreakpointResponseId == id;
}

bool GdbEngine::isHiddenBreakpoint(const BreakpointResponseId &id) const
{
    return isQFatalBreakpoint(id) || isQmlStepBreakpoint(id);
}

void GdbEngine::scheduleTestResponse(int testCase, const QByteArray &response)
{
    if (!m_testCases.contains(testCase))
        return;

    int token = currentToken() + 1;
    showMessage(_("SCHEDULING TEST RESPONSE (CASE: %1, TOKEN: %2, RESPONSE: '%3')")
        .arg(testCase).arg(token).arg(_(response)));
    m_scheduledTestResponses[token] = response;
}

//
// Factory
//

DebuggerEngine *createGdbEngine(const DebuggerStartParameters &startParameters,
    DebuggerEngine *masterEngine)
{
    return new GdbEngine(startParameters, masterEngine);
}

void addGdbOptionPages(QList<Core::IOptionsPage *> *opts)
{
    opts->push_back(new GdbOptionsPage());
}

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::MemoryAgentCookie)
Q_DECLARE_METATYPE(Debugger::Internal::DisassemblerAgentCookie)
Q_DECLARE_METATYPE(Debugger::Internal::GdbMi)
