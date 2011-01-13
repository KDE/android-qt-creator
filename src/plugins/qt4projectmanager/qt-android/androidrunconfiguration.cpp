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

#include "androidrunconfiguration.h"

#include "androiddeploystep.h"
#include "androiddeviceconfiglistmodel.h"
#include "androidglobal.h"
#include "androidrunconfigurationwidget.h"
#include "androidtoolchain.h"
#include "qtoutputformatter.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QtCore/QStringBuilder>

namespace Qt4ProjectManager {
namespace Internal {

namespace {
const bool DefaultUseRemoteGdbValue = false;
} // anonymous namespace

using namespace ProjectExplorer;

AndroidRunConfiguration::AndroidRunConfiguration(Qt4Target *parent,
        const QString &proFilePath)
    : RunConfiguration(parent, QLatin1String(ANDROID_RC_ID))
    , m_proFilePath(proFilePath)
    , m_useRemoteGdb(DefaultUseRemoteGdbValue)
    , m_baseEnvironmentBase(SystemEnvironmentBase)
    , m_validParse(parent->qt4Project()->validParse(m_proFilePath))
{
    init();
}

AndroidRunConfiguration::AndroidRunConfiguration(Qt4Target *parent,
        AndroidRunConfiguration *source)
    : RunConfiguration(parent, source)
    , m_proFilePath(source->m_proFilePath)
    , m_gdbPath(source->m_gdbPath)
    , m_arguments(source->m_arguments)
    , m_baseEnvironmentBase(source->m_baseEnvironmentBase)
    , m_systemEnvironment(source->m_systemEnvironment)
    , m_userEnvironmentChanges(source->m_userEnvironmentChanges)
    , m_validParse(source->m_validParse)
{
    init();
}

void AndroidRunConfiguration::init()
{
    setDefaultDisplayName(defaultDisplayName());
    setUseCppDebugger(true);
    setUseQmlDebugger(false);

    connect(target(),
        SIGNAL(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)),
        this, SLOT(handleDeployConfigChanged()));
    handleDeployConfigChanged();

    Qt4Project *pro = qt4Target()->qt4Project();
    connect(pro, SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)),
            this, SLOT(proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)));
    connect(pro, SIGNAL(proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode *)),
            this, SLOT(proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode*)));
}

AndroidRunConfiguration::~AndroidRunConfiguration()
{
}

Qt4Target *AndroidRunConfiguration::qt4Target() const
{
    return static_cast<Qt4Target *>(target());
}

Qt4BuildConfiguration *AndroidRunConfiguration::activeQt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(activeBuildConfiguration());
}

bool AndroidRunConfiguration::isEnabled(ProjectExplorer::BuildConfiguration *config) const
{
    if (!m_validParse)
        return false;
    Qt4BuildConfiguration *qt4bc = qobject_cast<Qt4BuildConfiguration*>(config);
    QTC_ASSERT(qt4bc, return false);
    const ProjectExplorer::ToolChainType type = qt4bc->toolChainType();
    return type == ProjectExplorer::ToolChain_GCC_ANDROID;
}

QWidget *AndroidRunConfiguration::createConfigurationWidget()
{
    return new AndroidRunConfigurationWidget(this);
}

ProjectExplorer::OutputFormatter *AndroidRunConfiguration::createOutputFormatter() const
{
    return new QtOutputFormatter(qt4Target()->qt4Project());
}

void AndroidRunConfiguration::handleParseState(bool success)
{
    bool enabled = isEnabled();
    m_validParse = success;
    if (enabled != isEnabled()) {
        qDebug()<<"Emitting isEnabledChanged()"<<!enabled;
        emit isEnabledChanged(!enabled);
    }
}

void AndroidRunConfiguration::proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode *pro)
{
    if (m_proFilePath != pro->path())
        return;
    qDebug()<<"proFileInvalidated";
    handleParseState(false);
}

void AndroidRunConfiguration::proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro, bool success)
{
    if (m_proFilePath == pro->path()) {
        handleParseState(success);
        emit targetInformationChanged();
    }
}

QVariantMap AndroidRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(AndroidArgumentsKey, m_arguments);
    const QDir dir = QDir(target()->project()->projectDirectory());
    map.insert(AndroidProFileKey, dir.relativeFilePath(m_proFilePath));
    map.insert(AndroidBaseEnvironmentBaseKey, m_baseEnvironmentBase);
    map.insert(AndroidUserEnvironmentChangesKey,
        Utils::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    return map;
}

bool AndroidRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    m_arguments = map.value(AndroidArgumentsKey).toString();
    const QDir dir = QDir(target()->project()->projectDirectory());
    m_proFilePath = dir.filePath(map.value(AndroidProFileKey).toString());
    m_useRemoteGdb = map.value(AndroidUseRemoteGdbKey, DefaultUseRemoteGdbValue).toBool();
    m_userEnvironmentChanges =
        Utils::EnvironmentItem::fromStringList(map.value(AndroidUserEnvironmentChangesKey)
        .toStringList());
    m_baseEnvironmentBase = static_cast<BaseEnvironmentBase> (map.value(AndroidBaseEnvironmentBaseKey,
        SystemEnvironmentBase).toInt());

    m_validParse = qt4Target()->qt4Project()->validParse(m_proFilePath);

    setDefaultDisplayName(defaultDisplayName());

    return true;
}

QString AndroidRunConfiguration::defaultDisplayName()
{
    return tr("Run on Android device");
}

