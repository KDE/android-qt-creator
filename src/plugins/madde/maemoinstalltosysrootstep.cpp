/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "maemoinstalltosysrootstep.h"

#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "maemoqtversion.h"

#include <utils/fileutils.h>

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4target.h>
#include <qtsupport/baseqtversion.h>
#include <remotelinux/deployablefile.h>
#include <remotelinux/deploymentinfo.h>
#include <remotelinux/remotelinuxdeployconfiguration.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLatin1Char>
#include <QtCore/QProcess>
#include <QtCore/QWeakPointer>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {

class AbstractMaemoInstallPackageToSysrootWidget : public BuildStepConfigWidget
{
    Q_OBJECT
public:
    AbstractMaemoInstallPackageToSysrootWidget(AbstractMaemoInstallPackageToSysrootStep *step)
        : m_step(step)
    {
        BuildStepList * const list
             = qobject_cast<BuildStepList *>(m_step->parent());
        connect(list, SIGNAL(stepInserted(int)), SIGNAL(updateSummary()));
        connect(list, SIGNAL(stepMoved(int,int)), SIGNAL(updateSummary()));
        connect(list, SIGNAL(aboutToRemoveStep(int)), SLOT(handleStepToBeRemoved(int)));
        connect(list, SIGNAL(stepRemoved(int)), SIGNAL(updateSummary()));
    }

    virtual QString summaryText() const
    {
        if (!m_step->deployConfiguration()->earlierBuildStep<AbstractMaemoPackageCreationStep>(m_step)) {
            return QLatin1String("<font color=\"red\">")
                + tr("Cannot deploy to sysroot: No packaging step found.")
                + QLatin1String("</font>");
        }
        return QLatin1String("<b>") + displayName() + QLatin1String("</b>");
    }

private:
    Q_SLOT void handleStepToBeRemoved(int step)
    {
        BuildStepList * const list
            = qobject_cast<BuildStepList *>(m_step->parent());
        if (list->steps().at(step) == m_step)
            disconnect(list, 0, this, 0);
    }

    const AbstractMaemoInstallPackageToSysrootStep * const m_step;
};


class MaemoInstallDebianPackageToSysrootWidget : public AbstractMaemoInstallPackageToSysrootWidget
{
    Q_OBJECT
public:
    MaemoInstallDebianPackageToSysrootWidget(AbstractMaemoInstallPackageToSysrootStep *step)
        : AbstractMaemoInstallPackageToSysrootWidget(step) {}

    virtual QString displayName() const { return MaemoInstallDebianPackageToSysrootStep::displayName(); }
};

class MaemoInstallRpmPackageToSysrootWidget : public AbstractMaemoInstallPackageToSysrootWidget
{
    Q_OBJECT
public:
    MaemoInstallRpmPackageToSysrootWidget(AbstractMaemoInstallPackageToSysrootStep *step)
        : AbstractMaemoInstallPackageToSysrootWidget(step) {}

    virtual QString displayName() const { return MaemoInstallRpmPackageToSysrootStep::displayName(); }
};

class MaemoCopyFilesToSysrootWidget : public BuildStepConfigWidget
{
    Q_OBJECT
public:
    MaemoCopyFilesToSysrootWidget(const BuildStep *buildStep)
        : m_buildStep(buildStep)
    {
        if (m_buildStep) {
            connect(m_buildStep.data(), SIGNAL(displayNameChanged()),
                SIGNAL(updateSummary()));
        }
    }
    virtual QString summaryText() const {
        return QLatin1String("<b>") + displayName() + QLatin1String("</b>"); }
    virtual QString displayName() const {
        return m_buildStep ? m_buildStep.data()->displayName() : QString();
    }
private:
    const QWeakPointer<const BuildStep> m_buildStep;
};


AbstractMaemoInstallPackageToSysrootStep::AbstractMaemoInstallPackageToSysrootStep(BuildStepList *bsl,
    const QString &id)
        : BuildStep(bsl, id)
{
}

AbstractMaemoInstallPackageToSysrootStep::AbstractMaemoInstallPackageToSysrootStep(BuildStepList *bsl,
    AbstractMaemoInstallPackageToSysrootStep *other)
        : BuildStep(bsl, other)
{
}

RemoteLinuxDeployConfiguration *AbstractMaemoInstallPackageToSysrootStep::deployConfiguration() const
{
    return qobject_cast<RemoteLinuxDeployConfiguration *>(BuildStep::deployConfiguration());
}

bool AbstractMaemoInstallPackageToSysrootStep::init()
{
    const Qt4BuildConfiguration * const bc
        = qobject_cast<Qt4BaseTarget *>(target())->activeQt4BuildConfiguration();
    if (!bc) {
        addOutput(tr("Cannot install to sysroot without build configuration."),
            ErrorMessageOutput);
        return false;
    }

    const AbstractMaemoPackageCreationStep * const pStep
        = deployConfiguration()->earlierBuildStep<AbstractMaemoPackageCreationStep>(this);
    if (!pStep) {
        addOutput(tr("Cannot install package to sysroot without packaging step."),
            ErrorMessageOutput);
        return false;
    }

    if (!bc->qtVersion()) {
        addOutput(tr("Cannot install package to sysroot without a Qt version."),
            ErrorMessageOutput);
        return false;
    }

    m_qmakeCommand = bc->qtVersion()->qmakeCommand();
    m_packageFilePath = pStep->packageFilePath();
    return true;
}

