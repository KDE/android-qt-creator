/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
#include "maemodeploybymountsteps.h"

#include "maemodeploymentmounter.h"
#include "maemoglobal.h"
#include "maemomountspecification.h"
#include "maemopackagecreationstep.h"
#include "maemopackageinstaller.h"
#include "maemoqemumanager.h"
#include "maemoremotecopyfacility.h"
#include "qt4maemodeployconfiguration.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qtsupport/baseqtversion.h>
#include <remotelinux/abstractremotelinuxdeployservice.h>
#include <remotelinux/deployablefile.h>
#include <remotelinux/deploymentinfo.h>
#include <remotelinux/linuxdeviceconfiguration.h>
#include <utils/qtcassert.h>
#include <utils/ssh/sshconnection.h>

#include <QtCore/QFileInfo>
#include <QtCore/QList>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {
class AbstractMaemoDeployByMountService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractMaemoDeployByMountService)
protected:
    AbstractMaemoDeployByMountService(QObject *parent);

    QString deployMountPoint() const;

private slots:
    void handleMounted();
    void handleUnmounted();
    void handleMountError(const QString &errorMsg);
    void handleInstallationFinished(const QString &errorMsg);

private:
    virtual QList<MaemoMountSpecification> mountSpecifications() const=0;
    virtual void doInstall() = 0;
    virtual void cancelInstallation() = 0;
    virtual void handleInstallationSuccess() = 0;

    void doDeviceSetup();
    void stopDeviceSetup();
    void doDeploy();
    void stopDeployment();

    void unmount();
    void setFinished();

    MaemoDeploymentMounter * const m_mounter;
    enum State { Inactive, Mounting, Installing, Unmounting } m_state;
    bool m_stopRequested;
};

class MaemoMountAndInstallPackageService : public AbstractMaemoDeployByMountService
{
    Q_OBJECT

public:
    MaemoMountAndInstallPackageService(QObject *parent);

    void setPackageFilePath(const QString &filePath) { m_packageFilePath = filePath; }

private:
    bool isDeploymentNecessary() const;

    QList<MaemoMountSpecification> mountSpecifications() const;
    void doInstall();
    void cancelInstallation();
    void handleInstallationSuccess();

    MaemoDebianPackageInstaller * const m_installer;
    QString m_packageFilePath;
};

class MaemoMountAndCopyFilesService : public AbstractMaemoDeployByMountService
{
    Q_OBJECT

public:
    MaemoMountAndCopyFilesService(QObject *parent);

    void setDeployableFiles(const QList<DeployableFile> &deployableFiles) {
        m_deployableFiles = deployableFiles;
    }

private:
    bool isDeploymentNecessary() const;

    QList<MaemoMountSpecification> mountSpecifications() const;
    void doInstall();
    void cancelInstallation();
    void handleInstallationSuccess();

    Q_SLOT void handleFileCopied(const RemoteLinux::DeployableFile &deployable);

    MaemoRemoteCopyFacility * const m_copyFacility;
    mutable QList<DeployableFile> m_filesToCopy;
    QList<DeployableFile> m_deployableFiles;
};


AbstractMaemoDeployByMountService::AbstractMaemoDeployByMountService(QObject *parent)
    : AbstractRemoteLinuxDeployService(parent),
      m_mounter(new MaemoDeploymentMounter(this)),
      m_state(Inactive),
      m_stopRequested(false)
{
    connect(m_mounter, SIGNAL(setupDone()), SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(tearDownDone()), SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(error(QString)), SLOT(handleMountError(QString)));
    connect(m_mounter, SIGNAL(reportProgress(QString)), SIGNAL(progressMessage(QString)));
    connect(m_mounter, SIGNAL(debugOutput(QString)), SIGNAL(stdErrData(QString)));
}

