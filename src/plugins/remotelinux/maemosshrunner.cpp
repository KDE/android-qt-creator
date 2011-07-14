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

#include "maemosshrunner.h"

#include "maemoglobal.h"
#include "maemoqemumanager.h"
#include "maemoremotemounter.h"
#include "maemoremotemountsmodel.h"
#include "remotelinuxrunconfiguration.h"

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qtsupport/baseqtversion.h>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(MountState, state, m_mountState)

using namespace Qt4ProjectManager;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

MaemoSshRunner::MaemoSshRunner(QObject *parent, MaemoRunConfiguration *runConfig)
    : RemoteLinuxApplicationRunner(parent, runConfig),
      m_mounter(new MaemoRemoteMounter(this)),
      m_mountSpecs(runConfig->remoteMounts()->mountSpecs()),
      m_mountState(InactiveMountState)
{
    const Qt4BuildConfiguration * const bc = runConfig->activeQt4BuildConfiguration();
    m_qtId = bc && bc->qtVersion() ? bc->qtVersion()->uniqueId() : -1;
    m_mounter->setBuildConfiguration(bc);
    connect(m_mounter, SIGNAL(mounted()), this, SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(unmounted()), this, SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(error(QString)), this,
        SLOT(handleMounterError(QString)));
    connect(m_mounter, SIGNAL(reportProgress(QString)), this,
        SIGNAL(reportProgress(QString)));
    connect(m_mounter, SIGNAL(debugOutput(QString)), this,
        SIGNAL(mountDebugOutput(QString)));
}

MaemoSshRunner::~MaemoSshRunner() {}

bool MaemoSshRunner::canRun(QString &whyNot) const
{
    if (!RemoteLinuxApplicationRunner::canRun(whyNot))
        return false;

    if (devConfig()->type() == LinuxDeviceConfiguration::Emulator
            && !MaemoQemuManager::instance().qemuIsRunning()) {
        MaemoQemuRuntime rt;
        if (MaemoQemuManager::instance().runtimeForQtVersion(m_qtId, &rt)) {
            MaemoQemuManager::instance().startRuntime();
            whyNot = tr("Qemu was not running. It has now been started up for you, but it will "
                "take a bit of time until it is ready. Please try again then.");
        } else {
            whyNot = tr("You want to run on Qemu, but it is not enabled for this Qt version.");
        }
        return false;
    }

    return true;
}

void MaemoSshRunner::doAdditionalInitialCleanup()
{
    ASSERT_STATE(InactiveMountState);

    m_mounter->setConnection(connection(), devConfig());
    m_mounter->resetMountSpecifications();
    for (int i = 0; i < m_mountSpecs.count(); ++i)
        m_mounter->addMountSpecification(m_mountSpecs.at(i), false);
    m_mountState = InitialUnmounting;
    unmount();
}

void MaemoSshRunner::doAdditionalInitializations()
{
    mount();
}

void MaemoSshRunner::doAdditionalPostRunCleanup()
{
    ASSERT_STATE(Mounted);
    m_mountState = PostRunUnmounting;
    unmount();
}

void MaemoSshRunner::handleUnmounted()
{
    ASSERT_STATE(QList<MountState>() << InitialUnmounting << PostRunUnmounting);

    switch (m_mountState) {
    case InitialUnmounting:
        m_mountState = InactiveMountState;
        handleInitialCleanupDone(true);
        break;
    case PostRunUnmounting:
        m_mountState = InactiveMountState;
        handlePostRunCleanupDone();
        break;
    default:
        break;
    }
    m_mountState = InactiveMountState;
}

void MaemoSshRunner::doAdditionalConnectionErrorHandling()
{
    m_mountState = InactiveMountState;
}

void MaemoSshRunner::handleMounted()
{
    ASSERT_STATE(Mounting);

    if (m_mountState == Mounting) {
        m_mountState = Mounted;
        handleInitializationsDone(true);
    }
}

void MaemoSshRunner::handleMounterError(const QString &errorMsg)
{
    ASSERT_STATE(QList<MountState>() << InitialUnmounting << Mounting << PostRunUnmounting);

    const MountState oldMountState = m_mountState;
    m_mountState = InactiveMountState;
    emit error(errorMsg);
    switch (oldMountState) {
    case InitialUnmounting:
        handleInitialCleanupDone(false);
        break;
    case Mounting:
        handleInitializationsDone(false);
        break;
    case PostRunUnmounting:
        handlePostRunCleanupDone();
        break;
    default:
        break;
    }
}

void MaemoSshRunner::mount()
{
    m_mountState = Mounting;
    if (m_mounter->hasValidMountSpecifications()) {
        emit reportProgress(tr("Mounting host directories..."));
        m_mounter->mount(freePorts(), usedPortsGatherer());
    } else {
        handleMounted();
    }
}

void MaemoSshRunner::unmount()
{
    ASSERT_STATE(QList<MountState>() << InitialUnmounting << PostRunUnmounting);
    if (m_mounter->hasValidMountSpecifications()) {
        QString message;
        switch (m_mountState) {
        case InitialUnmounting:
            message = tr("Potentially unmounting left-over host directory mounts...");
            break;
        case PostRunUnmounting:
            message = tr("Unmounting host directories...");
            break;
        default:
            break;
        }
        emit reportProgress(message);
        m_mounter->unmount();
    } else {
        handleUnmounted();
    }
}

} // namespace Internal
} // namespace RemoteLinux

