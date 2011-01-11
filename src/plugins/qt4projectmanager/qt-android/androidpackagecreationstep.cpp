/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "androidpackagecreationstep.h"

#include "androidconstants.h"
#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidpackagecreationwidget.h"
#include "androidtemplatesmanager.h"
#include "androidtoolchain.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qt4buildconfiguration.h>
#include <qt4project.h>
#include <qt4target.h>
#include <utils/environment.h>

#include <QtCore/QDateTime>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QStringBuilder>
#include <QtGui/QWidget>

using namespace ProjectExplorer::Constants;
using ProjectExplorer::BuildStepList;
using ProjectExplorer::BuildStepConfigWidget;
using ProjectExplorer::Task;

namespace Qt4ProjectManager {
namespace Internal {

const QLatin1String AndroidPackageCreationStep::DefaultVersionNumber("0.0.1");

AndroidPackageCreationStep::AndroidPackageCreationStep(BuildStepList *bsl)
    : ProjectExplorer::BuildStep(bsl, CreatePackageId)
{
    ctor();
}

AndroidPackageCreationStep::AndroidPackageCreationStep(BuildStepList *bsl,
    AndroidPackageCreationStep *other)
    : BuildStep(bsl, other)
{
    ctor();
}

AndroidPackageCreationStep::~AndroidPackageCreationStep()
{
}

void AndroidPackageCreationStep::ctor()
{
    setDefaultDisplayName(tr("Packaging for Android"));
}

bool AndroidPackageCreationStep::init()
{
    return true;
}

void AndroidPackageCreationStep::run(QFutureInterface<bool> &fi)
{
    QProcess * const buildProc = new QProcess;
    connect(buildProc, SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleBuildOutput()));
    connect(buildProc, SIGNAL(readyReadStandardError()), this,
        SLOT(handleBuildOutput()));
    bool success=createPackage(buildProc);
    disconnect(buildProc, 0, this, 0);
    buildProc->deleteLater();