void AbstractMaemoDeployByMountService::doDeviceSetup()
{
    QTC_ASSERT(m_state == Inactive, return);

    if (deviceConfiguration()->deviceType() == LinuxDeviceConfiguration::Hardware) {
        handleDeviceSetupDone(true);
        return;
    }

    if (MaemoQemuManager::instance().qemuIsRunning()) {
        handleDeviceSetupDone(true);
        return;
    }

    MaemoQemuRuntime rt;
    const int qtId = qt4BuildConfiguration() && qt4BuildConfiguration()->qtVersion()
        ? qt4BuildConfiguration()->qtVersion()->uniqueId() : -1;
    if (MaemoQemuManager::instance().runtimeForQtVersion(qtId, &rt)) {
        MaemoQemuManager::instance().startRuntime();
        emit errorMessage(tr("Cannot deploy: Qemu was not running. "
            "It has now been started up for you, but it will take "
            "a bit of time until it is ready. Please try again then."));
    } else {
        emit errorMessage(tr("Cannot deploy: You want to deploy to Qemu, but it is not enabled "
            "for this Qt version."));
    }
    handleDeviceSetupDone(false);
}

void AbstractMaemoDeployByMountService::stopDeviceSetup()
{
    QTC_ASSERT(m_state == Inactive, return);

    handleDeviceSetupDone(false);
}

void AbstractMaemoDeployByMountService::doDeploy()
{
    QTC_ASSERT(m_state == Inactive, return);

    if (!qt4BuildConfiguration()) {
        emit errorMessage(tr("Missing build configuration."));
        setFinished();
        return;
    }

    m_state = Mounting;
    m_mounter->setupMounts(connection(), deviceConfiguration(), mountSpecifications(),
        qt4BuildConfiguration());
}

void AbstractMaemoDeployByMountService::stopDeployment()
{
    switch (m_state) {
    case Installing:
        m_stopRequested = true;
        cancelInstallation();

        // TODO: Possibly unsafe, because the mount point may still be in use if the
        // application did not exit immediately.
        unmount();
        break;
    case Mounting:
    case Unmounting:
        m_stopRequested = true;
        break;
    case Inactive:
        qWarning("%s: Unexpected state 'Inactive'.", Q_FUNC_INFO);
        break;
    }
}

void AbstractMaemoDeployByMountService::unmount()
{
    m_state = Unmounting;
    m_mounter->tearDownMounts();
}

void AbstractMaemoDeployByMountService::handleMounted()
{
    QTC_ASSERT(m_state == Mounting, return);

    if (m_stopRequested) {
        unmount();
        return;
    }

    m_state = Installing;
    doInstall();
}

void AbstractMaemoDeployByMountService::handleUnmounted()
{
    QTC_ASSERT(m_state == Unmounting, return);

    setFinished();
}

void AbstractMaemoDeployByMountService::handleMountError(const QString &errorMsg)
{
    QTC_ASSERT(m_state == Mounting, return);

    emit errorMessage(errorMsg);
    setFinished();
}

void AbstractMaemoDeployByMountService::handleInstallationFinished(const QString &errorMsg)
{
    QTC_ASSERT(m_state == Installing, return);

    if (errorMsg.isEmpty())
        handleInstallationSuccess();
    else
        emit errorMessage(errorMsg);
    unmount();
}

void AbstractMaemoDeployByMountService::setFinished()
{
    m_state = Inactive;
    m_stopRequested = false;
    handleDeploymentDone();
}

QString AbstractMaemoDeployByMountService::deployMountPoint() const
{
    return MaemoGlobal::homeDirOnDevice(deviceConfiguration()->sshParameters().userName)
        + QLatin1String("/deployMountPoint_")
        + qt4BuildConfiguration()->target()->project()->displayName();
}


