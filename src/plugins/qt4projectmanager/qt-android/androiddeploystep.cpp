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
#include "androidremotemounter.h"
#include "androidrunconfiguration.h"
#include "androidtoolchain.h"
#include "androidusedportsgatherer.h"

#include <coreplugin/ssh/sftpchannel.h>
#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qt4projectmanager/qt4buildconfiguration.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

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
    : BuildStep(parent, other), m_lastDeployed(other->m_lastDeployed)
{
    ctor();
}

AndroidDeployStep::~AndroidDeployStep() { }

void AndroidDeployStep::ctor()
{
    //: AndroidDeployStep default display name
    setDefaultDisplayName(tr("Deploy to Android device"));

    // A AndroidDeployables object is only dependent on the active build
    // configuration and therefore can (and should) be shared among all
    // deploy steps.
    const QList<DeployConfiguration *> &deployConfigs
        = target()->deployConfigurations();
    if (deployConfigs.isEmpty()) {
        m_deployables = QSharedPointer<AndroidDeployables>(new AndroidDeployables(this));
    } else {
        const AndroidDeployStep *const other
            = AndroidGlobal::buildStep<AndroidDeployStep>(deployConfigs.first());
        m_deployables = other->deployables();
    }

    m_state = Inactive;
    m_needsInstall = false;
    m_deviceConfigModel = new AndroidDeviceConfigListModel(this);
    m_sysrootInstaller = new QProcess(this);
    connect(m_sysrootInstaller, SIGNAL(finished(int,QProcess::ExitStatus)),
        this, SLOT(handleSysrootInstallerFinished()));
    connect(m_sysrootInstaller, SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleSysrootInstallerOutput()));
    connect(m_sysrootInstaller, SIGNAL(readyReadStandardError()), this,
        SLOT(handleSysrootInstallerErrorOutput()));
    m_mounter = new AndroidRemoteMounter(this);
    connect(m_mounter, SIGNAL(mounted()), this, SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(unmounted()), this, SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(error(QString)), this,
        SLOT(handleMountError(QString)));
    connect(m_mounter, SIGNAL(reportProgress(QString)), this,
        SLOT(handleProgressReport(QString)));
    connect(m_mounter, SIGNAL(debugOutput(QString)), this,
        SLOT(handleMountDebugOutput(QString)));
    m_portsGatherer = new AndroidUsedPortsGatherer(this);
    connect(m_portsGatherer, SIGNAL(error(QString)), this,
        SLOT(handlePortsGathererError(QString)));
    connect(m_portsGatherer, SIGNAL(portListReady()), this,
        SLOT(handlePortListReady()));
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

QVariantMap AndroidDeployStep::toMap() const
{
    QVariantMap map(BuildStep::toMap());
    addDeployTimesToMap(map);
    map.insert(AndroidDeployToSysrootKey, m_deployToSysroot);
    map.unite(m_deviceConfigModel->toMap());
    return map;
}

void AndroidDeployStep::addDeployTimesToMap(QVariantMap &map) const
{
    QVariantList hostList;
    QVariantList fileList;
    QVariantList remotePathList;
    QVariantList timeList;
    typedef QHash<DeployablePerHost, QDateTime>::ConstIterator DepIt;
    for (DepIt it = m_lastDeployed.begin(); it != m_lastDeployed.end(); ++it) {
        fileList << it.key().first.localFilePath;
        remotePathList << it.key().first.remoteDir;
        hostList << it.key().second;
        timeList << it.value();
    }
    map.insert(AndroidLastDeployedHostsKey, hostList);
    map.insert(AndroidLastDeployedFilesKey, fileList);
    map.insert(AndroidLastDeployedRemotePathsKey, remotePathList);
    map.insert(AndroidLastDeployedTimesKey, timeList);
}

bool AndroidDeployStep::fromMap(const QVariantMap &map)
{
    if (!BuildStep::fromMap(map))
        return false;
    getDeployTimesFromMap(map);
    m_deviceConfigModel->fromMap(map);
    m_deployToSysroot = map.value(AndroidDeployToSysrootKey, true).toBool();
    return true;
}

