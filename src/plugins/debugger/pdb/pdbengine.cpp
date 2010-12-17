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

#define QT_NO_CAST_FROM_ASCII

#include "pdbengine.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggerplugin.h"
#include "debuggerstringutils.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"

#include "../gdb/gdbmi.h"

#include <utils/qtcassert.h>

#include <texteditor/itexteditor.h>
#include <coreplugin/ifile.h>
#include <coreplugin/icore.h>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtCore/QVariant>

#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QToolTip>


#define DEBUG_SCRIPT 1
#if DEBUG_SCRIPT
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s


#define CB(callback) &PdbEngine::callback, STRINGIFY(callback)

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// PdbEngine
//
///////////////////////////////////////////////////////////////////////

PdbEngine::PdbEngine(const DebuggerStartParameters &startParameters)
    : DebuggerEngine(startParameters)
{
    setObjectName(QLatin1String("PdbEngine"));
}

PdbEngine::~PdbEngine()
{}

void PdbEngine::executeDebuggerCommand(const QString &command)
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    //XSDEBUG("PdbEngine::executeDebuggerCommand:" << command);
    if (state() == DebuggerNotReady) {
        showMessage(_("PDB PROCESS NOT RUNNING, PLAIN CMD IGNORED: ") + command);
        return;
    }
    QTC_ASSERT(m_pdbProc.state() == QProcess::Running, notifyEngineIll());
    postCommand(command.toLatin1(), CB(handleExecuteDebuggerCommand));
}

void PdbEngine::handleExecuteDebuggerCommand(const PdbResponse &response)
{
    Q_UNUSED(response);
}

void PdbEngine::postDirectCommand(const QByteArray &command)
{
    QTC_ASSERT(m_pdbProc.state() == QProcess::Running, notifyEngineIll());
    showMessage(_(command), LogInput);
    m_pdbProc.write(command + "\n");
}

void PdbEngine::postCommand(const QByteArray &command,
//                 PdbCommandFlags flags,
                 PdbCommandCallback callback,
                 const char *callbackName,
                 const QVariant &cookie)
{
    QTC_ASSERT(m_pdbProc.state() == QProcess::Running, notifyEngineIll());
    PdbCommand cmd;
    cmd.command = command;
    cmd.callback = callback;
    cmd.callbackName = callbackName;
    cmd.cookie = cookie;
    m_commands.enqueue(cmd);
    qDebug() << "ENQUEUE: " << command << cmd.callbackName;
    showMessage(_(cmd.command), LogInput);
    m_pdbProc.write(cmd.command + "\n");
}

void PdbEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    notifyInferiorShutdownOk();
}

void PdbEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    m_pdbProc.kill();
}

void PdbEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

    m_pdb = _("python");
    showMessage(_("STARTING PDB ") + m_pdb);

    connect(&m_pdbProc, SIGNAL(error(QProcess::ProcessError)),
        SLOT(handlePdbError(QProcess::ProcessError)));
    connect(&m_pdbProc, SIGNAL(finished(int, QProcess::ExitStatus)),
        SLOT(handlePdbFinished(int, QProcess::ExitStatus)));
    connect(&m_pdbProc, SIGNAL(readyReadStandardOutput()),
        SLOT(readPdbStandardOutput()));
    connect(&m_pdbProc, SIGNAL(readyReadStandardError()),
        SLOT(readPdbStandardError()));

    connect(this, SIGNAL(outputReady(QByteArray)),
        SLOT(handleOutput2(QByteArray)), Qt::QueuedConnection);

    // We will stop immediately, so setup a proper callback.
    PdbCommand cmd;
    cmd.callback = &PdbEngine::handleFirstCommand;
    m_commands.enqueue(cmd);

    m_pdbProc.start(m_pdb, QStringList() << _("-i"));

    if (!m_pdbProc.waitForStarted()) {
        const QString msg = tr("Unable to start pdb '%1': %2")
            .arg(m_pdb, m_pdbProc.errorString());
        notifyEngineSetupFailed();
        showMessage(_("ADAPTER START FAILED"));
        if (!msg.isEmpty()) {
            const QString title = tr("Adapter start failed");
            Core::ICore::instance()->showWarningWithOptions(title, msg);
        }
        notifyEngineSetupFailed();
        return;
    }
    notifyEngineSetupOk();
}

void PdbEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());

    QString fileName = QFileInfo(startParameters().executable).absoluteFilePath();
    QFile scriptFile(fileName);
    if (!scriptFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        showMessageBox(QMessageBox::Critical, tr("Python Error"),
            _("Cannot open script file %1:\n%2").
               arg(fileName, scriptFile.errorString()));
        notifyInferiorSetupFailed();
        return;
    }
    notifyInferiorSetupOk();
}

void PdbEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    showStatusMessage(tr("Running requested..."), 5000);
    const QByteArray dumperSourcePath =
        Core::ICore::instance()->resourcePath().toLocal8Bit() + "/gdbmacros/";
    QString fileName = QFileInfo(startParameters().executable).absoluteFilePath();
    postDirectCommand("import sys");
    postDirectCommand("sys.argv.append('" + fileName.toLocal8Bit() + "')");
    postDirectCommand("execfile('/usr/bin/pdb')");
    postDirectCommand("execfile('" + dumperSourcePath + "pdumper.py')");
    attemptBreakpointSynchronization();
    notifyEngineRunAndInferiorStopOk();
    continueInferior();
}

void PdbEngine::interruptInferior()
{
    notifyInferiorStopOk();
}

void PdbEngine::executeStep()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postCommand("step", CB(handleUpdateAll));
}

void PdbEngine::executeStepI()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postCommand("step", CB(handleUpdateAll));
}

void PdbEngine::executeStepOut()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postCommand("finish", CB(handleUpdateAll));
}

void PdbEngine::executeNext()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postCommand("next", CB(handleUpdateAll));
}

void PdbEngine::executeNextI()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postCommand("next", CB(handleUpdateAll));
}

void PdbEngine::continueInferior()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    // Callback will be triggered e.g. when breakpoint is hit.
    postCommand("continue", CB(handleUpdateAll));
}

void PdbEngine::executeRunToLine(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName)
    Q_UNUSED(lineNumber)
    SDEBUG("FIXME:  PdbEngine::runToLineExec()");
}

void PdbEngine::executeRunToFunction(const QString &functionName)
{
    Q_UNUSED(functionName)
    XSDEBUG("FIXME:  PdbEngine::runToFunctionExec()");
}

void PdbEngine::executeJumpToLine(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName)
    Q_UNUSED(lineNumber)
    XSDEBUG("FIXME:  PdbEngine::jumpToLineExec()");
}

void PdbEngine::activateFrame(int frameIndex)
{
    resetLocation();
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    StackHandler *handler = stackHandler();
    int oldIndex = handler->currentIndex();

    //if (frameIndex == handler->stackSize()) {
    //    reloadFullStack();
    //    return;
    //}

    QTC_ASSERT(frameIndex < handler->stackSize(), return);

    if (oldIndex != frameIndex) {
        // Assuming the command always succeeds this saves a roundtrip.
        // Otherwise the lines below would need to get triggered
        // after a response to this -stack-select-frame here.
        handler->setCurrentIndex(frameIndex);
        //postCommand("-stack-select-frame " + QByteArray::number(frameIndex),
        //    CB(handleStackSelectFrame));
    }
    gotoLocation(handler->currentFrame());
}

void PdbEngine::selectThread(int index)
{
    Q_UNUSED(index)
}

bool PdbEngine::acceptsBreakpoint(BreakpointId id) const
{
    const QString fileName = breakHandler()->fileName(id);
    return fileName.endsWith(QLatin1String(".py"));
}

void PdbEngine::insertBreakpoint(BreakpointId id)
{
    BreakHandler *handler = breakHandler();
    QTC_ASSERT(handler->state(id) == BreakpointInsertRequested, /**/);
    handler->notifyBreakpointInsertProceeding(id);

    QByteArray loc;
    if (handler->type(id) == BreakpointByFunction)
        loc = handler->functionName(id).toLatin1();
    else
        loc = handler->fileName(id).toLocal8Bit() + ':'
         + QByteArray::number(handler->lineNumber(id));

    postCommand("break " + loc, CB(handleBreakInsert), QVariant(id));
}