MaemoMountAndInstallPackageService::MaemoMountAndInstallPackageService(QObject *parent)
    : AbstractMaemoDeployByMountService(parent), m_installer(new MaemoDebianPackageInstaller(this))
{
    connect(m_installer, SIGNAL(stdoutData(QString)), SIGNAL(stdOutData(QString)));
    connect(m_installer, SIGNAL(stderrData(QString)), SIGNAL(stdErrData(QString)));
    connect(m_installer, SIGNAL(finished(QString)), SLOT(handleInstallationFinished(QString)));
}

bool MaemoMountAndInstallPackageService::isDeploymentNecessary() const
{
    return hasChangedSinceLastDeployment(DeployableFile(m_packageFilePath, QString()));
}

QList<MaemoMountSpecification> MaemoMountAndInstallPackageService::mountSpecifications() const
{
    const QString localDir = QFileInfo(m_packageFilePath).absolutePath();
    return QList<MaemoMountSpecification>()
        << MaemoMountSpecification(localDir, deployMountPoint());
}

void MaemoMountAndInstallPackageService::doInstall()
{
    const QString remoteFilePath = deployMountPoint() + QLatin1Char('/')
        + QFileInfo(m_packageFilePath).fileName();
    m_installer->installPackage(connection(), remoteFilePath, false);
}

void MaemoMountAndInstallPackageService::cancelInstallation()
{
    m_installer->cancelInstallation();
}

void MaemoMountAndInstallPackageService::handleInstallationSuccess()
{
    saveDeploymentTimeStamp(DeployableFile(m_packageFilePath, QString()));
    emit progressMessage(tr("Package installed."));
}


MaemoMountAndCopyFilesService::MaemoMountAndCopyFilesService(QObject *parent)
    : AbstractMaemoDeployByMountService(parent), m_copyFacility(new MaemoRemoteCopyFacility(this))
{
    connect(m_copyFacility, SIGNAL(stdoutData(QString)), SIGNAL(stdOutData(QString)));
    connect(m_copyFacility, SIGNAL(stderrData(QString)), SIGNAL(stdErrData(QString)));
    connect(m_copyFacility, SIGNAL(progress(QString)), SIGNAL(progressMessage(QString)));
    connect(m_copyFacility, SIGNAL(fileCopied(RemoteLinux::DeployableFile)),
        SLOT(handleFileCopied(RemoteLinux::DeployableFile)));
    connect(m_copyFacility, SIGNAL(finished(QString)), SLOT(handleInstallationFinished(QString)));
}

bool MaemoMountAndCopyFilesService::isDeploymentNecessary() const
{
    m_filesToCopy.clear();
    for (int i = 0; i < m_deployableFiles.count(); ++i) {
        const DeployableFile &d = m_deployableFiles.at(i);
        if (hasChangedSinceLastDeployment(d) || QFileInfo(d.localFilePath).isDir())
            m_filesToCopy << d;
    }
    return !m_filesToCopy.isEmpty();
}

QList<MaemoMountSpecification> MaemoMountAndCopyFilesService::mountSpecifications() const
{
    QList<MaemoMountSpecification> mountSpecs;
#ifdef Q_OS_WIN
    bool drivesToMount[26];
    qFill(drivesToMount, drivesToMount + sizeof drivesToMount / sizeof drivesToMount[0], false);
    for (int i = 0; i < m_filesToCopy.count(); ++i) {
        const QString localDir = QFileInfo(m_filesToCopy.at(i).localFilePath).canonicalPath();
        const char driveLetter = localDir.at(0).toLower().toLatin1();
        if (driveLetter < 'a' || driveLetter > 'z') {
            qWarning("Weird: drive letter is '%c'.", driveLetter);
            continue;
        }

        const int index = driveLetter - 'a';
        if (drivesToMount[index])
            continue;

        const QString mountPoint = deployMountPoint() + QLatin1Char('/') + QLatin1Char(driveLetter);
        const MaemoMountSpecification mountSpec(localDir.left(3), mountPoint);
        mountSpecs << mountSpec;
        drivesToMount[index] = true;
    }
#else
    mountSpecs << MaemoMountSpecification(QLatin1String("/"), deployMountPoint());
#endif
    return mountSpecs;
}

