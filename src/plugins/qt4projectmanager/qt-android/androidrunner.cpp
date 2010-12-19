/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "androidrunner.h"

#include "androiddeploystep.h"
#include "androidconfigurations.h"
#include "androidglobal.h"
#include "androidrunconfiguration.h"

#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>

#include <QtCore/QFileInfo>

#include <limits>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(State, state, m_state)

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

AndroidRunner::AndroidRunner(QObject *parent,
    AndroidRunConfiguration *runConfig, bool debugging)
    : QObject(parent),
      m_devConfig(runConfig->deviceConfig()),
      m_appArguments(runConfig->arguments()),
      m_userEnvChanges(runConfig->userEnvironmentChanges()),
      m_state(Inactive)
{
#warning FIXME Android
//    m_procsToKill << QFileInfo(m_remoteExecutable).fileName();
}

AndroidRunner::~AndroidRunner() {}

void AndroidRunner::start()
{
#warning FIXME Android

//    ASSERT_STATE(QList<State>() << Inactive << StopRequested);

//    if (m_remoteExecutable.isEmpty()) {
//        emitError(tr("Cannot run: No remote executable set."));
//        return;
//    }
//    if (!m_devConfig.isValid()) {
//        emitError(tr("Cannot run: No device configuration set."));
//        return;
//    }

//    setState(Connecting);
//    m_exitStatus = -1;
//    m_freePorts = m_initialFreePorts;
//    if (m_connection)
//        disconnect(m_connection.data(), 0, this, 0);
//    const bool reUse = isConnectionUsable();
//    if (!reUse)
//        m_connection = SshConnection::create();
//    connect(m_connection.data(), SIGNAL(connected()), this,
//        SLOT(handleConnected()));
//    connect(m_connection.data(), SIGNAL(error(Core::SshError)), this,
//        SLOT(handleConnectionFailure()));
//    if (reUse) {
//        handleConnected();
//    } else {
//        emit reportProgress(tr("Connecting to device..."));
//        m_connection->connectToHost(m_devConfig.server);
//    }
}

void AndroidRunner::stop()
{
    if (m_state == PostRunCleaning || m_state == StopRequested
        || m_state == Inactive)
        return;

    setState(StopRequested);
    cleanup();
}

void AndroidRunner::handleConnected()
{
    ASSERT_STATE(QList<State>() << Connecting << StopRequested);
    if (m_state == StopRequested) {
        setState(Inactive);
    } else {
        setState(PreRunCleaning);
        cleanup();
    }
}

void AndroidRunner::handleConnectionFailure()
{
    if (m_state == Inactive)
        qWarning("Unexpected state %d in %s.", m_state, Q_FUNC_INFO);

    const QString errorTemplate = m_state == Connecting
        ? tr("Could not connect to host: %1") : tr("Connection failed: %1");
    emitError(errorTemplate.arg(m_connection->errorString()));
}

void AndroidRunner::cleanup()
{
    ASSERT_STATE(QList<State>() << PreRunCleaning << PostRunCleaning
        << StopRequested);

    emit reportProgress(tr("Killing remote process(es)..."));

    // pkill behaves differently on Fremantle and Harmattan.
    const char *const killTemplate = "pkill -%2 '^%1$'; pkill -%2 '/%1$';";
    QString niceKill;
    QString brutalKill;
    foreach (const QString &proc, m_procsToKill) {
        niceKill += QString::fromLocal8Bit(killTemplate).arg(proc).arg("SIGTERM");
        brutalKill += QString::fromLocal8Bit(killTemplate).arg(proc).arg("SIGKILL");
    }
    QString remoteCall = niceKill + QLatin1String("sleep 1; ") + brutalKill;
    remoteCall.remove(remoteCall.count() - 1, 1); // Get rid of trailing semicolon.

    m_cleaner = m_connection->createRemoteProcess(remoteCall.toUtf8());
    connect(m_cleaner.data(), SIGNAL(closed(int)), this,
        SLOT(handleCleanupFinished(int)));
    m_cleaner->start();
}