void PdbEngine::handleBreakInsert(const PdbResponse &response)
{
    //qDebug() << "BP RESPONSE: " << response.data;
    // "Breakpoint 1 at /pdb/math.py:10"
    BreakpointId id(response.cookie.toInt());
    BreakHandler *handler = breakHandler();
    QTC_ASSERT(response.data.startsWith("Breakpoint "), return);
    int pos1 = response.data.indexOf(" at ");
    QTC_ASSERT(pos1 != -1, return);
    QByteArray bpnr = response.data.mid(11, pos1 - 11);
    int pos2 = response.data.lastIndexOf(":");
    QByteArray file = response.data.mid(pos1 + 4, pos2 - pos1 - 4);
    QByteArray line = response.data.mid(pos2 + 1);
    BreakpointResponse br;
    br.number = bpnr.toInt();
    br.fileName = _(file);
    br.lineNumber = line.toInt();
    handler->setResponse(id, br);
}

void PdbEngine::removeBreakpoint(BreakpointId id)
{
    BreakHandler *handler = breakHandler();
    QTC_ASSERT(handler->state(id) == BreakpointRemoveRequested, /**/);
    handler->notifyBreakpointRemoveProceeding(id);
    BreakpointResponse br = handler->response(id);
    showMessage(_("DELETING BP %1 IN %2").arg(br.number)
        .arg(handler->fileName(id)));
    postCommand("clear " + QByteArray::number(br.number));
    // Pretend it succeeds without waiting for response.
    handler->notifyBreakpointRemoveOk(id);
}

void PdbEngine::loadSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

void PdbEngine::loadAllSymbols()
{
}

void PdbEngine::reloadModules()
{
    //postCommand("qdebug('listmodules')", CB(handleListModules));
}

void PdbEngine::handleListModules(const PdbResponse &response)
{
    GdbMi out;
    out.fromString(response.data.trimmed());
    Modules modules;
    foreach (const GdbMi &item, out.children()) {
        Module module;
        module.moduleName = _(item.findChild("name").data());
        QString path = _(item.findChild("value").data());
        int pos = path.indexOf(_("' from '"));
        if (pos != -1) {
            path = path.mid(pos + 8);
            if (path.size() >= 2)
                path.chop(2);
        } else if (path.startsWith(_("<module '"))
                && path.endsWith(_("' (built-in)>"))) {
            path = _("(builtin)");
        }
        module.modulePath = path;
        modules.append(module);
    }
    modulesHandler()->setModules(modules);
}

void PdbEngine::requestModuleSymbols(const QString &moduleName)
{
    postCommand("qdebug('listsymbols','" + moduleName.toLatin1() + "')",
        CB(handleListSymbols), moduleName);
}

void PdbEngine::handleListSymbols(const PdbResponse &response)
{
    GdbMi out;
    out.fromString(response.data.trimmed());
    Symbols symbols;
    QString moduleName = response.cookie.toString();
    foreach (const GdbMi &item, out.children()) {
        Symbol symbol;
        symbol.name = _(item.findChild("name").data());
        symbols.append(symbol);
    }
   debuggerCore()->showModuleSymbols(moduleName, symbols);
}

//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////


static WatchData m_toolTip;
static QPoint m_toolTipPos;
static QHash<QString, WatchData> m_toolTipCache;