void MaemoMountAndCopyFilesService::doInstall()
{
    m_copyFacility->copyFiles(connection(), deviceConfiguration(), m_filesToCopy,
        deployMountPoint());
}

void MaemoMountAndCopyFilesService::cancelInstallation()
{
    m_copyFacility->cancel();
}

void MaemoMountAndCopyFilesService::handleInstallationSuccess()
{
    emit progressMessage(tr("All files copied."));
}

void MaemoMountAndCopyFilesService::handleFileCopied(const DeployableFile &deployable)
{
    saveDeploymentTimeStamp(deployable);
}

MaemoInstallPackageViaMountStep::MaemoInstallPackageViaMountStep(BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    ctor();
}

MaemoInstallPackageViaMountStep::MaemoInstallPackageViaMountStep(BuildStepList *bsl,
        MaemoInstallPackageViaMountStep *other)
    : AbstractRemoteLinuxDeployStep(bsl, other)
{
    ctor();
}

void MaemoInstallPackageViaMountStep::ctor()
{
    m_deployService = new MaemoMountAndInstallPackageService(this);
    setDefaultDisplayName(displayName());
}

AbstractRemoteLinuxDeployService *MaemoInstallPackageViaMountStep::deployService() const
{
    return m_deployService;
}

bool MaemoInstallPackageViaMountStep::isDeploymentPossible(QString *whyNot) const
{
    const AbstractMaemoPackageCreationStep * const pStep
        = deployConfiguration()->earlierBuildStep<MaemoDebianPackageCreationStep>(this);
    if (!pStep) {
        if (whyNot)
            *whyNot = tr("No Debian package creation step found.");
        return false;
    }
    m_deployService->setPackageFilePath(pStep->packageFilePath());
    return AbstractRemoteLinuxDeployStep::isDeploymentPossible(whyNot);
}

QString MaemoInstallPackageViaMountStep::stepId()
{
    return QLatin1String("MaemoMountAndInstallDeployStep");
}

QString MaemoInstallPackageViaMountStep::displayName()
{
    return tr("Deploy package via UTFS mount");
}


MaemoCopyFilesViaMountStep::MaemoCopyFilesViaMountStep(BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    ctor();
}

MaemoCopyFilesViaMountStep::MaemoCopyFilesViaMountStep(BuildStepList *bsl,
        MaemoCopyFilesViaMountStep *other)
    : AbstractRemoteLinuxDeployStep(bsl, other)
{
    ctor();
}

void MaemoCopyFilesViaMountStep::ctor()
{
    m_deployService = new MaemoMountAndCopyFilesService(this);
    setDefaultDisplayName(displayName());
}

AbstractRemoteLinuxDeployService *MaemoCopyFilesViaMountStep::deployService() const
{
    return m_deployService;
}

bool MaemoCopyFilesViaMountStep::isDeploymentPossible(QString *whyNot) const
{
    QList<DeployableFile> deployableFiles;
    const QSharedPointer<DeploymentInfo> deploymentInfo = deployConfiguration()->deploymentInfo();
    const int deployableCount = deploymentInfo->deployableCount();
    for (int i = 0; i < deployableCount; ++i)
        deployableFiles << deploymentInfo->deployableAt(i);
    m_deployService->setDeployableFiles(deployableFiles);
    return AbstractRemoteLinuxDeployStep::isDeploymentPossible(whyNot);
}

QString MaemoCopyFilesViaMountStep::stepId()
{
    return QLatin1String("MaemoMountAndCopyDeployStep");
}

QString MaemoCopyFilesViaMountStep::displayName()
{
    return tr("Deploy files via UTFS mount");
}

} // namespace Internal
} // namespace Madde

#include "maemodeploybymountsteps.moc"