void AndroidDeployStep::getDeployTimesFromMap(const QVariantMap &map)
{
    const QVariantList &hostList = map.value(AndroidLastDeployedHostsKey).toList();
    const QVariantList &fileList = map.value(AndroidLastDeployedFilesKey).toList();
    const QVariantList &remotePathList
        = map.value(AndroidLastDeployedRemotePathsKey).toList();
    const QVariantList &timeList = map.value(AndroidLastDeployedTimesKey).toList();
    const int elemCount
        = qMin(qMin(hostList.size(), fileList.size()),
            qMin(remotePathList.size(), timeList.size()));
    for (int i = 0; i < elemCount; ++i) {
        const AndroidDeployable d(fileList.at(i).toString(),
            remotePathList.at(i).toString());
        m_lastDeployed.insert(DeployablePerHost(d, hostList.at(i).toString()),
            timeList.at(i).toDateTime());
    }
}

const AndroidPackageCreationStep *AndroidDeployStep::packagingStep() const
{
    const AndroidPackageCreationStep * const step
        = AndroidGlobal::buildStep<AndroidPackageCreationStep>(target()->activeDeployConfiguration());
    Q_ASSERT_X(step, Q_FUNC_INFO,
        "Impossible: Android build configuration without packaging step.");
    return step;
}

void AndroidDeployStep::raiseError(const QString &errorString)
{
    emit addTask(Task(Task::Error, errorString, QString(), -1,
        Constants::TASK_CATEGORY_BUILDSYSTEM));
    emit error();
}

void AndroidDeployStep::writeOutput(const QString &text, OutputFormat format)
{
    emit addOutput(text, format);
}

void AndroidDeployStep::stop()
{
    if (m_state == StopRequested || m_state == Inactive)
        return;

    const State oldState = m_state;
    setState(StopRequested);
    switch (oldState) {
    case InstallingToSysroot:
        if (m_needsInstall)
            m_sysrootInstaller->terminate();
        break;
    case Connecting:
        m_connection->disconnectFromHost();
        setState(Inactive);
        break;
    case InstallingToDevice:
    case CopyingFile: {
        const QByteArray programToKill = oldState == CopyingFile
            ? " cp " : "dpkg";
        const QByteArray killCommand
            = AndroidGlobal::remoteSudo().toUtf8() + " pkill -f ";
        const QByteArray cmdLine = killCommand + programToKill + "; sleep 1; "
            + killCommand + "-9 " + programToKill;
        SshRemoteProcess::Ptr killProc
            = m_connection->createRemoteProcess(cmdLine);
        killProc->start();
        break;
    }
    case Uploading:
        m_uploader->closeChannel();
        break;
    case UnmountingOldDirs:
    case UnmountingCurrentDirs:
    case UnmountingCurrentMounts:
    case GatheringPorts:
    case Mounting:
    case InitializingSftp:
        break; // Nothing to do here.
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Missing switch case.");
    }
}

QString AndroidDeployStep::uploadDir() const
{
    return AndroidGlobal::homeDirOnDevice(m_connection->connectionParameters().uname);
}

bool AndroidDeployStep::currentlyNeedsDeployment(const QString &host,
    const AndroidDeployable &deployable) const
{
    const QDateTime &lastDeployed
        = m_lastDeployed.value(DeployablePerHost(deployable, host));
    return !lastDeployed.isValid()
        || QFileInfo(deployable.localFilePath).lastModified() > lastDeployed;
}

void AndroidDeployStep::setDeployed(const QString &host,
    const AndroidDeployable &deployable)
{
    m_lastDeployed.insert(DeployablePerHost(deployable, host),
        QDateTime::currentDateTime());
}

AndroidConfig AndroidDeployStep::deviceConfig() const
{
    return deviceConfigModel()->config();
}

