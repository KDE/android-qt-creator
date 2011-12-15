/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/


#include "androiddeploystep.h"
#include "androidpackageinstallationstep.h"
#include "androidpackagecreationstep.h"
#include "androiddeployconfiguration.h"
#include "androidtarget.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>

#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qt4projectmanager/qt4project.h>

using namespace Android::Internal;

AndroidDeployConfiguration::AndroidDeployConfiguration(ProjectExplorer::Target *parent)
    :DeployConfiguration(parent, ANDROID_DEPLOYCONFIGURATION_ID)
{
    setDisplayName(tr("Deploy to Android device"));
    setDefaultDisplayName(displayName());
}

AndroidDeployConfiguration::~AndroidDeployConfiguration()
{

}

AndroidDeployConfiguration::AndroidDeployConfiguration(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source)
    :DeployConfiguration(parent, source)
{

}

AndroidDeployConfigurationFactory::AndroidDeployConfigurationFactory(QObject *parent) :
    ProjectExplorer::DeployConfigurationFactory(parent)
{ }

bool AndroidDeployConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const QString &id) const
{
    AndroidTarget * t = qobject_cast<AndroidTarget *>(parent);
    if (!t || t->id() != QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID)
            || !id.startsWith(QLatin1String(ANDROID_DEPLOYCONFIGURATION_ID)))
        return false;
    return true;
}

ProjectExplorer::DeployConfiguration *AndroidDeployConfigurationFactory::create(ProjectExplorer::Target *parent, const QString &id)
{
    AndroidDeployConfiguration *dc = new AndroidDeployConfiguration(parent);
    if (!dc)
        return 0;
    dc->stepList()->insertStep(0, new AndroidPackageInstallationStep(dc->stepList()));
    dc->stepList()->insertStep(1, new AndroidPackageCreationStep(dc->stepList()));
    dc->stepList()->insertStep(2, new AndroidDeployStep(dc->stepList()));
    return dc;
}

bool AndroidDeployConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::DeployConfiguration *AndroidDeployConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    AndroidTarget *t = static_cast<AndroidTarget *>(parent);
    AndroidDeployConfiguration *dc = new AndroidDeployConfiguration(t);
    if (dc->fromMap(map))
        return dc;

    delete dc;
    return 0;
}

bool AndroidDeployConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source) const
{
    if (!qobject_cast<AndroidTarget*>(parent))
        return false;
    return source->id() == QLatin1String(ANDROID_DEPLOYCONFIGURATION_ID);
}

ProjectExplorer::DeployConfiguration *AndroidDeployConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    AndroidTarget *t = static_cast<AndroidTarget *>(parent);
    return new AndroidDeployConfiguration(t, source);
}

QStringList AndroidDeployConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    AndroidTarget *target = qobject_cast<AndroidTarget *>(parent);
    if (!target ||
        target->id() != QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
        return QStringList();

    return target->qt4Project()->applicationProFilePathes(QLatin1String(ANDROID_DC_PREFIX));
}

QString AndroidDeployConfigurationFactory::displayNameForId(const QString &id) const
{
    return tr("Deploy on Android");
}