void PdbEngine::setToolTipExpression(const QPoint &mousePos,
    TextEditor::ITextEditor *editor, int cursorPos)
{
    Q_UNUSED(mousePos)
    Q_UNUSED(editor)
    Q_UNUSED(cursorPos)

    if (state() != InferiorStopOk) {
        //SDEBUG("SUPPRESSING DEBUGGER TOOLTIP, INFERIOR NOT STOPPED");
        return;
    }
    // Check mime type and get expression (borrowing some C++ - functions)
    const QString javaPythonMimeType =
        QLatin1String("application/javascript");
    if (!editor->file() || editor->file()->mimeType() != javaPythonMimeType)
        return;

    int line;
    int column;
    QString exp = cppExpressionAt(editor, cursorPos, &line, &column);

/*
    if (m_toolTipCache.contains(exp)) {
        const WatchData & data = m_toolTipCache[exp];
        q->watchHandler()->removeChildren(data.iname);
        insertData(data);
        return;
    }
*/

    QToolTip::hideText();
    if (exp.isEmpty() || exp.startsWith(QLatin1Char('#')))  {
        QToolTip::hideText();
        return;
    }

    if (!hasLetterOrNumber(exp)) {
        QToolTip::showText(m_toolTipPos, tr("'%1' contains no identifier").arg(exp));
        return;
    }

    if (exp.startsWith(QLatin1Char('"')) && exp.endsWith(QLatin1Char('"'))) {
        QToolTip::showText(m_toolTipPos, tr("String literal %1").arg(exp));
        return;
    }

    if (exp.startsWith(QLatin1String("++")) || exp.startsWith(QLatin1String("--")))
        exp.remove(0, 2);

    if (exp.endsWith(QLatin1String("++")) || exp.endsWith(QLatin1String("--")))
        exp.remove(0, 2);

    if (exp.startsWith(QLatin1Char('<')) || exp.startsWith(QLatin1Char('[')))
        return;

    if (hasSideEffects(exp)) {
        QToolTip::showText(m_toolTipPos,
            tr("Cowardly refusing to evaluate expression '%1' "
               "with potential side effects").arg(exp));
        return;
    }

#if 0
    //if (status() != InferiorStopOk)
    //    return;

    // FIXME: 'exp' can contain illegal characters
    m_toolTip = WatchData();
    m_toolTip.exp = exp;
    m_toolTip.name = exp;
    m_toolTip.iname = tooltipIName;
    insertData(m_toolTip);
#endif
}


//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void PdbEngine::assignValueInDebugger(const Internal::WatchData *, const QString &expression, const QVariant &value)
{
    Q_UNUSED(expression);
    Q_UNUSED(value);
    SDEBUG("ASSIGNING: " << (expression + QLatin1Char('=') + value.toString()));
#if 0
    m_scriptEngine->evaluate(expression + QLatin1Char('=') + value.toString());
    updateLocals();
#endif
}


void PdbEngine::updateWatchData(const WatchData &data, const WatchUpdateFlags &flags)
{
    Q_UNUSED(data);
    Q_UNUSED(flags);
    updateAll();
}

void PdbEngine::handlePdbError(QProcess::ProcessError error)
{
    qDebug() << "HANDLE PDB ERROR";
    showMessage(_("HANDLE PDB ERROR"));
    switch (error) {
    case QProcess::Crashed:
        break; // will get a processExited() as well
    // impossible case QProcess::FailedToStart:
    case QProcess::ReadError:
    case QProcess::WriteError:
    case QProcess::Timedout:
    default:
        //setState(EngineShutdownRequested, true);
        m_pdbProc.kill();
        showMessageBox(QMessageBox::Critical, tr("Pdb I/O Error"),
                       errorMessage(error));
        break;
    }
}

QString PdbEngine::errorMessage(QProcess::ProcessError error) const
{
    switch (error) {
        case QProcess::FailedToStart:
            return tr("The Pdb process failed to start. Either the "
                "invoked program '%1' is missing, or you may have insufficient "
                "permissions to invoke the program.")
                .arg(m_pdb);
        case QProcess::Crashed:
            return tr("The Pdb process crashed some time after starting "
                "successfully.");
        case QProcess::Timedout:
            return tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
        case QProcess::WriteError:
            return tr("An error occurred when attempting to write "
                "to the Pdb process. For example, the process may not be running, "
                "or it may have closed its input channel.");
        case QProcess::ReadError:
            return tr("An error occurred when attempting to read from "
                "the Pdb process. For example, the process may not be running.");
        default:
            return tr("An unknown error in the Pdb process occurred. ");
    }
}

void PdbEngine::handlePdbFinished(int code, QProcess::ExitStatus type)
{
    qDebug() << "PDB FINISHED";
    showMessage(_("PDB PROCESS FINISHED, status %1, code %2").arg(type).arg(code));
    notifyEngineSpontaneousShutdown();
}

