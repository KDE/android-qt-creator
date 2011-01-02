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
    m_deployQtLibs = true;
    m_forceDeploy = false;
}

bool AndroidDeployStep::init()
{
    return true;
}

void AndroidDeployStep::run(QFutureInterface<bool> &fi)
{
    fi.reportResult(deployPackage());
}

BuildStepConfigWidget *AndroidDeployStep::createConfigWidget()
{
    return new AndroidDeployStepWidget(this);
}

bool AndroidDeployStep::deployQtLibs()
{
    return m_deployQtLibs;
}

void AndroidDeployStep::setDeployQtLibs(bool deploy)
{
    m_deployQtLibs=deploy;
}

bool AndroidDeployStep::forceDeploy()
{
    return m_forceDeploy;
}

void AndroidDeployStep::setForceDeploy(bool force)
{
    m_forceDeploy=force;
}

QVariantMap AndroidDeployStep::toMap() const
{
    QVariantMap map(BuildStep::toMap());
    map.insert(AndroidDeployQtLibsKey, m_deployQtLibs);
    map.insert(AndroidForceDeployKey, m_forceDeploy);
    return map;
}

bool AndroidDeployStep::fromMap(const QVariantMap &map)
{
    if (!BuildStep::fromMap(map))
        return false;
    m_deployQtLibs = map.value(AndroidDeployQtLibsKey, m_deployQtLibs).toBool();
    m_forceDeploy = map.value(AndroidForceDeployKey, m_forceDeploy).toBool();
    return true;
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

QString AndroidDeployStep::deviceSerialNumber()
{
    return m_deviceSerialNumber;
}

void AndroidDeployStep::copyLibs(const QString &srcPath, const QString &destPath, QStringList & copiedLibs, const QStringList &filter)
{
    QDir dir;
    dir.mkpath(destPath);
    QDirIterator libsIt(srcPath, filter, QDir::NoFilter, QDirIterator::Subdirectories);
    int pos=srcPath.size();
    while(libsIt.hasNext())
    {
        libsIt.next();
        const QString destFile(destPath+libsIt.filePath().mid(pos));
        if (libsIt.fileInfo().isDir())
            dir.mkpath(destFile);
        else
        {
            QFile::copy(libsIt.filePath(), destFile);
            copiedLibs.append(destFile);
        }
    }
}

bool AndroidDeployStep::deployPackage()
{
    const Qt4BuildConfiguration * const bc
        = static_cast<Qt4BuildConfiguration *>(buildConfiguration());
    const QString packageName=AndroidTemplatesManager::instance()->packageName(bc->qt4Target()->qt4Project());
    writeOutput(tr("Please wait, searching for a siutable device."));

    m_deviceSerialNumber=AndroidConfigurations::instance().getDeployDeviceSerialNumber(8);
    if (!m_deviceSerialNumber.length())
    {
        m_deviceSerialNumber.clear();
        raiseError(tr("Cannot deploy: no devices, emulators found for your package."));
        return false;
    }

    QProcess proc;
    connect(&proc, SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleBuildOutput()));
    connect(&proc, SIGNAL(readyReadStandardError()), this,
        SLOT(handleBuildOutput()));

    bool deployQtLibs=m_forceDeploy;
    if (!deployQtLibs) // check is deploy is needed
    {
        writeOutput(tr("Checking if qt libs deploy is needed"));
        deployQtLibs = !runCommand(&proc, AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)
                                   +" shell ls /data/local/qt/lib");
    }
    else
    {
        writeOutput(tr("Clean old qt libs"));
        runCommand(&proc, AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)
                   +" shell rm -r /data/local/qt");
    }

    if (deployQtLibs)
    {
        writeOutput(tr("Deploy qt libs ... it may take some time, please wait"));
        const QString tempPath=QDir::tempPath()+"/android_qt_libs_"+packageName;
        AndroidPackageCreationStep::removeDirectory(tempPath);
        QStringList stripFiles;
        copyLibs(bc->qtVersion()->sourcePath()+"/lib", tempPath+"/lib",stripFiles,QStringList()<<"*.so");
        copyLibs(bc->qtVersion()->sourcePath()+"/plugins", tempPath+"/plugins",stripFiles);
        AndroidPackageCreationStep::stripAndroidLibs(stripFiles);
        runCommand(&proc,AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)
                   +QString(" push %1 /data/local/qt").arg(tempPath));
        AndroidPackageCreationStep::removeDirectory(tempPath);
    }

    proc.setWorkingDirectory(AndroidTemplatesManager::instance()->androidDirPath(bc->qt4Target()->qt4Project()));

    writeOutput(tr("Installing package onto %1.").arg(m_deviceSerialNumber));
    runCommand(&proc,AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)
               +QLatin1String(" uninstall ")
               +packageName);

    if (!runCommand(&proc, AndroidConfigurations::instance().antToolPath()+QLatin1String(" install")))
        raiseError(tr("Package instalation failed"));
    return true;
}

void AndroidDeployStep::raiseError(const QString &errorString)
{ 
    emit addTask(Task(Task::Error, errorString, QString(), -1,
        ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
}

void AndroidDeployStep::writeOutput(const QString &text, OutputFormat format)
{
    emit addOutput(text, format);
}

} // namespace Internal
} // namespace Qt4ProjectManager