void AndroidDeployStep::start()
{
    if (m_state != Inactive) {
        raiseError(tr("Cannot deploy: Still cleaning up from last time."));
        emit done();
        return;
    }

//    if (!deviceConfig().isValid()) {
//        raiseError(tr("Deployment failed: No valid device set."));
//        emit done();
//        return;
//    }

    Q_ASSERT(!m_currentDeviceDeployAction);
    Q_ASSERT(!m_needsInstall);
    Q_ASSERT(m_filesToCopy.isEmpty());
    const AndroidPackageCreationStep * const pStep = packagingStep();
//    const QString hostName = deviceConfig().server.host;
//    if (pStep->isPackagingEnabled()) {
//        const AndroidDeployable d(pStep->packageFilePath(), QString());
//        if (currentlyNeedsDeployment(hostName, d))
//            m_needsInstall = true;
//    } else {
//        const int deployableCount = m_deployables->deployableCount();
//        for (int i = 0; i < deployableCount; ++i) {
//            const AndroidDeployable &d = m_deployables->deployableAt(i);
//            if (currentlyNeedsDeployment(hostName, d))
//                m_filesToCopy << d;
//        }
//    }

    if (m_needsInstall || !m_filesToCopy.isEmpty()) {
        if (m_deployToSysroot)
            installToSysroot();
        else
            connectToDevice();
    } else {
        writeOutput(tr("All files up to date, no installation necessary."));
        emit done();
    }
}

void AndroidDeployStep::handleConnectionFailure()
{
    if (m_state != Inactive) {
        raiseError(tr("Could not connect to host: %1")
            .arg(m_connection->errorString()));
        setState(Inactive);
    }
}

void AndroidDeployStep::handleSftpChannelInitialized()
{
    ASSERT_STATE(QList<State>() << InitializingSftp << StopRequested);

    switch (m_state) {
    case InitializingSftp: {
        const QString filePath = packagingStep()->packageFilePath();
        const QString filePathNative = QDir::toNativeSeparators(filePath);
        const QString fileName = QFileInfo(filePath).fileName();
        const QString remoteFilePath = uploadDir() + QLatin1Char('/') + fileName;
        const SftpJobId job = m_uploader->uploadFile(filePath,
            remoteFilePath, SftpOverwriteExisting);
        if (job == SftpInvalidJob) {
            raiseError(tr("Upload failed: Could not open file '%1'")
                .arg(filePathNative));
            setState(Inactive);
        } else {
            setState(Uploading);
            writeOutput(tr("Started uploading file '%1'.").arg(filePathNative));
        }
        break;
    }
    case StopRequested:
        setState(Inactive);
        break;
    default:
        break;
    }
}

void AndroidDeployStep::handleSftpChannelInitializationFailed(const QString &error)
{
    ASSERT_STATE(QList<State>() << InitializingSftp << StopRequested);

    switch (m_state) {
    case InitializingSftp:
    case StopRequested:
        raiseError(tr("Could not set up SFTP connection: %1").arg(error));
        setState(Inactive);
        break;
    default:
        break;
    }
}

void AndroidDeployStep::handleSftpJobFinished(Core::SftpJobId,
    const QString &error)
{
    ASSERT_STATE(QList<State>() << Uploading << StopRequested);

    const QString filePathNative
        = QDir::toNativeSeparators(packagingStep()->packageFilePath());
    if (!error.isEmpty()) {
        raiseError(tr("Failed to upload file %1: %2")
            .arg(filePathNative, error));
        if (m_state == Uploading)
            setState(Inactive);
    } else if (m_state == Uploading) {
        writeOutput(tr("Successfully uploaded file '%1'.")
            .arg(filePathNative));
        const QString remoteFilePath
            = uploadDir() + QLatin1Char('/') + QFileInfo(filePathNative).fileName();
        runDpkg(remoteFilePath);
    }
}

void AndroidDeployStep::handleSftpChannelClosed()
{
    ASSERT_STATE(StopRequested);
    setState(Inactive);
}

void AndroidDeployStep::handleMounted()
{
    ASSERT_STATE(QList<State>() << Mounting << StopRequested << Inactive);

    switch (m_state) {
    case Mounting:
        if (m_needsInstall) {
            const QString remoteFilePath = deployMountPoint() + QLatin1Char('/')
                + QFileInfo(packagingStep()->packageFilePath()).fileName();
            runDpkg(remoteFilePath);
        } else {
            setState(CopyingFile);
            copyNextFileToDevice();
        }
        break;
    case StopRequested:
        unmount();
        break;
    case Inactive:
    default:
        break;
    }
}

