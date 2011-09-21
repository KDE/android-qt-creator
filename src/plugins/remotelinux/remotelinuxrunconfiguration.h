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

#ifndef REMOTELINUXRUNCONFIGURATION_H
#define REMOTELINUXRUNCONFIGURATION_H

#include "remotelinux_export.h"

#include <projectexplorer/runconfiguration.h>
#include <utils/environment.h>

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
class Qt4BaseTarget;
class Qt4ProFileNode;
} // namespace Qt4ProjectManager

namespace RemoteLinux {
class LinuxDeviceConfiguration;
class PortList;
class RemoteLinuxRunConfigurationWidget;
class RemoteLinuxDeployConfiguration;

namespace Internal {
class RemoteLinuxRunConfigurationPrivate;
class RemoteLinuxRunConfigurationFactory;
} // namespace Internal

class REMOTELINUX_EXPORT RemoteLinuxRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    Q_DISABLE_COPY(RemoteLinuxRunConfiguration)
    friend class Internal::RemoteLinuxRunConfigurationFactory;
    friend class RemoteLinuxRunConfigurationWidget;

public:
    enum BaseEnvironmentType {
        CleanBaseEnvironment = 0,
        SystemBaseEnvironment = 1
    };

    enum DebuggingType { DebugCppOnly, DebugQmlOnly, DebugCppAndQml };

    RemoteLinuxRunConfiguration(Qt4ProjectManager::Qt4BaseTarget *parent, const QString &id,
        const QString &proFilePath);
    virtual ~RemoteLinuxRunConfiguration();

    bool isEnabled() const;
    QString disabledReason() const;
    QWidget *createConfigurationWidget();
    Utils::OutputFormatter *createOutputFormatter() const;
    Qt4ProjectManager::Qt4BaseTarget *qt4Target() const;
    Qt4ProjectManager::Qt4BuildConfiguration *activeQt4BuildConfiguration() const;

    RemoteLinuxDeployConfiguration *deployConfig() const;

    virtual QString environmentPreparationCommand() const;
    virtual QString commandPrefix() const;
    virtual PortList freePorts() const;
    virtual DebuggingType debuggingType() const;

    QString localExecutableFilePath() const;
    QString remoteExecutableFilePath() const;
    QString arguments() const;
    void setArguments(const QString &args);
    QSharedPointer<const LinuxDeviceConfiguration> deviceConfig() const;
    QString gdbCmd() const;

    virtual QVariantMap toMap() const;

    QString baseEnvironmentText() const;
    BaseEnvironmentType baseEnvironmentType() const;
    Utils::Environment environment() const;
    Utils::Environment baseEnvironment() const;
    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;
    Utils::Environment systemEnvironment() const;

    int portsUsedByDebuggers() const;

    QString proFilePath() const;

    static const QString Id;

signals:
    void deviceConfigurationChanged(ProjectExplorer::Target *target);
    void deploySpecsChanged();
    void targetInformationChanged() const;
    void baseEnvironmentChanged();
    void systemEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &diff);

protected:
    RemoteLinuxRunConfiguration(Qt4ProjectManager::Qt4BaseTarget *parent,
        RemoteLinuxRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);
    QString defaultDisplayName();
    void setDisabledReason(const QString &reason) const;
    QString userEnvironmentChangesAsString() const;
    Q_SLOT void updateEnabledState() { emit isEnabledChanged(isEnabled()); }

private slots:
    void proFileUpdate(Qt4ProjectManager::Qt4ProFileNode *pro, bool success, bool parseInProgress);
    void updateDeviceConfigurations();
    void handleDeployConfigChanged();
    void handleDeployablesUpdated();

private:
    void init();

    void setBaseEnvironmentType(BaseEnvironmentType env);
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);
    void setSystemEnvironment(const Utils::Environment &environment);

    Internal::RemoteLinuxRunConfigurationPrivate * const d;
};

} // namespace RemoteLinux

#endif // REMOTELINUXRUNCONFIGURATION_H