void AbstractMaemoInstallPackageToSysrootStep::run(QFutureInterface<bool> &fi)
{
    m_installerProcess = new QProcess;
    connect(m_installerProcess, SIGNAL(readyReadStandardOutput()),
        SLOT(handleInstallerStdout()));
    connect(m_installerProcess, SIGNAL(readyReadStandardError()),
        SLOT(handleInstallerStderr()));

    emit addOutput(tr("Installing package to sysroot ..."), MessageOutput);
    const int packageFileSize = QFileInfo(m_packageFilePath).size() / (1024*1024);
    const QStringList args = madArguments() << m_packageFilePath;
    MaemoGlobal::callMadAdmin(*m_installerProcess, args, m_qmakeCommand, true);
    if (!m_installerProcess->waitForFinished((2*packageFileSize + 10)*1000)
            || m_installerProcess->exitStatus() != QProcess::NormalExit
            || m_installerProcess->exitCode() != 0) {
        emit addOutput(tr("Installation to sysroot failed, continuing anyway."),
            ErrorMessageOutput);
        if (m_installerProcess->state() != QProcess::NotRunning) {
            m_installerProcess->terminate();
            m_installerProcess->waitForFinished();
            m_installerProcess->kill();
        }
        fi.reportResult(true);
        return;
    }

    fi.reportResult(true);
    m_installerProcess->deleteLater();
    m_installerProcess = 0;
}

void AbstractMaemoInstallPackageToSysrootStep::handleInstallerStdout()
{
    if (m_installerProcess)
        emit addOutput(QString::fromLocal8Bit(m_installerProcess->readAllStandardOutput()), NormalOutput);
}

void AbstractMaemoInstallPackageToSysrootStep::handleInstallerStderr()
{
    if (m_installerProcess)
        emit addOutput(QString::fromLocal8Bit(m_installerProcess->readAllStandardError()), ErrorOutput);
}


MaemoInstallDebianPackageToSysrootStep::MaemoInstallDebianPackageToSysrootStep(BuildStepList *bsl)
    : AbstractMaemoInstallPackageToSysrootStep(bsl, Id)
{
    setDisplayName(displayName());
}

MaemoInstallDebianPackageToSysrootStep::MaemoInstallDebianPackageToSysrootStep(BuildStepList *bsl,
    MaemoInstallDebianPackageToSysrootStep *other)
        : AbstractMaemoInstallPackageToSysrootStep(bsl, other)
{
    setDisplayName(displayName());
}

BuildStepConfigWidget *MaemoInstallDebianPackageToSysrootStep::createConfigWidget()
{
    return new MaemoInstallDebianPackageToSysrootWidget(this);
}

QStringList MaemoInstallDebianPackageToSysrootStep::madArguments() const
{
    return QStringList() << QLatin1String("xdpkg") << QLatin1String("--no-force-downgrade")
        << QLatin1String("-i");
}

const QString MaemoInstallDebianPackageToSysrootStep::Id
    = QLatin1String("MaemoInstallDebianPackageToSysrootStep");

QString MaemoInstallDebianPackageToSysrootStep::displayName()
{
    return tr("Install Debian package to sysroot");
}

MaemoInstallRpmPackageToSysrootStep::MaemoInstallRpmPackageToSysrootStep(BuildStepList *bsl)
    : AbstractMaemoInstallPackageToSysrootStep(bsl, Id)
{
    setDisplayName(displayName());
}

MaemoInstallRpmPackageToSysrootStep::MaemoInstallRpmPackageToSysrootStep(BuildStepList *bsl,
    MaemoInstallRpmPackageToSysrootStep *other)
        : AbstractMaemoInstallPackageToSysrootStep(bsl, other)
{
    setDisplayName(displayName());
}

BuildStepConfigWidget *MaemoInstallRpmPackageToSysrootStep::createConfigWidget()
{
    return new MaemoInstallRpmPackageToSysrootWidget(this);
}

QStringList MaemoInstallRpmPackageToSysrootStep::madArguments() const
{
    return QStringList() << QLatin1String("xrpm") << QLatin1String("-i");
}

const QString MaemoInstallRpmPackageToSysrootStep::Id
    = QLatin1String("MaemoInstallRpmPackageToSysrootStep");

QString MaemoInstallRpmPackageToSysrootStep::displayName()
{
    return tr("Install RPM package to sysroot");
}

MaemoCopyToSysrootStep::MaemoCopyToSysrootStep(BuildStepList *bsl)
    : BuildStep(bsl, Id)
{
    setDisplayName(displayName());
}

MaemoCopyToSysrootStep::MaemoCopyToSysrootStep(BuildStepList *bsl,
    MaemoCopyToSysrootStep *other)
        : BuildStep(bsl, other)
{
    setDisplayName(displayName());
}