void AndroidDeployStep::handleUnmounted()
{
    ASSERT_STATE(QList<State>() << UnmountingOldDirs << UnmountingCurrentDirs
        << UnmountingCurrentMounts << StopRequested << Inactive);

    switch (m_state) {
    case StopRequested:
        m_mounter->resetMountSpecifications();
        setState(Inactive);
        break;
    case UnmountingOldDirs:
        if (toolChain()->allowsRemoteMounts())
            setupMount();
        else
            prepareSftpConnection();
        break;
//    case UnmountingCurrentDirs:
//        setState(GatheringPorts);
//        m_portsGatherer->start(m_connection, deviceConfig().freePorts());
//        break;
    case UnmountingCurrentMounts:
        writeOutput(tr("Deployment finished."));
        setState(Inactive);
        break;
    case Inactive:
    default:
        break;
    }
}

void AndroidDeployStep::handleMountError(const QString &errorMsg)
{
    ASSERT_STATE(QList<State>() << UnmountingOldDirs << UnmountingCurrentDirs
        << UnmountingCurrentMounts << Mounting << StopRequested << Inactive);

    switch (m_state) {
    case UnmountingOldDirs:
    case UnmountingCurrentDirs:
    case UnmountingCurrentMounts:
    case StopRequested:
        raiseError(errorMsg);
        setState(Inactive);
        break;
    case Inactive:
    default:
        break;
    }
}

void AndroidDeployStep::handleMountDebugOutput(const QString &output)
{
    ASSERT_STATE(QList<State>() << UnmountingOldDirs << UnmountingCurrentDirs
        << UnmountingCurrentMounts << Mounting << StopRequested << Inactive);

    switch (m_state) {
    case UnmountingOldDirs:
    case UnmountingCurrentDirs:
    case UnmountingCurrentMounts:
    case StopRequested:
        writeOutput(output, ErrorOutput);
        break;
    case Inactive:
    default:
        break;
    }
}

void AndroidDeployStep::setupMount()
{
    ASSERT_STATE(UnmountingOldDirs);
    setState(UnmountingCurrentDirs);

    Q_ASSERT(m_needsInstall || !m_filesToCopy.isEmpty());
    m_mounter->resetMountSpecifications();
    m_mounter->setToolchain(toolChain());
    if (m_needsInstall) {
        const QString localDir
            = QFileInfo(packagingStep()->packageFilePath()).absolutePath();
        const AndroidMountSpecification mountSpec(localDir, deployMountPoint());
        m_mounter->addMountSpecification(mountSpec, true);
    } else {
#ifdef Q_OS_WIN
        bool drivesToMount[26];
        qFill(drivesToMount, drivesToMount + sizeof drivesToMount / sizeof drivesToMount[0], false);
        for (int i = 0; i < m_filesToCopy.count(); ++i) {
            const QString localDir
                = QFileInfo(m_filesToCopy.at(i).localFilePath).canonicalPath();
            const char driveLetter = localDir.at(0).toLower().toLatin1();
            if (driveLetter < 'a' || driveLetter > 'z') {
                qWarning("Weird: drive letter is '%c'.", driveLetter);
                continue;
            }

            const int index = driveLetter - 'a';
            if (drivesToMount[index])
                continue;

            const QString mountPoint = deployMountPoint() + QLatin1Char('/')
                + QLatin1Char(driveLetter);
            const AndroidMountSpecification mountSpec(localDir.left(3),
                mountPoint);
            m_mounter->addMountSpecification(mountSpec, true);
            drivesToMount[index] = true;
        }
#else
        m_mounter->addMountSpecification(AndroidMountSpecification(QLatin1String("/"),
            deployMountPoint()), true);
#endif
    }
    unmount();
}

void AndroidDeployStep::prepareSftpConnection()
{
    setState(InitializingSftp);
    m_uploader = m_connection->createSftpChannel();
    connect(m_uploader.data(), SIGNAL(initialized()), this,
        SLOT(handleSftpChannelInitialized()));
    connect(m_uploader.data(), SIGNAL(initializationFailed(QString)), this,
        SLOT(handleSftpChannelInitializationFailed(QString)));
    connect(m_uploader.data(), SIGNAL(finished(Core::SftpJobId, QString)),
        this, SLOT(handleSftpJobFinished(Core::SftpJobId, QString)));
    connect(m_uploader.data(), SIGNAL(closed()), this,
        SLOT(handleSftpChannelClosed()));
    m_uploader->initialize();
}

