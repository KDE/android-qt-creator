/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidrunconfiguration.h"
#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidrunconfigurationwidget.h"
#include "androidtoolchain.h"
#include "androidtarget.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <qtsupport/qtoutputformatter.h>

#include <QtCore/QStringBuilder>

using namespace Qt4ProjectManager;

namespace Android {
namespace Internal {

namespace {
const bool DefaultUseRemoteGdbValue = false;
} // anonymous namespace

using namespace ProjectExplorer;

AndroidRunConfiguration::AndroidRunConfiguration(AndroidTarget *parent,
        const QString &proFilePath)
    : RunConfiguration(parent, QLatin1String(ANDROID_RC_ID))
    , m_proFilePath(proFilePath)
    , m_useRemoteGdb(DefaultUseRemoteGdbValue)
    , m_baseEnvironmentBase(SystemEnvironmentBase)
    , m_validParse(parent->qt4Project()->validParse(m_proFilePath))
{
    init();
}

AndroidRunConfiguration::AndroidRunConfiguration(AndroidTarget *parent,
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

    Qt4Project *pro = androidTarget()->qt4Project();
    connect(pro, SIGNAL(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)),
            this, SLOT(proFileUpdate(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)));
}

AndroidRunConfiguration::~AndroidRunConfiguration()
{
}

AndroidTarget *AndroidRunConfiguration::androidTarget() const
{
    return static_cast<AndroidTarget *>(target());
}

Qt4BuildConfiguration *AndroidRunConfiguration::activeQt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(activeBuildConfiguration());
}

bool AndroidRunConfiguration::isEnabled(ProjectExplorer::BuildConfiguration *config) const
{
    if (!m_validParse)
        return false;
    return true;
}

QWidget *AndroidRunConfiguration::createConfigurationWidget()
{
    return new AndroidRunConfigurationWidget(this);
}

Utils::OutputFormatter *AndroidRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(androidTarget()->qt4Project());
}

void AndroidRunConfiguration::handleParseState(bool success)
{
    bool enabled = isEnabled();
    m_validParse = success;
    if (enabled != isEnabled())
        emit isEnabledChanged(!enabled);
}

void AndroidRunConfiguration::proFileUpdate(Qt4ProjectManager::Qt4ProFileNode *pro, bool success, bool parseInProgress)
{
    if (m_proFilePath == pro->path()) {
        handleParseState(success);
        if (!parseInProgress)
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

    m_validParse = androidTarget()->qt4Project()->validParse(m_proFilePath);

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
    return AndroidConfigurations::instance().gdbPath(activeQt4BuildConfiguration()->toolChain()->targetAbi().architecture());
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

const QString AndroidRunConfiguration::remoteChannel() const
{
#ifdef __GNUC__
#warning FIXME Android
#endif
    return QString(":5039");
}

const QString AndroidRunConfiguration::dumperLib() const
{
    Qt4BuildConfiguration *qt4bc(activeQt4BuildConfiguration());
    return qt4bc->qtVersion()->gdbDebuggingHelperLibrary();
}

QString AndroidRunConfiguration::localExecutableFilePath() const
{
    TargetInformation ti = androidTarget()->qt4Project()->rootProjectNode()
        ->targetInformation(m_proFilePath);
    if (!ti.valid)
        return QString();

    return QDir::cleanPath(ti.workingDir + QLatin1Char('/') + ti.target);
}

void AndroidRunConfiguration::setArguments(const QString &args)
{
    m_arguments = args;
}

QString AndroidRunConfiguration::proFilePath() const
{
    return m_proFilePath;
}

AndroidRunConfiguration::DebuggingType AndroidRunConfiguration::debuggingType() const
{
#ifdef __GNUC__
#warning FIXME Android
#endif

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
} // namespace Android
