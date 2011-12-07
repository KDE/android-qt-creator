/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "remotelinuxrunconfiguration.h"

#include "deploymentinfo.h"
#include "linuxdeviceconfiguration.h"
#include "portlist.h"
#include "remotelinuxdeployconfiguration.h"
#include "remotelinuxrunconfigurationwidget.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtoutputformatter.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {
namespace {
const char ArgumentsKey[] = "Qt4ProjectManager.MaemoRunConfiguration.Arguments";
const char ProFileKey[] = "Qt4ProjectManager.MaemoRunConfiguration.ProFile";
const char BaseEnvironmentBaseKey[] = "Qt4ProjectManager.MaemoRunConfiguration.BaseEnvironmentBase";
const char UserEnvironmentChangesKey[]
    = "Qt4ProjectManager.MaemoRunConfiguration.UserEnvironmentChanges";
const char UseAlternateExeKey[] = "RemoteLinux.RunConfig.UseAlternateRemoteExecutable";
const char AlternateExeKey[] = "RemoteLinux.RunConfig.AlternateRemoteExecutable";
const char WorkingDirectoryKey[] = "RemoteLinux.RunConfig.WorkingDirectory";

} // anonymous namespace

class RemoteLinuxRunConfigurationPrivate {
public:
    RemoteLinuxRunConfigurationPrivate(const QString &proFilePath, const Qt4BaseTarget *target)
        : proFilePath(proFilePath),
          baseEnvironmentType(RemoteLinuxRunConfiguration::RemoteBaseEnvironment),
          validParse(target->qt4Project()->validParse(proFilePath)),
          parseInProgress(target->qt4Project()->parseInProgress(proFilePath)),
          useAlternateRemoteExecutable(false)
    {
    }

    RemoteLinuxRunConfigurationPrivate(const RemoteLinuxRunConfigurationPrivate *other)
        : proFilePath(other->proFilePath), gdbPath(other->gdbPath), arguments(other->arguments),
          baseEnvironmentType(other->baseEnvironmentType),
          remoteEnvironment(other->remoteEnvironment),
          userEnvironmentChanges(other->userEnvironmentChanges),
          validParse(other->validParse),
          parseInProgress(other->parseInProgress),
          useAlternateRemoteExecutable(other->useAlternateRemoteExecutable),
          alternateRemoteExecutable(other->alternateRemoteExecutable),
          workingDirectory(other->workingDirectory)
    {
    }

    QString proFilePath;
    QString gdbPath;
    QString arguments;
    RemoteLinuxRunConfiguration::BaseEnvironmentType baseEnvironmentType;
    Utils::Environment remoteEnvironment;
    QList<Utils::EnvironmentItem> userEnvironmentChanges;
    bool validParse;
    bool parseInProgress;
    QString disabledReason;
    bool useAlternateRemoteExecutable;
    QString alternateRemoteExecutable;
    QString workingDirectory;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxRunConfiguration::RemoteLinuxRunConfiguration(Qt4BaseTarget *parent, const QString &id,
        const QString &proFilePath)
    : RunConfiguration(parent, id),
      d(new RemoteLinuxRunConfigurationPrivate(proFilePath, parent))
{
    init();
}

RemoteLinuxRunConfiguration::RemoteLinuxRunConfiguration(Qt4BaseTarget *parent,
        RemoteLinuxRunConfiguration *source)
    : RunConfiguration(parent, source),
      d(new RemoteLinuxRunConfigurationPrivate(source->d))
{
    init();
}

void RemoteLinuxRunConfiguration::init()
{
    setDefaultDisplayName(defaultDisplayName());

    connect(target(),
        SIGNAL(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)),
        this, SLOT(handleDeployConfigChanged()));
    handleDeployConfigChanged();

    Qt4Project *pro = qt4Target()->qt4Project();
    connect(pro, SIGNAL(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)),
            this, SLOT(proFileUpdate(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)));
}