void AndroidDeployStep::installToSysroot()
{
    ASSERT_STATE(Inactive);
    setState(InstallingToSysroot);

    if (m_needsInstall) {
        writeOutput(tr("Installing package to sysroot ..."));
        const AndroidToolChain * const tc = toolChain();
        const QStringList args = QStringList() << QLatin1String("-t")
            << tc->targetName() << QLatin1String("xdpkg") << QLatin1String("-i")
            << packagingStep()->packageFilePath();
        m_sysrootInstaller->start(tc->madAdminCommand(), args);
        if (!m_sysrootInstaller->waitForStarted()) {
            writeOutput(tr("Installation to sysroot failed, continuing anyway."),
                ErrorMessageOutput);
            connectToDevice();
        }
    } else {
        writeOutput(tr("Copying files to sysroot ..."));
        Q_ASSERT(!m_filesToCopy.isEmpty());
        QDir sysRootDir(toolChain()->sysrootRoot());
        foreach (const AndroidDeployable &d, m_filesToCopy) {
            const QLatin1Char sep('/');
            const QString targetFilePath = toolChain()->sysrootRoot() + sep
                + d.remoteDir + sep + QFileInfo(d.localFilePath).fileName();
            sysRootDir.mkpath(d.remoteDir.mid(1));
            QFile::remove(targetFilePath);
            if (!QFile::copy(d.localFilePath, targetFilePath)) {
                writeOutput(tr("Sysroot installation failed: "
                    "Could not copy '%1' to '%2'. Continuing anyway.")
                    .arg(QDir::toNativeSeparators(d.localFilePath),
                         QDir::toNativeSeparators(targetFilePath)),
                    ErrorMessageOutput);
            }
            QCoreApplication::processEvents();
            if (m_state == StopRequested) {
                setState(Inactive);
                return;
            }
        }
        connectToDevice();
    }
}

void AndroidDeployStep::handleSysrootInstallerFinished()
{
    ASSERT_STATE(QList<State>() << InstallingToSysroot << StopRequested);

    if (m_state == StopRequested) {
        setState(Inactive);
        return;
    }

    if (m_sysrootInstaller->error() != QProcess::UnknownError
        || m_sysrootInstaller->exitCode() != 0) {
        writeOutput(tr("Installation to sysroot failed, continuing anyway."),
            ErrorMessageOutput);
    }
    connectToDevice();
}

void AndroidDeployStep::connectToDevice()
{
#warning FIXME Android
//    ASSERT_STATE(QList<State>() << Inactive << InstallingToSysroot);
//    setState(Connecting);

//    const bool canReUse = m_connection
//        && m_connection->state() == SshConnection::Connected
//        && m_connection->connectionParameters() == deviceConfig().server;
//    if (!canReUse)
//        m_connection = SshConnection::create();
//    connect(m_connection.data(), SIGNAL(connected()), this,
//        SLOT(handleConnected()));
//    connect(m_connection.data(), SIGNAL(error(Core::SshError)), this,
//        SLOT(handleConnectionFailure()));
//    if (canReUse) {
//        handleConnected();
//    } else {
//        writeOutput(tr("Connecting to device..."));
//        m_connection->connectToHost(deviceConfig().server);
//    }
}

void AndroidDeployStep::handleConnected()
{
    ASSERT_STATE(QList<State>() << Connecting << StopRequested);

    if (m_state == Connecting)
        unmountOldDirs();
}

void AndroidDeployStep::unmountOldDirs()
{
    setState(UnmountingOldDirs);
    m_mounter->setConnection(m_connection);
    unmount();
}