void PdbEngine::readPdbStandardError()
{
    QByteArray err = m_pdbProc.readAllStandardError();
    qDebug() << "\nPDB STDERR" << err;
    //qWarning() << "Unexpected pdb stderr:" << err;
    //showMessage(_("Unexpected pdb stderr: " + err));
    //handleOutput(err);
}

void PdbEngine::readPdbStandardOutput()
{
    QByteArray out = m_pdbProc.readAllStandardOutput();
    qDebug() << "\nPDB STDOUT" << out;
    handleOutput(out);
}

void PdbEngine::handleOutput(const QByteArray &data)
{
    //qDebug() << "READ: " << data;
    m_inbuffer.append(data);
    qDebug() << "BUFFER FROM: '" << m_inbuffer << "'";
    while (true) {
        int pos = m_inbuffer.indexOf("(Pdb)");
        if (pos == -1)
            pos = m_inbuffer.indexOf(">>>");
        if (pos == -1)
            break;
        QByteArray response = m_inbuffer.left(pos).trimmed();
        m_inbuffer = m_inbuffer.mid(pos + 6);
        emit outputReady(response);
    }
    qDebug() << "BUFFER LEFT: '" << m_inbuffer << "'";
    //m_inbuffer.clear();
}


void PdbEngine::handleOutput2(const QByteArray &data)
{
    PdbResponse response;
    response.data = data;
    showMessage(_(data));
    QTC_ASSERT(!m_commands.isEmpty(), qDebug() << "RESPONSE: " << data; return)
    PdbCommand cmd = m_commands.dequeue();
    response.cookie = cmd.cookie;
    qDebug() << "DEQUE: " << cmd.command << cmd.callbackName;
    if (cmd.callback) {
        //qDebug() << "EXECUTING CALLBACK " << cmd.callbackName
        //    << " RESPONSE: " << response.data;
        (this->*cmd.callback)(response);
    } else {
        qDebug() << "NO CALLBACK FOR RESPONSE: " << response.data;
    }
}
/*
void PdbEngine::handleResponse(const QByteArray &response0)
{
    QByteArray response = response0;
    qDebug() << "RESPONSE: '" << response << "'";
    if (response.startsWith("--Call--")) {
        qDebug() << "SKIPPING '--Call--' MARKER";
        response = response.mid(9);
    }
    if (response.startsWith("--Return--")) {
        qDebug() << "SKIPPING '--Return--' MARKER";
        response = response.mid(11);
    }
    if (response.startsWith("> ")) {
        int pos1 = response.indexOf('(');
        int pos2 = response.indexOf(')', pos1);
        if (pos1 != -1 && pos2 != -1) {
            int lineNumber = response.mid(pos1 + 1, pos2 - pos1 - 1).toInt();
            QByteArray fileName = response.mid(2, pos1 - 2);
            qDebug() << " " << pos1 << pos2 << lineNumber << fileName
                << response.mid(pos1 + 1, pos2 - pos1 - 1);
            StackFrame frame;
            frame.file = _(fileName);
            frame.line = lineNumber;
            if (frame.line > 0 && QFileInfo(frame.file).exists()) {
                gotoLocation(frame);
                notifyInferiorSpontaneousStop();
                return;
            }
        }
    }
    qDebug() << "COULD NOT PARSE RESPONSE: '" << response << "'";
}
*/

void PdbEngine::handleFirstCommand(const PdbResponse &response)
{
    Q_UNUSED(response);
}

void PdbEngine::handleUpdateAll(const PdbResponse &response)
{
    Q_UNUSED(response);
    notifyInferiorSpontaneousStop();
    updateAll();
}

void PdbEngine::updateAll()
{
    postCommand("bt", CB(handleBacktrace));
    //updateLocals();
}

