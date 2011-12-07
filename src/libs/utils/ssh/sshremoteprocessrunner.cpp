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

#include "sshremoteprocessrunner.h"

#include "sshconnectionmanager.h"
#include "sshpseudoterminal.h"

#include <utils/qtcassert.h>


/*!
    \class Utils::SshRemoteProcessRunner

    \brief Convenience class for running a remote process over an SSH connection.
*/

namespace Utils {
namespace Internal {
namespace {
enum State { Inactive, Connecting, Connected, ProcessRunning };
} // anonymous namespace

class SshRemoteProcessRunnerPrivate
{
public:
    SshRemoteProcessRunnerPrivate() : m_state(Inactive) {}

    SshRemoteProcess::Ptr m_process;
    SshConnection::Ptr m_connection;
    bool m_runInTerminal;
    SshPseudoTerminal m_terminal;
    QByteArray m_command;
    Utils::SshError m_lastConnectionError;
    QString m_lastConnectionErrorString;
    SshRemoteProcess::ExitStatus m_exitStatus;
    QByteArray m_exitSignal;
    int m_exitCode;
    QString m_processErrorString;
    State m_state;
};

} // namespace Internal

using namespace Internal;

SshRemoteProcessRunner::SshRemoteProcessRunner(QObject *parent)
    : QObject(parent), d(new SshRemoteProcessRunnerPrivate)
{
}

SshRemoteProcessRunner::~SshRemoteProcessRunner()
{
    disconnect();
    setState(Inactive);
    delete d;
}

void SshRemoteProcessRunner::run(const QByteArray &command,
    const SshConnectionParameters &sshParams)
{
    QTC_ASSERT(d->m_state == Inactive, return);

    d->m_runInTerminal = false;
    runInternal(command, sshParams);
}

void SshRemoteProcessRunner::runInTerminal(const QByteArray &command,
    const SshPseudoTerminal &terminal, const SshConnectionParameters &sshParams)
{
    d->m_terminal = terminal;
    d->m_runInTerminal = true;
    runInternal(command, sshParams);
}

void SshRemoteProcessRunner::runInternal(const QByteArray &command,
    const SshConnectionParameters &sshParams)
{
    setState(Connecting);

    d->m_lastConnectionError = SshNoError;
    d->m_lastConnectionErrorString.clear();
    d->m_processErrorString.clear();
    d->m_exitSignal.clear();
    d->m_exitCode = -1;
    d->m_command = command;
    d->m_connection = SshConnectionManager::instance().acquireConnection(sshParams);
    connect(d->m_connection.data(), SIGNAL(error(Utils::SshError)),
        SLOT(handleConnectionError(Utils::SshError)));
    connect(d->m_connection.data(), SIGNAL(disconnected()), SLOT(handleDisconnected()));
    if (d->m_connection->state() == SshConnection::Connected) {
        handleConnected();
    } else {
        connect(d->m_connection.data(), SIGNAL(connected()), SLOT(handleConnected()));
        if (d->m_connection->state() == SshConnection::Unconnected)
            d->m_connection->connectToHost();
    }
}

void SshRemoteProcessRunner::handleConnected()
{
    QTC_ASSERT(d->m_state == Connecting, return);
    setState(Connected);

    d->m_process = d->m_connection->createRemoteProcess(d->m_command);
    connect(d->m_process.data(), SIGNAL(started()), SLOT(handleProcessStarted()));
    connect(d->m_process.data(), SIGNAL(closed(int)), SLOT(handleProcessFinished(int)));
    connect(d->m_process.data(), SIGNAL(readyReadStandardOutput()), SLOT(handleStdout()));
    connect(d->m_process.data(), SIGNAL(readyReadStandardError()), SLOT(handleStderr()));
    if (d->m_runInTerminal)
        d->m_process->requestTerminal(d->m_terminal);
    d->m_process->start();
}

void SshRemoteProcessRunner::handleConnectionError(Utils::SshError error)
{
    d->m_lastConnectionError = error;
    d->m_lastConnectionErrorString = d->m_connection->errorString();
    handleDisconnected();
    emit connectionError();
}

void SshRemoteProcessRunner::handleDisconnected()
{
    QTC_ASSERT(d->m_state == Connecting || d->m_state == Connected
        || d->m_state == ProcessRunning, return);
    setState(Inactive);
}

void SshRemoteProcessRunner::handleProcessStarted()
{
    QTC_ASSERT(d->m_state == Connected, return);

    setState(ProcessRunning);
    emit processStarted();
}

void SshRemoteProcessRunner::handleProcessFinished(int exitStatus)
{
    d->m_exitStatus = static_cast<SshRemoteProcess::ExitStatus>(exitStatus);
    switch (d->m_exitStatus) {
    case SshRemoteProcess::FailedToStart:
        QTC_ASSERT(d->m_state == Connected, return);
        break;
    case SshRemoteProcess::KilledBySignal:
        QTC_ASSERT(d->m_state == ProcessRunning, return);
        d->m_exitSignal = d->m_process->exitSignal();
        break;
    case SshRemoteProcess::ExitedNormally:
        QTC_ASSERT(d->m_state == ProcessRunning, return);
        d->m_exitCode = d->m_process->exitCode();
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Impossible exit status.");
    }
    d->m_processErrorString = d->m_process->errorString();
    setState(Inactive);
    emit processClosed(exitStatus);
}

void SshRemoteProcessRunner::handleStdout()
{
    emit processOutputAvailable(d->m_process->readAllStandardOutput());
}

void SshRemoteProcessRunner::handleStderr()
{
    emit processErrorOutputAvailable(d->m_process->readAllStandardError());
}

void SshRemoteProcessRunner::setState(int newState)
{
    if (d->m_state == newState)
        return;

    d->m_state = static_cast<State>(newState);
    if (d->m_state == Inactive) {
        if (d->m_process) {
            disconnect(d->m_process.data(), 0, this, 0);
            d->m_process->close();
            d->m_process.clear();
        }
        if (d->m_connection) {
            disconnect(d->m_connection.data(), 0, this, 0);
            SshConnectionManager::instance().releaseConnection(d->m_connection);
            d->m_connection.clear();
        }
    }
}

QByteArray SshRemoteProcessRunner::command() const { return d->m_command; }
SshError SshRemoteProcessRunner::lastConnectionError() const { return d->m_lastConnectionError; }
QString SshRemoteProcessRunner::lastConnectionErrorString() const {
    return d->m_lastConnectionErrorString;
}

bool SshRemoteProcessRunner::isProcessRunning() const
{
    return d->m_process && d->m_process->isRunning();
}

SshRemoteProcess::ExitStatus SshRemoteProcessRunner::processExitStatus() const
{
    QTC_CHECK(!isProcessRunning());
    return d->m_exitStatus;
}

QByteArray SshRemoteProcessRunner::processExitSignal() const
{
    QTC_CHECK(processExitStatus() == SshRemoteProcess::KilledBySignal);
    return d->m_exitSignal;
}

int SshRemoteProcessRunner::processExitCode() const
{
    QTC_CHECK(processExitStatus() == SshRemoteProcess::ExitedNormally);
    return d->m_exitCode;
}

QString SshRemoteProcessRunner::processErrorString() const
{
    return d->m_processErrorString;
}

void SshRemoteProcessRunner::writeDataToProcess(const QByteArray &data)
{
    QTC_ASSERT(isProcessRunning(), return);
    d->m_process->write(data);
}

void SshRemoteProcessRunner::sendSignalToProcess(const QByteArray &signal)
{
    QTC_ASSERT(isProcessRunning(), return);
    d->m_process->sendSignal(signal);
}

void SshRemoteProcessRunner::cancel()
{
    setState(Inactive);
}

} // namespace Utils