void AndroidDeployStep::runDpkg(const QString &packageFilePath)
{
    ASSERT_STATE(QList<State>() << Mounting << Uploading);
    const bool removeAfterInstall = m_state == Uploading;
    setState(InstallingToDevice);

    writeOutput(tr("Installing package to device..."));
    QByteArray cmd = AndroidGlobal::remoteSudo().toUtf8() + " dpkg -i "
        + packageFilePath.toUtf8();
    if (removeAfterInstall)
        cmd += " && (rm " + packageFilePath.toUtf8() + " || :)";
    m_deviceInstaller = m_connection->createRemoteProcess(cmd);
    connect(m_deviceInstaller.data(), SIGNAL(closed(int)), this,
        SLOT(handleInstallationFinished(int)));
    connect(m_deviceInstaller.data(), SIGNAL(outputAvailable(QByteArray)),
        this, SLOT(handleDeviceInstallerOutput(QByteArray)));
    connect(m_deviceInstaller.data(),
        SIGNAL(errorOutputAvailable(QByteArray)), this,
        SLOT(handleDeviceInstallerErrorOutput(QByteArray)));
    m_deviceInstaller->start();
}

void AndroidDeployStep::handleProgressReport(const QString &progressMsg)
{
    ASSERT_STATE(QList<State>() << UnmountingOldDirs << UnmountingCurrentDirs
        << UnmountingCurrentMounts << Mounting << StopRequested << Inactive);

    switch (m_state) {
    case UnmountingOldDirs:
    case UnmountingCurrentDirs:
    case UnmountingCurrentMounts:
    case StopRequested:
        writeOutput(progressMsg);
        break;
    case Inactive:
    default:
        break;
    }
}

void AndroidDeployStep::copyNextFileToDevice()
{
    ASSERT_STATE(CopyingFile);
    Q_ASSERT(!m_filesToCopy.isEmpty());
    Q_ASSERT(!m_currentDeviceDeployAction);
    const AndroidDeployable d = m_filesToCopy.takeFirst();
    QString sourceFilePath = deployMountPoint();
#ifdef Q_OS_WIN
    const QString localFilePath = QDir::fromNativeSeparators(d.localFilePath);
    sourceFilePath += QLatin1Char('/') + localFilePath.at(0).toLower()
        + localFilePath.mid(2);
#else
    sourceFilePath += d.localFilePath;
#endif

    QString command = QString::fromLatin1("%1 cp -r %2 %3")
        .arg(AndroidGlobal::remoteSudo(), sourceFilePath,
            d.remoteDir + QLatin1Char('/'));
    SshRemoteProcess::Ptr copyProcess
        = m_connection->createRemoteProcess(command.toUtf8());
    connect(copyProcess.data(), SIGNAL(errorOutputAvailable(QByteArray)),
        this, SLOT(handleDeviceInstallerErrorOutput(QByteArray)));
    connect(copyProcess.data(), SIGNAL(closed(int)), this,
        SLOT(handleCopyProcessFinished(int)));
    m_currentDeviceDeployAction.reset(new DeviceDeployAction(d, copyProcess));
    writeOutput(tr("Copying file '%1' to path '%2' on the device...")
        .arg(d.localFilePath, d.remoteDir));
    copyProcess->start();
}

void AndroidDeployStep::handleCopyProcessFinished(int exitStatus)
{
    ASSERT_STATE(QList<State>() << CopyingFile << StopRequested << Inactive);

    switch (m_state) {
    case CopyingFile: {
        Q_ASSERT(m_currentDeviceDeployAction);
        const QString localFilePath
            = m_currentDeviceDeployAction->first.localFilePath;
        if (exitStatus != SshRemoteProcess::ExitedNormally
                || m_currentDeviceDeployAction->second->exitCode() != 0) {
            raiseError(tr("Copying file '%1' failed.").arg(localFilePath));
            m_currentDeviceDeployAction.reset(0);
            setState(UnmountingCurrentMounts);
            unmount();
        } else {
            writeOutput(tr("Successfully copied file '%1'.").arg(localFilePath));
            setDeployed(m_connection->connectionParameters().host,
                m_currentDeviceDeployAction->first);
            m_currentDeviceDeployAction.reset(0);
            if (m_filesToCopy.isEmpty()) {
                writeOutput(tr("All files copied."));
                setState(UnmountingCurrentMounts);
                unmount();
            } else {
                copyNextFileToDevice();
            }
        }
        break;
    }
    case StopRequested:
        unmount();
        break;
    case Inactive:
    default:
        break;
    }
}

QString AndroidDeployStep::deployMountPoint() const
{
#warning FIXME Android
    return "";/*AndroidGlobal::homeDirOnDevice(deviceConfig().server.uname)
        + QLatin1String("/deployMountPoint_") + packagingStep()->projectName();*/
}