void AndroidRunner::handleCleanupFinished(int exitStatus)
{
#warning FIXME Android
//    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
//        || exitStatus == SshRemoteProcess::KilledBySignal
//        || exitStatus == SshRemoteProcess::ExitedNormally);

//    ASSERT_STATE(QList<State>() << PreRunCleaning << PostRunCleaning
//        << StopRequested << Inactive);

//    if (m_state == Inactive)
//        return;
//    if (m_state == StopRequested || m_state == PostRunCleaning) {
//        unmount();
//        return;
//    }

//    if (exitStatus != SshRemoteProcess::ExitedNormally) {
//        emitError(tr("Initial cleanup failed: %1")
//            .arg(m_cleaner->errorString()));
//    } else {
//        m_mounter->setConnection(m_connection);
//        unmount();
//    }
}

void AndroidRunner::handleUnmounted()
{
#warning FIXME Android
//    ASSERT_STATE(QList<State>() << PreRunCleaning << PreMountUnmounting
//        << PostRunCleaning << StopRequested);

//    switch (m_state) {
//    case PreRunCleaning: {
//        for (int i = 0; i < m_mountSpecs.count(); ++i)
//            m_mounter->addMountSpecification(m_mountSpecs.at(i), false);
//        setState(PreMountUnmounting);
//        unmount();
//        break;
//    }
//    case PreMountUnmounting:
//        setState(GatheringPorts);
//        m_portsGatherer->start(m_connection, m_freePorts);
//        break;
//    case PostRunCleaning:
//    case StopRequested: {
//        m_mounter->resetMountSpecifications();
//        const bool stopRequested = m_state == StopRequested;
//        setState(Inactive);
//        if (stopRequested) {
//            emit remoteProcessFinished(InvalidExitCode);
//        } else if (m_exitStatus == SshRemoteProcess::ExitedNormally) {
//            emit remoteProcessFinished(m_runner->exitCode());
//        } else {
//            emit error(tr("Error running remote process: %1")
//                .arg(m_runner->errorString()));
//        }
//        break;
//    }
//    default: ;
//    }
}

void AndroidRunner::handleMounted()
{
    ASSERT_STATE(QList<State>() << Mounting << StopRequested);

    if (m_state == Mounting) {
        setState(ReadyForExecution);
        emit readyForExecution();
    }
}

void AndroidRunner::handleMounterError(const QString &errorMsg)
{
    ASSERT_STATE(QList<State>() << PreRunCleaning << PostRunCleaning
        << PreMountUnmounting << Mounting << StopRequested << Inactive);

    emitError(errorMsg);
}

void AndroidRunner::startExecution(const QByteArray &remoteCall)
{
    ASSERT_STATE(ReadyForExecution);

    m_runner = m_connection->createRemoteProcess(remoteCall);
    connect(m_runner.data(), SIGNAL(started()), this,
        SIGNAL(remoteProcessStarted()));
    connect(m_runner.data(), SIGNAL(closed(int)), this,
        SLOT(handleRemoteProcessFinished(int)));
    connect(m_runner.data(), SIGNAL(outputAvailable(QByteArray)), this,
        SIGNAL(remoteOutput(QByteArray)));
    connect(m_runner.data(), SIGNAL(errorOutputAvailable(QByteArray)), this,
        SIGNAL(remoteErrorOutput(QByteArray)));
    setState(ProcessStarting);
    m_runner->start();
}

void AndroidRunner::handleRemoteProcessFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);
    ASSERT_STATE(QList<State>() << ProcessStarting << StopRequested << Inactive);

    m_exitStatus = exitStatus;
    if (m_state != StopRequested && m_state != Inactive) {
        setState(PostRunCleaning);
        cleanup();
    }
}

bool AndroidRunner::isConnectionUsable() const
{
#warning FIXME Android
    return false;/*m_connection && m_connection->state() == SshConnection::Connected
        && m_connection->connectionParameters() == m_devConfig.server;*/
}

void AndroidRunner::setState(State newState)
{
    m_state = newState;
}

void AndroidRunner::emitError(const QString &errorMsg)
{
    if (m_state != Inactive) {
        setState(Inactive);
        emit error(errorMsg);
    }
}


void AndroidRunner::handlePortsGathererError(const QString &errorMsg)
{
    emitError(errorMsg);
}

const qint64 AndroidRunner::InvalidExitCode
    = std::numeric_limits<qint64>::min();

} // namespace Internal
} // namespace Qt4ProjectManager