bool MaemoCopyToSysrootStep::init()
{
    const Qt4BuildConfiguration * const bc
        = qobject_cast<Qt4BaseTarget *>(target())->activeQt4BuildConfiguration();
    if (!bc) {
        addOutput(tr("Cannot copy to sysroot without build configuration."),
            ErrorMessageOutput);
        return false;
    }

    const MaemoQtVersion * const qtVersion = dynamic_cast<MaemoQtVersion *>(bc->qtVersion());
    if (!qtVersion) {
        addOutput(tr("Cannot copy to sysroot without valid Qt version."),
            ErrorMessageOutput);
        return false;
    }
    m_systemRoot = qtVersion->systemRoot();
    return true;
}

void MaemoCopyToSysrootStep::run(QFutureInterface<bool> &fi)
{
    emit addOutput(tr("Copying files to sysroot ..."), MessageOutput);
    QDir sysrootDir(m_systemRoot);
    const QSharedPointer<DeploymentInfo> deploymentInfo
        = qobject_cast<RemoteLinuxDeployConfiguration *>(deployConfiguration())->deploymentInfo();
    const QChar sep = QLatin1Char('/');
    for (int i = 0; i < deploymentInfo->deployableCount(); ++i) {
        const DeployableFile &deployable = deploymentInfo->deployableAt(i);
        const QFileInfo localFileInfo(deployable.localFilePath);
        const QString targetFilePath = m_systemRoot + sep
            + deployable.remoteDir + sep + localFileInfo.fileName();
        sysrootDir.mkpath(deployable.remoteDir.mid(1));
        QString errorMsg;
        Utils::FileUtils::removeRecursively(targetFilePath, &errorMsg);
        if (!Utils::FileUtils::copyRecursively(deployable.localFilePath,
                targetFilePath, &errorMsg)) {
            emit addOutput(tr("Sysroot installation failed: %1\n"
                " Continuing anyway.").arg(errorMsg), ErrorMessageOutput);
        }
        QCoreApplication::processEvents();
        if (fi.isCanceled()) {
            fi.reportResult(false);
            return;
        }
    }
    fi.reportResult(true);
}

BuildStepConfigWidget *MaemoCopyToSysrootStep::createConfigWidget()
{
    return new MaemoCopyFilesToSysrootWidget(this);
}

const QString MaemoCopyToSysrootStep::Id
    = QLatin1String("MaemoCopyToSysrootStep");
QString MaemoCopyToSysrootStep::displayName()
{
    return tr("Copy files to sysroot");
}

MaemoMakeInstallToSysrootStep::MaemoMakeInstallToSysrootStep(BuildStepList *bsl)
    : AbstractProcessStep(bsl, Id)
{
    setDefaultDisplayName(displayName());
}

MaemoMakeInstallToSysrootStep::MaemoMakeInstallToSysrootStep(BuildStepList *bsl,
        MaemoMakeInstallToSysrootStep *other)
    : AbstractProcessStep(bsl, other)
{
    setDefaultDisplayName(displayName());
}

bool MaemoMakeInstallToSysrootStep::init()
{
    const Qt4BuildConfiguration * const bc
        = qobject_cast<Qt4BuildConfiguration *>(target()->activeBuildConfiguration());
    if (!bc) {
        addOutput("Cannot deploy: No active build dconfiguration.",
            ErrorMessageOutput);
        return false;
    }
    const QtSupport::BaseQtVersion * const qtVersion = bc->qtVersion();
    if (!qtVersion) {
        addOutput("Cannot deploy: Unusable build configuration.",
            ErrorMessageOutput);
        return false;

    }
    Utils::Environment env = bc->environment();
    MaemoGlobal::addMaddeEnvironment(env, qtVersion->qmakeCommand());
    QString command = MaemoGlobal::madCommand(qtVersion->qmakeCommand());
    QStringList args = QStringList() << QLatin1String("-t")
        << MaemoGlobal::targetName(qtVersion->qmakeCommand()) << QLatin1String("make")
        << QLatin1String("install") << (QLatin1String("INSTALL_ROOT=") + qtVersion->systemRoot());
    MaemoGlobal::transformMaddeCall(command, args, qtVersion->qmakeCommand());
    processParameters()->setCommand(command);
    processParameters()->setArguments(args.join(QLatin1String(" ")));
    processParameters()->setEnvironment(env);
    processParameters()->setWorkingDirectory(bc->buildDirectory());
    return true;
}

BuildStepConfigWidget *MaemoMakeInstallToSysrootStep::createConfigWidget()
{
    return new MaemoCopyFilesToSysrootWidget(this);
}

const QString MaemoMakeInstallToSysrootStep::Id
    = QLatin1String("MaemoMakeInstallToSysrootStep");
QString MaemoMakeInstallToSysrootStep::displayName()
{
    return tr("Copy files to sysroot");
}

} // namespace Internal
} // namespace Madde

#include "maemoinstalltosysrootstep.moc"