const AndroidToolChain *AndroidDeployStep::toolChain() const
{
    const Qt4BuildConfiguration * const bc
        = static_cast<Qt4BuildConfiguration *>(buildConfiguration());
    return static_cast<AndroidToolChain *>(bc->toolChain());
}

void AndroidDeployStep::handleSysrootInstallerOutput()
{
    ASSERT_STATE(QList<State>() << InstallingToSysroot << StopRequested);

    switch (m_state) {
    case InstallingToSysroot:
    case StopRequested:
        writeOutput(QString::fromLocal8Bit(m_sysrootInstaller->readAllStandardOutput()),
            NormalOutput);
        break;
    default:
        break;
    }
}

void AndroidDeployStep::handleSysrootInstallerErrorOutput()
{
    ASSERT_STATE(QList<State>() << InstallingToSysroot << StopRequested);

    switch (m_state) {
    case InstallingToSysroot:
    case StopRequested:
        writeOutput(QString::fromLocal8Bit(m_sysrootInstaller->readAllStandardError()),
            BuildStep::ErrorOutput);
        break;
    default:
        break;
    }
}

void AndroidDeployStep::handleInstallationFinished(int exitStatus)
{
    ASSERT_STATE(QList<State>() << InstallingToDevice << StopRequested
        << Inactive);

    switch (m_state) {
    case InstallingToDevice:
        if (exitStatus != SshRemoteProcess::ExitedNormally
            || m_deviceInstaller->exitCode() != 0) {
            raiseError(tr("Installing package failed."));
        } else {
            m_needsInstall = false;
            setDeployed(m_connection->connectionParameters().host,
                AndroidDeployable(packagingStep()->packageFilePath(), QString()));
            writeOutput(tr("Package installed."));
        }
        setState(UnmountingCurrentMounts);
        unmount();
        break;
    case StopRequested:
        unmount();
        break;
    case Inactive:
    default:
        break;
    }
}

void AndroidDeployStep::handlePortsGathererError(const QString &errorMsg)
{
    ASSERT_STATE(QList<State>() << GatheringPorts << StopRequested << Inactive);

    if (m_state != Inactive) {
        raiseError(errorMsg);
        setState(Inactive);
    }
}

void AndroidDeployStep::handlePortListReady()
{
    ASSERT_STATE(QList<State>() << GatheringPorts << StopRequested);

//    if (m_state == GatheringPorts) {
//        setState(Mounting);
//        m_freePorts = deviceConfig().freePorts();
//        m_mounter->mount(&m_freePorts, m_portsGatherer);
//    } else {
//        setState(Inactive);
//    }
}

void AndroidDeployStep::setState(State newState)
{
    if (newState == m_state)
        return;
    m_state = newState;
    if (m_state == Inactive) {
        m_needsInstall = false;
        m_filesToCopy.clear();
        m_currentDeviceDeployAction.reset(0);
        if (m_connection)
            disconnect(m_connection.data(), 0, this, 0);
        if (m_uploader) {
            disconnect(m_uploader.data(), 0, this, 0);
            m_uploader->closeChannel();
        }
        if (m_deviceInstaller)
            disconnect(m_deviceInstaller.data(), 0, this, 0);
        emit done();
    }
}

void AndroidDeployStep::unmount()
{
    if (m_mounter->hasValidMountSpecifications())
        m_mounter->unmount();
    else
        handleUnmounted();
}

void AndroidDeployStep::handleDeviceInstallerOutput(const QByteArray &output)
{
    ASSERT_STATE(QList<State>() << InstallingToDevice << StopRequested);

    switch (m_state) {
    case InstallingToDevice:
    case StopRequested:
        writeOutput(QString::fromUtf8(output), NormalOutput);
        break;
    default:
        break;
    }
}

void AndroidDeployStep::handleDeviceInstallerErrorOutput(const QByteArray &output)
{
    ASSERT_STATE(QList<State>() << InstallingToDevice << StopRequested);

    switch (m_state) {
    case InstallingToDevice:
    case StopRequested:
        writeOutput(QString::fromUtf8(output), ErrorOutput);
        break;
    default:
        break;
    }
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