RemoteLinuxRunConfiguration::~RemoteLinuxRunConfiguration()
{
}

Qt4BaseTarget *RemoteLinuxRunConfiguration::qt4Target() const
{
    return static_cast<Qt4BaseTarget *>(target());
}

Qt4BuildConfiguration *RemoteLinuxRunConfiguration::activeQt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(activeBuildConfiguration());
}

bool RemoteLinuxRunConfiguration::isEnabled() const
{
    if (d->parseInProgress) {
        d->disabledReason = tr("The .pro file is being parsed.");
        return false;
    }
    if (!d->validParse) {
        d->disabledReason = tr("The .pro file could not be parsed.");
        return false;
    }
    if (!activeQt4BuildConfiguration()) {
        d->disabledReason = tr("No active build configuration.");
        return false;
    }
    if (remoteExecutableFilePath().isEmpty()) {
        d->disabledReason = tr("Don't know what to run.");
        return false;
    }
    d->disabledReason.clear();
    return true;
}

QString RemoteLinuxRunConfiguration::disabledReason() const
{
    return d->disabledReason;
}

QWidget *RemoteLinuxRunConfiguration::createConfigurationWidget()
{
    return new RemoteLinuxRunConfigurationWidget(this);
}

Utils::OutputFormatter *RemoteLinuxRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(qt4Target()->qt4Project());
}

void RemoteLinuxRunConfiguration::proFileUpdate(Qt4ProjectManager::Qt4ProFileNode *pro, bool success, bool parseInProgress)
{
    if (d->proFilePath == pro->path()) {
        bool enabled = isEnabled();
        d->validParse = success;
        d->parseInProgress = parseInProgress;
        if (enabled != isEnabled())
            updateEnabledState();
        if (!parseInProgress)
            emit targetInformationChanged();
    }
}

QVariantMap RemoteLinuxRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(QLatin1String(ArgumentsKey), d->arguments);
    const QDir dir = QDir(target()->project()->projectDirectory());
    map.insert(QLatin1String(ProFileKey), dir.relativeFilePath(d->proFilePath));
    map.insert(QLatin1String(BaseEnvironmentBaseKey), d->baseEnvironmentType);
    map.insert(QLatin1String(UserEnvironmentChangesKey),
        Utils::EnvironmentItem::toStringList(d->userEnvironmentChanges));
    map.insert(QLatin1String(UseAlternateExeKey), d->useAlternateRemoteExecutable);
    map.insert(QLatin1String(AlternateExeKey), d->alternateRemoteExecutable);
    map.insert(QLatin1String(WorkingDirectoryKey), d->workingDirectory);
    return map;
}

bool RemoteLinuxRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    d->arguments = map.value(QLatin1String(ArgumentsKey)).toString();
    const QDir dir = QDir(target()->project()->projectDirectory());
    d->proFilePath = QDir::cleanPath(dir.filePath(map.value(QLatin1String(ProFileKey)).toString()));
    d->userEnvironmentChanges =
        Utils::EnvironmentItem::fromStringList(map.value(QLatin1String(UserEnvironmentChangesKey))
        .toStringList());
    d->baseEnvironmentType = static_cast<BaseEnvironmentType>(map.value(QLatin1String(BaseEnvironmentBaseKey),
        RemoteBaseEnvironment).toInt());
    d->useAlternateRemoteExecutable = map.value(QLatin1String(UseAlternateExeKey), false).toBool();
    d->alternateRemoteExecutable = map.value(QLatin1String(AlternateExeKey)).toString();
    d->workingDirectory = map.value(QLatin1String(WorkingDirectoryKey)).toString();

    d->validParse = qt4Target()->qt4Project()->validParse(d->proFilePath);
    d->parseInProgress = qt4Target()->qt4Project()->parseInProgress(d->proFilePath);

    setDefaultDisplayName(defaultDisplayName());

    return true;
}

