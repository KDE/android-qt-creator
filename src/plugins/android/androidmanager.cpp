/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidmanager.h"

#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androiddeploystepfactory.h"
#include "androidconfigurations.h"
#include "androidpackagecreationfactory.h"
#include "androidpackageinstallationfactory.h"
#include "androidrunfactories.h"
#include "androidsettingspage.h"
#include "androidtoolchain.h"
#include "androidqtversionfactory.h"
#include "androidtargetfactory.h"


#include <QtCore/QtPlugin>


using namespace Android;
using namespace Android::Internal;

AndroidManager::AndroidManager()
{
}

AndroidManager::~AndroidManager()
{
}

bool AndroidManager::initialize(const QStringList &arguments,
    QString *error_message)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error_message)

    AndroidConfigurations::instance(this);

    addAutoReleasedObject(new AndroidRunControlFactory);
    addAutoReleasedObject(new AndroidRunConfigurationFactory);
    addAutoReleasedObject(new AndroidPackageInstallationFactory);
    addAutoReleasedObject(new AndroidPackageCreationFactory);
    addAutoReleasedObject(new AndroidDeployStepFactory);
    addAutoReleasedObject(new AndroidSettingsPage);
    addAutoReleasedObject(new AndroidTargetFactory);
    addAutoReleasedObject(new AndroidQtVersionFactory);
    addAutoReleasedObject(new AndroidToolChainFactory);
    return true;
}

void AndroidManager::extensionsInitialized()
{
}

Q_EXPORT_PLUGIN(Android::AndroidManager)
