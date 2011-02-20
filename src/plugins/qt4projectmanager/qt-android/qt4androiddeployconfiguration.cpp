/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/


#include "androiddeploystep.h"
#include "androidpackagecreationstep.h"
#include "qt4androiddeployconfiguration.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>

#include <qt4projectmanager/qt4projectmanagerconstants.h>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

Qt4AndroidDeployConfigurationFactory::Qt4AndroidDeployConfigurationFactory(QObject *parent) :
    ProjectExplorer::DeployConfigurationFactory(parent)
{ }

ProjectExplorer::DeployConfiguration *Qt4AndroidDeployConfigurationFactory::create(ProjectExplorer::Target *parent, const QString &id)
{
    ProjectExplorer::DeployConfiguration *dc = ProjectExplorer::DeployConfigurationFactory::create(parent, id);
    if (!dc)
        return 0;
    if (parent->id() == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID))
        dc->setDefaultDisplayName(tr("Deploy to Android device"));
    dc->stepList()->insertStep(0, new AndroidPackageCreationStep(dc->stepList()));
    dc->stepList()->insertStep(1, new AndroidDeployStep(dc->stepList()));
    return dc;
}
