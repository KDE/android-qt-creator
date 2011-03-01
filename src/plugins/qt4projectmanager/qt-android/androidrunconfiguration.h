/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDRUNCONFIGURATION_H
#define ANDROIDRUNCONFIGURATION_H

#include "androidconstants.h"
#include "androidconfigurations.h"

#include <utils/environment.h>

#include <projectexplorer/runconfiguration.h>

#include <QtCore/QDateTime>
#include <QtCore/QStringList>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace Qt4ProjectManager {

class Qt4BuildConfiguration;
class Qt4Project;

namespace Internal {

class Qt4ProFileNode;

class AndroidDeviceConfigListModel;
class AndroidDeployStep;
class AndroidRunConfigurationFactory;
class AndroidToolChain;
class Qt4AndroidTarget;

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

    AndroidRunConfiguration(Qt4AndroidTarget *parent, const QString &proFilePath);
    virtual ~AndroidRunConfiguration();

    using ProjectExplorer::RunConfiguration::isEnabled;
    bool isEnabled(ProjectExplorer::BuildConfiguration *config) const;
    QWidget *createConfigurationWidget();
    ProjectExplorer::OutputFormatter *createOutputFormatter() const;
    Qt4AndroidTarget *androidTarget() const;
    Qt4BuildConfiguration *activeQt4BuildConfiguration() const;

    AndroidDeployStep *deployStep() const;

    const AndroidToolChain *toolchain() const;
    QString localExecutableFilePath() const;
    const QString arguments() const;
    void setArguments(const QString &args);
    AndroidConfig config() const;
    void updateFactoryState() { emit isEnabledChanged(true); }
    QString proFilePath() const;

    DebuggingType debuggingType() const;

    const QString gdbCmd() const;
    const QString remoteChannel() const;
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
    AndroidRunConfiguration(Qt4AndroidTarget *parent, AndroidRunConfiguration *source);
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