QString RemoteLinuxRunConfiguration::defaultDisplayName()
{
    if (!d->proFilePath.isEmpty())
        //: %1 is the name of a project which is being run on remote Linux
        return tr("%1 (on Remote Device)").arg(QFileInfo(d->proFilePath).completeBaseName());
    //: Remote Linux run configuration default display name
    return tr("Run on Remote Device");
}

LinuxDeviceConfiguration::ConstPtr RemoteLinuxRunConfiguration::deviceConfig() const
{
    return deployConfig()
        ? deployConfig()->deviceConfiguration() : LinuxDeviceConfiguration::ConstPtr();
}

QString RemoteLinuxRunConfiguration::gdbCmd() const
{
    return QDir::toNativeSeparators(activeBuildConfiguration()->toolChain()->debuggerCommand());
}

RemoteLinuxDeployConfiguration *RemoteLinuxRunConfiguration::deployConfig() const
{
    return qobject_cast<RemoteLinuxDeployConfiguration *>(target()->activeDeployConfiguration());
}

QString RemoteLinuxRunConfiguration::arguments() const
{
    return d->arguments;
}

QString RemoteLinuxRunConfiguration::environmentPreparationCommand() const
{
    QString command;
    const QStringList filesToSource = QStringList() << QLatin1String("/etc/profile")
        << QLatin1String("$HOME/.profile");
    foreach (const QString &filePath, filesToSource)
        command += QString::fromLocal8Bit("test -f %1 && source %1;").arg(filePath);
    if (!workingDirectory().isEmpty())
        command += QLatin1String("cd ") + workingDirectory();
    else
        command.chop(1); // Trailing semicolon.
    return command;
}

QString RemoteLinuxRunConfiguration::commandPrefix() const
{
    return QString::fromLocal8Bit("%1; DISPLAY=:0.0 %2")
        .arg(environmentPreparationCommand(), userEnvironmentChangesAsString());
}

QString RemoteLinuxRunConfiguration::localExecutableFilePath() const
{
    TargetInformation ti = qt4Target()->qt4Project()->rootQt4ProjectNode()
        ->targetInformation(d->proFilePath);
    if (!ti.valid)
        return QString();

    return QDir::cleanPath(ti.workingDir + QLatin1Char('/') + ti.target);
}

QString RemoteLinuxRunConfiguration::defaultRemoteExecutableFilePath() const
{
    return deployConfig()
        ? deployConfig()->deploymentInfo()->remoteExecutableFilePath(localExecutableFilePath())
        : QString();
}

QString RemoteLinuxRunConfiguration::remoteExecutableFilePath() const
{
    return d->useAlternateRemoteExecutable
        ? alternateRemoteExecutable() : defaultRemoteExecutableFilePath();
}

PortList RemoteLinuxRunConfiguration::freePorts() const
{
    const LinuxDeviceConfiguration::ConstPtr &devConf = deviceConfig();
    if (!devConf)
        return PortList();
    return devConf->freePorts();
}

void RemoteLinuxRunConfiguration::setArguments(const QString &args)
{
    d->arguments = args;
}

QString RemoteLinuxRunConfiguration::workingDirectory() const
{
    return d->workingDirectory;
}

void RemoteLinuxRunConfiguration::setWorkingDirectory(const QString &wd)
{
    d->workingDirectory = wd;
}

void RemoteLinuxRunConfiguration::setUseAlternateExecutable(bool useAlternate)
{
    d->useAlternateRemoteExecutable = useAlternate;
}

bool RemoteLinuxRunConfiguration::useAlternateExecutable() const
{
    return d->useAlternateRemoteExecutable;
}

void RemoteLinuxRunConfiguration::setAlternateRemoteExecutable(const QString &exe)
{
    d->alternateRemoteExecutable = exe;
}

QString RemoteLinuxRunConfiguration::alternateRemoteExecutable() const
{
    return d->alternateRemoteExecutable;
}

