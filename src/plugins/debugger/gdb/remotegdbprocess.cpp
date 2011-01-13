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

#include "remotegdbprocess.h"

#include "remoteplaingdbadapter.h"

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QtCore/QFileInfo>

#include <ctype.h>

using namespace Core;

namespace Debugger {
namespace Internal {

RemoteGdbProcess::RemoteGdbProcess(const Core::SshConnectionParameters &connParams,
    RemotePlainGdbAdapter *adapter, QObject *parent)
    : AbstractGdbProcess(parent), m_connParams(connParams),
      m_state(Inactive), m_adapter(adapter)
{
}

QByteArray RemoteGdbProcess::readAllStandardOutput()
{
    QByteArray output = m_gdbOutput;
    m_gdbOutput.clear();
    return output;
}

QByteArray RemoteGdbProcess::readAllStandardError()
{
    QByteArray errorOutput = m_errorOutput;
    m_errorOutput.clear();
    return errorOutput;
}

void RemoteGdbProcess::start(const QString &cmd, const QStringList &args)
{
    Q_UNUSED(cmd);
    Q_UNUSED(args);
    QTC_ASSERT(m_state == RunningGdb, return);
}

void RemoteGdbProcess::realStart(const QString &cmd, const QStringList &args,
    const QString &executableFilePath)
{
    QTC_ASSERT(m_state == Inactive, return);
    setState(Connecting);

    m_command = cmd;
    m_cmdArgs = args;
    m_appOutputFileName = "app_output_"
        + QFileInfo(executableFilePath).fileName().toUtf8();
    m_error.clear();
    m_lastSeqNr.clear();
    m_currentGdbOutput.clear();
    m_gdbOutput.clear();
    m_errorOutput.clear();
    m_inputToSend.clear();
    m_conn = SshConnection::create();
    connect(m_conn.data(), SIGNAL(connected()), this, SLOT(handleConnected()));
    connect(m_conn.data(), SIGNAL(error(Core::SshError)), this,
        SLOT(handleConnectionError()));
    m_conn->connectToHost(m_connParams);
}

void RemoteGdbProcess::handleConnected()
{
    if (m_state == Inactive)
        return;

    QTC_ASSERT(m_state == Connecting, return);
    setState(CreatingFifo);

    m_fifoCreator = m_conn->createRemoteProcess( "rm -f "
        + m_appOutputFileName + " && mkfifo " + m_appOutputFileName);
    connect(m_fifoCreator.data(), SIGNAL(closed(int)), this,
        SLOT(handleFifoCreationFinished(int)));
    m_fifoCreator->start();
}

void RemoteGdbProcess::handleConnectionError()
{
    if (m_state != Inactive)
        emitErrorExit(tr("Connection failure: %1.").arg(m_conn->errorString()));
}

void RemoteGdbProcess::handleFifoCreationFinished(int exitStatus)
{
    if (m_state == Inactive)
        return;
    QTC_ASSERT(m_state == CreatingFifo, return);

    if (exitStatus != SshRemoteProcess::ExitedNormally) {
        emitErrorExit(tr("Could not create FIFO."));
    } else {
        setState(StartingFifoReader);
        m_appOutputReader = m_conn->createRemoteProcess("cat "
            + m_appOutputFileName + " && rm -f " + m_appOutputFileName);
        connect(m_appOutputReader.data(), SIGNAL(started()), this,
            SLOT(handleAppOutputReaderStarted()));
        connect(m_appOutputReader.data(), SIGNAL(closed(int)), this,
            SLOT(handleAppOutputReaderFinished(int)));
        m_appOutputReader->start();
    }
}

void RemoteGdbProcess::handleAppOutputReaderStarted()
{
    if (m_state == Inactive)
        return;
    QTC_ASSERT(m_state == StartingFifoReader, return);
    setState(StartingGdb);

    connect(m_appOutputReader.data(), SIGNAL(outputAvailable(QByteArray)),
        this, SLOT(handleAppOutput(QByteArray)));
    QByteArray cmdLine = "DISPLAY=:0.0 " + m_command.toUtf8() + ' '
        + Utils::QtcProcess::joinArgsUnix(m_cmdArgs).toUtf8()
        + " -tty=" + m_appOutputFileName;
    if (!m_wd.isEmpty())
        cmdLine.prepend("cd " + Utils::QtcProcess::quoteArgUnix(m_wd).toUtf8() + " && ");
    m_gdbProc = m_conn->createRemoteProcess(cmdLine);
    connect(m_gdbProc.data(), SIGNAL(started()), this,
        SLOT(handleGdbStarted()));
    connect(m_gdbProc.data(), SIGNAL(closed(int)), this,
        SLOT(handleGdbFinished(int)));
    connect(m_gdbProc.data(), SIGNAL(outputAvailable(QByteArray)), this,
        SLOT(handleGdbOutput(QByteArray)));
    connect(m_gdbProc.data(), SIGNAL(errorOutputAvailable(QByteArray)), this,
        SLOT(handleErrOutput(QByteArray)));
    m_gdbProc->start();
}

void RemoteGdbProcess::handleAppOutputReaderFinished(int exitStatus)
{
    if (exitStatus != SshRemoteProcess::ExitedNormally)
        emitErrorExit(tr("Application output reader unexpectedly finished."));
}

void RemoteGdbProcess::handleGdbStarted()
{
    if (m_state == Inactive)
        return;
    QTC_ASSERT(m_state == StartingGdb, return);
    setState(RunningGdb);
    emit started();
}

void RemoteGdbProcess::handleGdbFinished(int exitStatus)
{
    if (m_state == Inactive)
        return;
    QTC_ASSERT(m_state == RunningGdb, return);

    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        m_error = tr("Remote gdb failed to start.");
        setState(Inactive);
        emit startFailed();
        break;
    case SshRemoteProcess::KilledBySignal:
        emitErrorExit(tr("Remote gdb crashed."));
        break;
    case SshRemoteProcess::ExitedNormally:
        const int exitCode = m_gdbProc->exitCode();
        setState(Inactive);
        emit finished(exitCode, QProcess::NormalExit);
        break;
    }
}

bool RemoteGdbProcess::waitForStarted()
{
    if (m_state == Inactive)
        return false;
    QTC_ASSERT(m_state == RunningGdb, return false);
    return true;
}

qint64 RemoteGdbProcess::write(const QByteArray &data)
{
    if (m_state != RunningGdb || !m_inputToSend.isEmpty()
            || !m_lastSeqNr.isEmpty())
        m_inputToSend.enqueue(data);
    else
        sendInput(data);
    return data.size();
}

void RemoteGdbProcess::kill()
{
    if (m_state == RunningGdb) {
        SshRemoteProcess::Ptr killProc
            = m_conn->createRemoteProcess("pkill -SIGKILL -x gdb");
        killProc->start();
    } else {
        setState(Inactive);
    }
}

void RemoteGdbProcess::interruptInferior()
{
    QTC_ASSERT(m_state == RunningGdb, return);

    SshRemoteProcess::Ptr intProc
        = m_conn->createRemoteProcess("pkill -x -SIGINT gdb");
    intProc->start();
}

QProcess::ProcessState RemoteGdbProcess::state() const
{
    switch (m_state) {
    case RunningGdb: return QProcess::Running;
    case Inactive: return QProcess::NotRunning;
    default: return QProcess::Starting;
    }
}

QString RemoteGdbProcess::errorString() const
{
    return m_error;
}

void RemoteGdbProcess::handleGdbOutput(const QByteArray &output)
{
    if (m_state == Inactive)
        return;
    QTC_ASSERT(m_state == RunningGdb, return);

    // TODO: Carriage return removal still necessary?
    m_currentGdbOutput += removeCarriageReturn(output);
#if 0
    qDebug("%s: complete unread output is '%s'", Q_FUNC_INFO, m_currentGdbOutput.data());
#endif
    if (!m_currentGdbOutput.endsWith('\n'))
        return;

    if (m_currentGdbOutput.contains(m_lastSeqNr + '^'))
        m_lastSeqNr.clear();

    if (m_lastSeqNr.isEmpty() && !m_inputToSend.isEmpty()) {
#if 0
        qDebug("Sending queued command: %s", m_inputToSend.head().data());
#endif
        sendInput(m_inputToSend.dequeue());
    }

    if (!m_currentGdbOutput.isEmpty()) {
        const int startPos
            = m_gdbOutput.isEmpty() ? findAnchor(m_currentGdbOutput) : 0;
        if (startPos != -1) {
            m_gdbOutput += m_currentGdbOutput.mid(startPos);
            m_currentGdbOutput.clear();
            emit readyReadStandardOutput();
        }
    }
}

QProcessEnvironment RemoteGdbProcess::processEnvironment() const
{
    return QProcessEnvironment(); // TODO: Provide actual environment.
}

void RemoteGdbProcess::setProcessEnvironment(const QProcessEnvironment & /* env */)
{
    // TODO: Do something. (if remote process exists: set, otherwise queue)
}

void RemoteGdbProcess::setEnvironment(const QStringList & /* env */)
{
    // TODO: Do something.
}

void RemoteGdbProcess::setWorkingDirectory(const QString &dir)
{
    m_wd = dir;
}

int RemoteGdbProcess::findAnchor(const QByteArray &data) const
{
    for (int pos = 0; pos < data.count(); ++pos) {
        const char c = data.at(pos);
        if (isdigit(c) || c == '*' || c == '+' || c == '=' || c == '~'
            || c == '@' || c == '&' || c == '^')
            return pos;
    }
    return -1;
}

void RemoteGdbProcess::sendInput(const QByteArray &data)
{
    QTC_ASSERT(m_state == RunningGdb, return);

    int pos;
    for (pos = 0; pos < data.size(); ++pos)
        if (!isdigit(data.at(pos)))
            break;
    m_lastSeqNr = data.left(pos);
    m_gdbProc->sendInput(data);
}

void RemoteGdbProcess::handleAppOutput(const QByteArray &output)
{
    if (m_state == RunningGdb)
        m_adapter->handleApplicationOutput(output);
}

void RemoteGdbProcess::handleErrOutput(const QByteArray &output)
{
    if (m_state == RunningGdb) {
        m_errorOutput += output;
        emit readyReadStandardError();
    }
}

QByteArray RemoteGdbProcess::removeCarriageReturn(const QByteArray &data)
{
    QByteArray output;
    for (int i = 0; i < data.size(); ++i) {
        const char c = data.at(i);
        if (c != '\r')
            output += c;
    }
    return output;
}

void RemoteGdbProcess::emitErrorExit(const QString &error)
{
    if (m_error.isEmpty()) {
        m_error = error;
        setState(Inactive);
        emit finished(-1, QProcess::CrashExit);
    }
}

void RemoteGdbProcess::setState(State newState)
{
    if (m_state == newState)
        return;
    m_state = newState;
    if (m_state == Inactive) {
        if (m_gdbProc) {
            disconnect(m_gdbProc.data(), 0, this, 0);
            m_gdbProc = SshRemoteProcess::Ptr();
        }
        if (m_appOutputReader) {
            disconnect(m_appOutputReader.data(), 0, this, 0);
            m_appOutputReader = SshRemoteProcess::Ptr();
        }
        if (m_fifoCreator) {
            disconnect(m_fifoCreator.data(), 0, this, 0);
            m_fifoCreator = SshRemoteProcess::Ptr();
        }
        disconnect(m_conn.data(), 0, this, 0);
        m_conn->disconnectFromHost();
    }
}

const QByteArray RemoteGdbProcess::CtrlC = QByteArray(1, 0x3);

} // namespace Internal
} // namespace Debugger