    fi.reportResult(success);
}

BuildStepConfigWidget *AndroidPackageCreationStep::createConfigWidget()
{
    return new AndroidPackageCreationWidget(this);
}

bool AndroidPackageCreationStep::createPackage(QProcess *buildProc)
{
    const Qt4BuildConfiguration * bc=static_cast<Qt4BuildConfiguration *>(buildConfiguration());

    emit addOutput(tr("Copy Qt app & libs to android ..."), MessageOutput);

    const QString androidDir(AndroidTemplatesManager::instance()->androidDirPath(bc->qt4Target()->qt4Project()));
    const QString androidLibPath(androidDir+QLatin1String("/libs/armeabi"));
    removeDirectory(androidLibPath);
    QDir d(androidDir);
    d.mkpath(androidLibPath);

    QStringList stripFiles;
    QList<Qt4ProFileNode *> nodes = bc->qt4Target()->qt4Project()->leafProFiles();
    foreach(Qt4ProFileNode * node, nodes)
    {
        QString fileName;
        QString androidFileName;
        switch(node->projectType())
        {
            case ApplicationTemplate:
                fileName=node->targetInformation().target;
                if (node->targetInformation().target.endsWith(QLatin1String(".so")))
                    androidFileName=node->targetInformation().target;
                else
                {
                    androidFileName=QLatin1String("lib")+node->targetInformation().target+QLatin1String(".so");
                    QFile::remove(node->targetInformation().buildDir+QLatin1Char('/')+androidFileName);
                    if (!QFile::copy(node->targetInformation().buildDir+QLatin1Char('/')+fileName,
                                node->targetInformation().buildDir+QLatin1Char('/')+androidFileName))
                    {
                        raiseError(tr("Cant copy '%1' from '%2' to '%3'").arg(fileName)
                                   .arg(node->targetInformation().buildDir)
                                   .arg(node->targetInformation().buildDir));
                        return false;
                    }
                }
                break;
            case LibraryTemplate:
                fileName=QLatin1String("lib")+node->targetInformation().target+QLatin1String(".so");
                androidFileName=fileName;
                break;
            default:
                continue;
        }

        if (!QFile::copy(node->targetInformation().buildDir+QLatin1Char('/')+fileName,
                    androidLibPath+QLatin1Char('/')+androidFileName))
        {
            raiseError(tr("Cant copy '%1' from '%2' to '%3'").arg(fileName)
                       .arg(node->targetInformation().buildDir)
                       .arg(androidLibPath));
            return false;
        }
        stripFiles<<androidLibPath+QLatin1Char('/')+androidFileName;
    }

    emit addOutput(tr("Stripping libraries, please wait"), BuildStep::MessageOutput);
    stripAndroidLibs(stripFiles);

    QString build;
    if (bc->qmakeBuildConfiguration() & QtVersion::DebugBuild)
    {
            if (!QFile::copy(AndroidConfigurations::instance().gdbServerPath(),
                             androidLibPath+QLatin1String("/gdbserver")))
            {
                raiseError(tr("Cant copy gdbserver from '%1' to '%2'").arg(AndroidConfigurations::instance().gdbServerPath())
                           .arg(androidLibPath+QLatin1String("/gdbserver")));
                return false;
            }
            build=QLatin1String(" debug");
    }
    else
        build=QLatin1String(" release");


    emit addOutput(tr("Creating package file ..."), MessageOutput);
    if (!AndroidTemplatesManager::instance()->createAndroidTemplatesIfNecessary(bc->qt4Target()->qt4Project()))
        return false;

    AndroidTemplatesManager::instance()->updateProject(bc->qt4Target()->qt4Project(),
                                                       AndroidTemplatesManager::instance()->targetSDK(bc->qt4Target()->qt4Project()));

    buildProc->setWorkingDirectory(androidDir);

    if (!runCommand(buildProc, AndroidConfigurations::instance().antToolPath()+build))
        return false;

    emit addOutput(tr("Package created."), BuildStep::MessageOutput);

    return true;
}

void AndroidPackageCreationStep::stripAndroidLibs(const QStringList & files)
{
    QProcess stripProcess;
    foreach(QString file, files)
    {
        qDebug()<<AndroidConfigurations::instance().stripPath()+" --strip-unneeded "+file;
        stripProcess.start(AndroidConfigurations::instance().stripPath()+" --strip-unneeded "+file);
        stripProcess.waitForFinished();
    }
}

bool AndroidPackageCreationStep::removeDirectory(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists())
        return true;

    const QStringList &files
        = dir.entryList(QDir::Files | QDir::Hidden | QDir::System);
    foreach (const QString &fileName, files) {
        if (!dir.remove(fileName))
            return false;
    }

    const QStringList &subDirs
        = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QString &subDirName, subDirs) {
        if (!removeDirectory(dirPath + QLatin1Char('/') + subDirName))
            return false;
    }

    return dir.rmdir(dirPath);
}

bool AndroidPackageCreationStep::runCommand(QProcess *buildProc,
    const QString &command)
{
    emit addOutput(tr("Package Creation: Running command '%1'.").arg(command), BuildStep::MessageOutput);
    buildProc->start(command);
    if (!buildProc->waitForStarted()) {
        raiseError(tr("Packaging failed."),
            tr("Packaging error: Could not start command '%1'. Reason: %2")
            .arg(command).arg(buildProc->errorString()));
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
        raiseError(mainMessage);
        return false;
    }
    return true;
}

void AndroidPackageCreationStep::handleBuildOutput()
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

void AndroidPackageCreationStep::raiseError(const QString &shortMsg,
                                          const QString &detailedMsg)
{
    emit addOutput(detailedMsg.isNull() ? shortMsg : detailedMsg, BuildStep::ErrorOutput);
    emit addTask(Task(Task::Error, shortMsg, QString(), -1,
                      TASK_CATEGORY_BUILDSYSTEM));
}

const QLatin1String AndroidPackageCreationStep::CreatePackageId("Qt4ProjectManager.AndroidPackageCreationStep");

} // namespace Internal
} // namespace Qt4ProjectManager