int RemoteLinuxRunConfiguration::portsUsedByDebuggers() const
{
    int ports = 0;
    if (useQmlDebugger())
        ++ports;
    if (useCppDebugger())
        ++ports;

    return ports;
}

void RemoteLinuxRunConfiguration::updateDeviceConfigurations()
{
    emit deviceConfigurationChanged(target());
    updateEnabledState();
}

void RemoteLinuxRunConfiguration::handleDeployConfigChanged()
{
    RemoteLinuxDeployConfiguration * const activeDeployConf = deployConfig();
    if (activeDeployConf) {
        connect(activeDeployConf->deploymentInfo().data(), SIGNAL(modelReset()),
            SLOT(handleDeployablesUpdated()), Qt::UniqueConnection);
        connect(activeDeployConf, SIGNAL(currentDeviceConfigurationChanged()),
            SLOT(updateDeviceConfigurations()), Qt::UniqueConnection);
    }

    updateDeviceConfigurations();
}

void RemoteLinuxRunConfiguration::handleDeployablesUpdated()
{
    emit deploySpecsChanged();
    updateEnabledState();
}

QString RemoteLinuxRunConfiguration::baseEnvironmentText() const
{
    if (d->baseEnvironmentType == CleanBaseEnvironment)
        return tr("Clean Environment");
    else  if (d->baseEnvironmentType == RemoteBaseEnvironment)
        return tr("System Environment");
    return QString();
}

RemoteLinuxRunConfiguration::BaseEnvironmentType RemoteLinuxRunConfiguration::baseEnvironmentType() const
{
    return d->baseEnvironmentType;
}

void RemoteLinuxRunConfiguration::setBaseEnvironmentType(BaseEnvironmentType env)
{
    if (d->baseEnvironmentType != env) {
        d->baseEnvironmentType = env;
        emit baseEnvironmentChanged();
    }
}

Utils::Environment RemoteLinuxRunConfiguration::environment() const
{
    Utils::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

Utils::Environment RemoteLinuxRunConfiguration::baseEnvironment() const
{
    return (d->baseEnvironmentType == RemoteBaseEnvironment ? remoteEnvironment()
        : Utils::Environment());
}

QList<Utils::EnvironmentItem> RemoteLinuxRunConfiguration::userEnvironmentChanges() const
{
    return d->userEnvironmentChanges;
}

QString RemoteLinuxRunConfiguration::userEnvironmentChangesAsString() const
{
    QString env;
    QString placeHolder = QLatin1String("%1=%2 ");
    foreach (const Utils::EnvironmentItem &item, userEnvironmentChanges())
        env.append(placeHolder.arg(item.name, item.value));
    return env.mid(0, env.size() - 1);
}

void RemoteLinuxRunConfiguration::setUserEnvironmentChanges(
    const QList<Utils::EnvironmentItem> &diff)
{
    if (d->userEnvironmentChanges != diff) {
        d->userEnvironmentChanges = diff;
        emit userEnvironmentChangesChanged(diff);
    }
}

Utils::Environment RemoteLinuxRunConfiguration::remoteEnvironment() const
{
    return d->remoteEnvironment;
}

void RemoteLinuxRunConfiguration::setRemoteEnvironment(const Utils::Environment &environment)
{
    if (d->remoteEnvironment.size() == 0 || d->remoteEnvironment != environment) {
        d->remoteEnvironment = environment;
        emit remoteEnvironmentChanged();
    }
}

QString RemoteLinuxRunConfiguration::proFilePath() const
{
    return d->proFilePath;
}

void RemoteLinuxRunConfiguration::setDisabledReason(const QString &reason) const
{
    d->disabledReason = reason;
}

const QString RemoteLinuxRunConfiguration::Id = QLatin1String("RemoteLinuxRunConfiguration");

} // namespace RemoteLinux
