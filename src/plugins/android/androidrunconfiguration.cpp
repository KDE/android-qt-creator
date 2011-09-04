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

using namespace ProjectExplorer;

AndroidRunConfiguration::AndroidRunConfiguration(AndroidTarget *parent,
                                                 const QString &proFilePath)
    : RunConfiguration(parent, QLatin1String(ANDROID_RC_ID))
    , m_proFilePath(proFilePath)
{
    init();
}

AndroidRunConfiguration::AndroidRunConfiguration(AndroidTarget *parent,
                                                 AndroidRunConfiguration *source)
    : RunConfiguration(parent, source)
    , m_proFilePath(source->m_proFilePath)
    , m_gdbPath(source->m_gdbPath)
{
    init();
}

void AndroidRunConfiguration::init()
{
    setDefaultDisplayName(defaultDisplayName());
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

QWidget *AndroidRunConfiguration::createConfigurationWidget()
{
    return 0;// no special running configurations
}

Utils::OutputFormatter *AndroidRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(androidTarget()->qt4Project());
}

QString AndroidRunConfiguration::defaultDisplayName()
{
    return tr("Run on Android device");
}

AndroidConfig AndroidRunConfiguration::config() const
{
    return AndroidConfigurations::instance().config();
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



} // namespace Internal
} // namespace Android