void PdbEngine::updateLocals()
{
    WatchHandler *handler = watchHandler();

    QByteArray watchers;
    //if (!m_toolTipExpression.isEmpty())
    //    watchers += m_toolTipExpression.toLatin1()
    //        + '#' + tooltipINameForExpression(m_toolTipExpression.toLatin1());

    QHash<QByteArray, int> watcherNames = handler->watcherNames();
    QHashIterator<QByteArray, int> it(watcherNames);
    while (it.hasNext()) {
        it.next();
        if (!watchers.isEmpty())
            watchers += "##";
        watchers += it.key() + "#watch." + QByteArray::number(it.value());
    }

    QByteArray options;
    if (debuggerCore()->boolSetting(UseDebuggingHelpers))
        options += "fancy,";
    if (debuggerCore()->boolSetting(AutoDerefPointers))
        options += "autoderef,";
    if (options.isEmpty())
        options += "defaults,";
    options.chop(1);

    postCommand("qdebug('" + options + "','"
        + handler->expansionRequests() + "','"
        + handler->typeFormatRequests() + "','"
        + handler->individualFormatRequests() + "','"
        + watchers.toHex() + "')", CB(handleListLocals));
}

void PdbEngine::handleBacktrace(const PdbResponse &response)
{
    //qDebug() << " BACKTRACE: '" << response.data << "'";
    // "  /usr/lib/python2.6/bdb.py(368)run()"
    // "-> exec cmd in globals, locals"
    // "  <string>(1)<module>()"
    // "  /python/math.py(19)<module>()"
    // "-> main()"
    // "  /python/math.py(14)main()"
    // "-> print cube(3)"
    // "  /python/math.py(7)cube()"
    // "-> x = square(a)"
    // "> /python/math.py(2)square()"
    // "-> def square(a):"

    // Populate stack view.
    StackFrames stackFrames;
    int level = 0;
    int currentIndex = -1;
    foreach (const QByteArray &line, response.data.split('\n')) {
        //qDebug() << "  LINE: '" << line << "'";
        if (line.startsWith("> ") || line.startsWith("  ")) {
            int pos1 = line.indexOf('(');
            int pos2 = line.indexOf(')', pos1);
            if (pos1 != -1 && pos2 != -1) {
                int lineNumber = line.mid(pos1 + 1, pos2 - pos1 - 1).toInt();
                QByteArray fileName = line.mid(2, pos1 - 2);
                //qDebug() << " " << pos1 << pos2 << lineNumber << fileName
                //    << line.mid(pos1 + 1, pos2 - pos1 - 1);
                StackFrame frame;
                frame.file = _(fileName);
                frame.line = lineNumber;
                frame.function = _(line.mid(pos2 + 1));
                if (frame.line > 0 && QFileInfo(frame.file).exists()) {
                    if (line.startsWith("> "))
                        currentIndex = level;
                    frame.level = level;
                    stackFrames.prepend(frame);
                    ++level;
                }
            }
        }
    }
    const int frameCount = stackFrames.size();
    for (int i = 0; i != frameCount; ++i)
        stackFrames[i].level = frameCount - stackFrames[i].level - 1;
    stackHandler()->setFrames(stackFrames);

    // Select current frame.
    if (currentIndex != -1) {
        currentIndex = frameCount - currentIndex - 1;
        stackHandler()->setCurrentIndex(currentIndex);
        gotoLocation(stackFrames.at(currentIndex));
    }

    updateLocals();
}

void PdbEngine::handleListLocals(const PdbResponse &response)
{
    //qDebug() << " LOCALS: '" << response.data << "'";
    QByteArray out = response.data.trimmed();

    GdbMi all;
    all.fromStringMultiple(out);
    //qDebug() << "ALL: " << all.toString();

    //GdbMi data = all.findChild("data");
    QList<WatchData> list;
    WatchHandler *handler = watchHandler();
    foreach (const GdbMi &child, all.children()) {
        WatchData dummy;
        dummy.iname = child.findChild("iname").data();
        dummy.name = _(child.findChild("name").data());
        //qDebug() << "CHILD: " << child.toString();
        parseWatchData(handler->expandedINames(), dummy, child, &list);
    }
    handler->insertBulkData(list);
}

unsigned PdbEngine::debuggerCapabilities() const
{
    return ReloadModuleCapability;
}

DebuggerEngine *createPdbEngine(const DebuggerStartParameters &startParameters)
{
    return new PdbEngine(startParameters);
}


} // namespace Internal
} // namespace Debugger
