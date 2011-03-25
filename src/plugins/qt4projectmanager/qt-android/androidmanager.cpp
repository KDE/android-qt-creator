/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidmanager.h"

#include "androidconstants.h"
#include "androiddeploystepfactory.h"
#include "androidconfigurations.h"
#include "androidpackagecreationfactory.h"
#include "androidrunfactories.h"
#include "androidsettingspage.h"
#include "androidtoolchain.h"
#include "qt4androidtargetfactory.h"

#include <extensionsystem/pluginmanager.h>
#include <qt4projectmanager/qtversionmanager.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

using namespace ExtensionSystem;
using namespace ProjectExplorer;

namespace Qt4ProjectManager {
    namespace Internal {

AndroidManager *AndroidManager::m_instance = 0;

AndroidManager::AndroidManager()
    : QObject(0)
    , m_runControlFactory(new AndroidRunControlFactory(this))
    , m_runConfigurationFactory(new AndroidRunConfigurationFactory(this))
    , m_packageCreationFactory(new AndroidPackageCreationFactory(this))
    , m_deployStepFactory(new AndroidDeployStepFactory(this))
    , m_settingsPage(new AndroidSettingsPage(this))
    , m_androidTargetFactory(new Qt4AndroidTargetFactory())
    , m_toolChainFactory(new AndroidToolChainFactory)
{
    Q_ASSERT(!m_instance);

    m_instance = this;
    AndroidConfigurations::instance(this);

    PluginManager *pluginManager = PluginManager::instance();
    pluginManager->addObject(m_toolChainFactory);
    pluginManager->addObject(m_runControlFactory);
    pluginManager->addObject(m_runConfigurationFactory);
    pluginManager->addObject(m_packageCreationFactory);
    pluginManager->addObject(m_deployStepFactory);
    pluginManager->addObject(m_settingsPage);
    pluginManager->addObject(m_androidTargetFactory);
}

AndroidManager::~AndroidManager()
{
    PluginManager *pluginManager = PluginManager::instance();
    pluginManager->removeObject(m_runControlFactory);
    pluginManager->removeObject(m_runConfigurationFactory);
    pluginManager->removeObject(m_deployStepFactory);
    pluginManager->removeObject(m_packageCreationFactory);
    pluginManager->removeObject(m_settingsPage);
    pluginManager->removeObject(m_androidTargetFactory);
    pluginManager->removeObject(m_toolChainFactory);
    delete m_toolChainFactory;

    m_instance = 0;
}

AndroidManager &AndroidManager::instance()
{
    Q_ASSERT(m_instance);
    return *m_instance;
}

} // namespace Internal
} // namespace Qt4ProjectManager
