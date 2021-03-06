/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "linuxdeviceconfiguration.h"

#include "genericlinuxdeviceconfigurationwidget.h"
#include "linuxdevicetestdialog.h"
#include "publickeydeploymentdialog.h"
#include "remotelinuxprocessesdialog.h"
#include "remotelinuxprocesslist.h"
#include "remotelinux_constants.h"

#include <coreplugin/id.h>

#include <utils/portlist.h>
#include <ssh/sshconnection.h>
#include <utils/qtcassert.h>

namespace RemoteLinux {

const QLatin1String MachineTypeKey("Type");
const LinuxDeviceConfiguration::MachineType DefaultMachineType = LinuxDeviceConfiguration::Hardware;

LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::create(const QString &name,
       Core::Id type, MachineType machineType, Origin origin, Core::Id id)
{
    return Ptr(new LinuxDeviceConfiguration(name, type, machineType, origin, id));
}

QString LinuxDeviceConfiguration::displayType() const
{
    return tr("Generic Linux");
}

ProjectExplorer::IDeviceWidget *LinuxDeviceConfiguration::createWidget()
{
    return new GenericLinuxDeviceConfigurationWidget(sharedFromThis()
            .staticCast<LinuxDeviceConfiguration>());
}

QList<Core::Id> LinuxDeviceConfiguration::actionIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::GenericTestDeviceActionId)
        << Core::Id(Constants::GenericDeployKeyToDeviceActionId)
        << Core::Id(Constants::GenericRemoteProcessesActionId);
}

QString LinuxDeviceConfiguration::displayNameForActionId(Core::Id actionId) const
{
    QTC_ASSERT(actionIds().contains(actionId), return QString());

    if (actionId == Core::Id(Constants::GenericTestDeviceActionId))
        return tr("Test");
    if (actionId == Core::Id(Constants::GenericRemoteProcessesActionId))
        return tr("Remote Processes...");
    if (actionId == Core::Id(Constants::GenericDeployKeyToDeviceActionId))
        return tr("Deploy Public Key...");
    return QString(); // Can't happen.
}

void LinuxDeviceConfiguration::executeAction(Core::Id actionId, QWidget *parent) const
{
    QTC_ASSERT(actionIds().contains(actionId), return);

    QDialog *d = 0;
    const LinuxDeviceConfiguration::ConstPtr device
            = sharedFromThis().staticCast<const LinuxDeviceConfiguration>();
    if (actionId == Core::Id(Constants::GenericTestDeviceActionId))
        d = new LinuxDeviceTestDialog(device, new GenericLinuxDeviceTester, parent);
    else if (actionId == Core::Id(Constants::GenericRemoteProcessesActionId))
        d = new RemoteLinuxProcessesDialog(new GenericRemoteLinuxProcessList(device, parent));
    else if (actionId == Core::Id(Constants::GenericDeployKeyToDeviceActionId))
        d = PublicKeyDeploymentDialog::createDialog(device, parent);
    if (d)
        d->exec();
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const QString &name, Core::Id type,
        MachineType machineType, Origin origin, Core::Id id)
    : IDevice(type, origin, id)
{
    setDisplayName(name);
    m_machineType = machineType;
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const LinuxDeviceConfiguration &other)
    : IDevice(other)
{
    m_machineType = other.machineType();
}

LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::create()
{
    return Ptr(new LinuxDeviceConfiguration);
}

void LinuxDeviceConfiguration::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
    m_machineType = static_cast<MachineType>(map.value(MachineTypeKey, DefaultMachineType).toInt());
}

QVariantMap LinuxDeviceConfiguration::toMap() const
{
    QVariantMap map = IDevice::toMap();
    map.insert(MachineTypeKey, m_machineType);
    return map;
}

ProjectExplorer::IDevice::Ptr LinuxDeviceConfiguration::clone() const
{
    return Ptr(new LinuxDeviceConfiguration(*this));
}

LinuxDeviceConfiguration::MachineType LinuxDeviceConfiguration::machineType() const
{
    return m_machineType;
}

} // namespace RemoteLinux
