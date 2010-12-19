/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "androiddeploystep.h"

#include "androidconstants.h"
#include "androiddeploystepwidget.h"
#include "androiddeviceconfiglistmodel.h"
#include "androidglobal.h"
#include "androidpackagecreationstep.h"
#include "androidrunconfiguration.h"
#include "androidtoolchain.h"
#include "androidtemplatesmanager.h"

#include <coreplugin/ssh/sftpchannel.h>
#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qt4project.h>
#include <qt4target.h>

#include <qt4projectmanager/qt4buildconfiguration.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtCore/QProcess>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(State, state, m_state)

using namespace Core;
using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {
namespace { const int DefaultMountPort = 1050; }

const QLatin1String AndroidDeployStep::Id("Qt4ProjectManager.AndroidDeployStep");

AndroidDeployStep::AndroidDeployStep(ProjectExplorer::BuildStepList *parent)
    : BuildStep(parent, Id)
{
    ctor();
}

AndroidDeployStep::AndroidDeployStep(ProjectExplorer::BuildStepList *parent,
    AndroidDeployStep *other)
    : BuildStep(parent, other)
{
    ctor();
}

AndroidDeployStep::~AndroidDeployStep() { }

void AndroidDeployStep::ctor()
{
    //: AndroidDeployStep default display name
    setDefaultDisplayName(tr("Deploy to Android device"));
}

bool AndroidDeployStep::init()
{
    return true;
}

void AndroidDeployStep::run(QFutureInterface<bool> &fi)
{
    // Move to GUI thread for connection sharing with run control.
    QTimer::singleShot(0, this, SLOT(start()));

    AndroidDeployEventHandler eventHandler(this, fi);
}

BuildStepConfigWidget *AndroidDeployStep::createConfigWidget()
{
    return new AndroidDeployStepWidget(this);
}

bool AndroidDeployStep::runCommand(QProcess *buildProc,
    const QString &command)
{

    writeOutput(tr("Package deploy: Running command '%1'.").arg(command), BuildStep::MessageOutput);
    buildProc->start(command);
    if (!buildProc->waitForStarted()) {
        writeOutput(tr("Packaging error: Could not start command '%1'. Reason: %2")
            .arg(command).arg(buildProc->errorString()), BuildStep::ErrorMessageOutput);
        return false;
    }
    buildProc->waitForFinished(-1);
    if (buildProc->error() != QProcess::UnknownError
        || buildProc->exitCode() != 0) {
        QString mainMessage = tr("Packaging Error: Command '%1' failed.")
            .arg(command);
        if (buildProc->error() != QProcess::UnknownError)
            mainMessage += tr(" Reason: %1").arg(buildProc->errorString());
        else
            mainMessage += tr("Exit code: %1").arg(buildProc->exitCode());
        writeOutput(mainMessage, BuildStep::ErrorMessageOutput);
        return false;
    }
    return true;
}

void AndroidDeployStep::handleBuildOutput()
{
    QProcess * const buildProc = qobject_cast<QProcess *>(sender());
    if (!buildProc)
        return;
    const QByteArray &stdOut = buildProc->readAllStandardOutput();
    if (!stdOut.isEmpty())
        emit addOutput(QString::fromLocal8Bit(stdOut), BuildStep::NormalOutput);
    const QByteArray &errorOut = buildProc->readAllStandardError();
    if (!errorOut.isEmpty()) {
        emit addOutput(QString::fromLocal8Bit(errorOut), BuildStep::ErrorOutput);
    }
}

void AndroidDeployStep::start()
{
    const Qt4BuildConfiguration * const bc
        = static_cast<Qt4BuildConfiguration *>(buildConfiguration());

    writeOutput(tr("Please wait, searching for a siutable device."));

    QString serialNumber=AndroidConfigurations::instance().getDeployDeviceSerialNumber(8);
    if (!serialNumber.length())
    {
        raiseError(tr("Cannot deploy: no devices, emulators found for your package."));
        emit done();
        return;
    }

    writeOutput(tr("Installing package onto %1.").arg(serialNumber));
    QProcess proc;
    connect(&proc, SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleBuildOutput()));
    connect(&proc, SIGNAL(readyReadStandardError()), this,
        SLOT(handleBuildOutput()));

    proc.setWorkingDirectory(AndroidTemplatesManager::instance()->androidDirPath(bc->qt4Target()->qt4Project()));

    runCommand(&proc,AndroidConfigurations::instance().adbToolPath()
               +QLatin1String(" uninstall ")
               +AndroidTemplatesManager::instance()->packageName(bc->qt4Target()->qt4Project()));

    if (!runCommand(&proc, AndroidConfigurations::instance().antToolPath()+QLatin1String(" install")))
        raiseError(tr("Package instalation failed"));
    emit done();
    return;
}


void AndroidDeployStep::raiseError(const QString &errorString)
{
    emit addTask(Task(Task::Error, errorString, QString(), -1,
        ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    emit error();
}

void AndroidDeployStep::writeOutput(const QString &text, OutputFormat format)
{
    emit addOutput(text, format);
}

void AndroidDeployStep::stop()
{
}


AndroidDeployEventHandler::AndroidDeployEventHandler(AndroidDeployStep *deployStep,
    QFutureInterface<bool> &future)
    : m_deployStep(deployStep), m_future(future), m_eventLoop(new QEventLoop),
      m_error(false)
{
    connect(m_deployStep, SIGNAL(done()), this, SLOT(handleDeployingDone()));
    connect(m_deployStep, SIGNAL(error()), this, SLOT(handleDeployingFailed()));
    QTimer cancelChecker;
    connect(&cancelChecker, SIGNAL(timeout()), this, SLOT(checkForCanceled()));
    cancelChecker.start(500);
    future.reportResult(m_eventLoop->exec() == 0);
}

void AndroidDeployEventHandler::handleDeployingDone()
{
    m_eventLoop->exit(m_error ? 1 : 0);
}

void AndroidDeployEventHandler::handleDeployingFailed()
{
    m_error = true;
}

void AndroidDeployEventHandler::checkForCanceled()
{
    if (!m_error && m_future.isCanceled()) {
        QMetaObject::invokeMethod(m_deployStep, "stop");
        m_error = true;
        handleDeployingDone();
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