AndroidConfig AndroidRunConfiguration::config() const
{
    return AndroidConfigurations::instance().config();
}

const AndroidToolChain *AndroidRunConfiguration::toolchain() const
{
    Qt4BuildConfiguration *qt4bc(activeQt4BuildConfiguration());
    QTC_ASSERT(qt4bc, return 0);
    AndroidToolChain *tc = dynamic_cast<AndroidToolChain *>(qt4bc->toolChain());
    QTC_ASSERT(tc != 0, return 0);
    return tc;
}

const QString AndroidRunConfiguration::gdbCmd() const
{
//    return "/usr/bin/gdb";
#warning FIXME Android
    return QString("/usr/local/android-ndk-r5/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-gdb");
}

AndroidDeployStep *AndroidRunConfiguration::deployStep() const
{
    AndroidDeployStep * const step
        = AndroidGlobal::buildStep<AndroidDeployStep>(target()->activeDeployConfiguration());
    Q_ASSERT_X(step, Q_FUNC_INFO,
        "Impossible: Android build configuration without deploy step.");
    return step;
}


const QString AndroidRunConfiguration::arguments() const
{
    return m_arguments;
}

const QString AndroidRunConfiguration::remoteChannel()
{
#warning FIXME Android
    return QString(":5039");
}

const QString AndroidRunConfiguration::dumperLib() const
{
    Qt4BuildConfiguration *qt4bc(activeQt4BuildConfiguration());
    return qt4bc->qtVersion()->debuggingHelperLibrary();
}


QString AndroidRunConfiguration::localExecutableFilePath() const
{
    TargetInformation ti = qt4Target()->qt4Project()->rootProjectNode()
        ->targetInformation(m_proFilePath);
    if (!ti.valid)
        return QString();

    return QDir::cleanPath(ti.workingDir + QLatin1Char('/') + ti.target);
}

void AndroidRunConfiguration::setArguments(const QString &args)
{
    m_arguments = args;
}

AndroidRunConfiguration::DebuggingType AndroidRunConfiguration::debuggingType() const
{
#warning FIXME Android

//    if (!toolchain() || !toolchain()->allowsQmlDebugging())
//        return DebugCppOnly;
//    if (useCppDebugger()) {
//        if (useQmlDebugger())
            return DebugCppAndQml;
//        return DebugCppOnly;
//    }
//    return DebugQmlOnly;
}

int AndroidRunConfiguration::portsUsedByDebuggers() const
{
    switch (debuggingType()) {
    case DebugCppOnly:
    case DebugQmlOnly:
        return 1;
    case DebugCppAndQml:
    default:
        return 2;
    }
}

void AndroidRunConfiguration::updateDeviceConfigurations()
{
    emit deviceConfigurationChanged(target());
}

void AndroidRunConfiguration::handleDeployConfigChanged()
{
    return;
//    const QList<DeployConfiguration *> &deployConfigs
//        = target()->deployConfigurations();
//    DeployConfiguration * const activeDeployConf
//        = target()->activeDeployConfiguration();
//    for (int i = 0; i < deployConfigs.count(); ++i) {
//        AndroidDeployStep * const step
//            = AndroidGlobal::buildStep<AndroidDeployStep>(deployConfigs.at(i));
//        AndroidDeviceConfigListModel * const devConfigModel
//            = step->deviceConfigModel();
//        if (deployConfigs.at(i) == activeDeployConf) {
//            connect(devConfigModel, SIGNAL(currentChanged()), this,
//                SLOT(updateDeviceConfigurations()));
//            connect(devConfigModel, SIGNAL(modelReset()), this,
//                SLOT(updateDeviceConfigurations()));
//        } else {
//            disconnect(devConfigModel, 0, this,
//                SLOT(updateDeviceConfigurations()));
//        }
//    }
//    updateDeviceConfigurations();
}

QString AndroidRunConfiguration::baseEnvironmentText() const
{
    if (m_baseEnvironmentBase == CleanEnvironmentBase)
        return tr("Clean Environment");
    else  if (m_baseEnvironmentBase == SystemEnvironmentBase)
        return tr("System Environment");
    return QString();
}

AndroidRunConfiguration::BaseEnvironmentBase AndroidRunConfiguration::baseEnvironmentBase() const
{
    return m_baseEnvironmentBase;
}

void AndroidRunConfiguration::setBaseEnvironmentBase(BaseEnvironmentBase env)
{
    if (m_baseEnvironmentBase != env) {
        m_baseEnvironmentBase = env;
        emit baseEnvironmentChanged();
    }
}

Utils::Environment AndroidRunConfiguration::environment() const
{
    Utils::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

Utils::Environment AndroidRunConfiguration::baseEnvironment() const
{
    return (m_baseEnvironmentBase == SystemEnvironmentBase ? systemEnvironment()
        : Utils::Environment());
}

QList<Utils::EnvironmentItem> AndroidRunConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void AndroidRunConfiguration::setUserEnvironmentChanges(
    const QList<Utils::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges != diff) {
        m_userEnvironmentChanges = diff;
        emit userEnvironmentChangesChanged(diff);
    }
}

Utils::Environment AndroidRunConfiguration::systemEnvironment() const
{
    return m_systemEnvironment;
}

void AndroidRunConfiguration::setSystemEnvironment(const Utils::Environment &environment)
{
    if (m_systemEnvironment.size() == 0 || m_systemEnvironment != environment) {
        m_systemEnvironment = environment;
        emit systemEnvironmentChanged();
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
