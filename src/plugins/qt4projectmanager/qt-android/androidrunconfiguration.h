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

#ifndef ANDROIDRUNCONFIGURATION_H
#define ANDROIDRUNCONFIGURATION_H

#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androiddeployable.h"

#include <utils/environment.h>

#include <projectexplorer/runconfiguration.h>

#include <QtCore/QDateTime>
#include <QtCore/QStringList>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace Qt4ProjectManager {

class Qt4BuildConfiguration;
class Qt4Project;
class Qt4Target;

namespace Internal {

class Qt4ProFileNode;

class AndroidDeviceConfigListModel;
class AndroidDeployStep;
class AndroidManager;
class AndroidRunConfigurationFactory;
class AndroidToolChain;

class AndroidRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class AndroidRunConfigurationFactory;

public:
    enum BaseEnvironmentBase {
        CleanEnvironmentBase = 0,
        SystemEnvironmentBase = 1
    };

    enum DebuggingType { DebugCppOnly, DebugQmlOnly, DebugCppAndQml };

    AndroidRunConfiguration(Qt4Target *parent, const QString &proFilePath);
    virtual ~AndroidRunConfiguration();

    using ProjectExplorer::RunConfiguration::isEnabled;
    bool isEnabled(ProjectExplorer::BuildConfiguration *config) const;
    QWidget *createConfigurationWidget();
    ProjectExplorer::OutputFormatter *createOutputFormatter() const;
    Qt4Target *qt4Target() const;
    Qt4BuildConfiguration *activeQt4BuildConfiguration() const;

    AndroidDeployStep *deployStep() const;

    const AndroidToolChain *toolchain() const;
    QString localExecutableFilePath() const;
    const QString arguments() const;
    void setArguments(const QString &args);
    AndroidConfig config() const;
    void updateFactoryState() { emit isEnabledChanged(true); }
    DebuggingType debuggingType() const;

    const QString gdbCmd() const;
    const QString remoteChannel();
    const QString dumperLib() const;

    virtual QVariantMap toMap() const;

    QString baseEnvironmentText() const;
    BaseEnvironmentBase baseEnvironmentBase() const;
    void setBaseEnvironmentBase(BaseEnvironmentBase env);

    Utils::Environment environment() const;
    Utils::Environment baseEnvironment() const;

    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);

    Utils::Environment systemEnvironment() const;
    void setSystemEnvironment(const Utils::Environment &environment);

    int portsUsedByDebuggers() const;

signals:
    void deviceConfigurationChanged(ProjectExplorer::Target *target);
    void targetInformationChanged() const;

    void baseEnvironmentChanged();
    void systemEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &diff);

protected:
    AndroidRunConfiguration(Qt4Target *parent, AndroidRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);
    QString defaultDisplayName();

private slots:
    void proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro, bool success);
    void proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode *pro);
    void updateDeviceConfigurations();
    void handleDeployConfigChanged();

private:
    void init();
    void handleParseState(bool success);

    QString m_proFilePath;
    mutable QString m_gdbPath;
    QString m_arguments;
    bool m_useRemoteGdb;

    BaseEnvironmentBase m_baseEnvironmentBase;
    Utils::Environment m_systemEnvironment;
    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
    bool m_validParse;
};

    } // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDRUNCONFIGURATION_H
