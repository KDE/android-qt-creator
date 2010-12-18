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

#include "androidruncontrol.h"

#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidrunconfiguration.h"
#include "androidrunner.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <QtGui/QMessageBox>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

using ProjectExplorer::RunConfiguration;

AndroidRunControl::AndroidRunControl(RunConfiguration *rc)
    : RunControl(rc, ProjectExplorer::Constants::RUNMODE)
    , m_runner(new AndroidRunner(this, qobject_cast<AndroidRunConfiguration *>(rc), false))
    , m_running(false)
{
}

AndroidRunControl::~AndroidRunControl()
{
    stop();
}

void AndroidRunControl::start()
{
    m_running = true;
    emit started();
    disconnect(m_runner, 0, this, 0);
    connect(m_runner, SIGNAL(error(QString)), SLOT(handleSshError(QString)));
    connect(m_runner, SIGNAL(readyForExecution()), SLOT(startExecution()));
    connect(m_runner, SIGNAL(remoteErrorOutput(QByteArray)),
        SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(m_runner, SIGNAL(remoteOutput(QByteArray)),
        SLOT(handleRemoteOutput(QByteArray)));
    connect(m_runner, SIGNAL(remoteProcessStarted()),
        SLOT(handleRemoteProcessStarted()));
    connect(m_runner, SIGNAL(remoteProcessFinished(qint64)),
        SLOT(handleRemoteProcessFinished(qint64)));
    connect(m_runner, SIGNAL(reportProgress(QString)),
        SLOT(handleProgressReport(QString)));
    connect(m_runner, SIGNAL(mountDebugOutput(QString)),
        SLOT(handleMountDebugOutput(QString)));
    m_runner->start();
}

ProjectExplorer::RunControl::StopResult AndroidRunControl::stop()
{
    m_runner->stop();
    return StoppedSynchronously;
}

void AndroidRunControl::handleSshError(const QString &error)
{
    handleError(error);
    setFinished();
}

void AndroidRunControl::startExecution()
{
    emit appendMessage(this, tr("Starting remote process ..."), false);
//    m_runner->startExecution(QString::fromLocal8Bit("%1 %2 %3")
//        .arg(AndroidGlobal::remoteEnvironment(m_runner->userEnvChanges()))
//        .arg(m_runner->remoteExecutable())
//        .arg(m_runner->arguments()).toUtf8());
}

void AndroidRunControl::handleRemoteProcessFinished(qint64 exitCode)
{
    if (exitCode != AndroidRunner::InvalidExitCode) {
        emit appendMessage(this,
            tr("Finished running remote process. Exit code was %1.")
            .arg(exitCode), false);
    }
    setFinished();
}

void AndroidRunControl::handleRemoteOutput(const QByteArray &output)
{
    emit addToOutputWindowInline(this, QString::fromUtf8(output), false);
}

void AndroidRunControl::handleRemoteErrorOutput(const QByteArray &output)
{
    emit addToOutputWindowInline(this, QString::fromUtf8(output), true);
}

void AndroidRunControl::handleProgressReport(const QString &progressString)
{
    emit appendMessage(this, progressString, false);
}

void AndroidRunControl::handleMountDebugOutput(const QString &output)
{
    emit addToOutputWindowInline(this, output, true);
}

bool AndroidRunControl::isRunning() const
{
    return m_running;
}

void AndroidRunControl::handleError(const QString &errString)
{
    stop();
    emit appendMessage(this, errString, true);
    QMessageBox::critical(0, tr("Remote Execution Failure"), errString);
}

void AndroidRunControl::setFinished()
{
    disconnect(m_runner, 0, this, 0);
    m_running = false;
    emit finished();
}

} // namespace Internal
} // namespace Qt4ProjectManager
