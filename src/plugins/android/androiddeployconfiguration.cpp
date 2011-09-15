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
#include "androidconstants.h"
#include "androidtarget.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>


#include <qt4projectmanager/qt4projectmanagerconstants.h>

using namespace Android::Internal;


ProjectExplorer::DeployConfigurationWidget *AndroidDeployConfiguration::configurationWidget() const
{
    return 0;
}

AndroidDeployConfiguration::AndroidDeployConfiguration(ProjectExplorer::Target *target, const QString &id)
    : ProjectExplorer::DeployConfiguration(target, id)
{

}

AndroidDeployConfiguration::AndroidDeployConfiguration(ProjectExplorer::Target *target, ProjectExplorer::DeployConfiguration *source)
    : ProjectExplorer::DeployConfiguration(target, source)
{

}

bool AndroidDeployConfiguration::fromMap(const QVariantMap &map)
{
    return ProjectExplorer::DeployConfiguration::fromMap(map);
}

QVariantMap AndroidDeployConfiguration::toMap() const
{
    return ProjectExplorer::DeployConfiguration::toMap();
}


/////////
// AndroidDeployConfigurationFactory
/////////

AndroidDeployConfigurationFactory::AndroidDeployConfigurationFactory(QObject *parent) :
    ProjectExplorer::DeployConfigurationFactory(parent)
{ }

QStringList AndroidDeployConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    AndroidTarget *target = qobject_cast<AndroidTarget *>(parent);
    if (!target)
        return QStringList();

    return QStringList() << Android::Internal::ANDROID_DEPLOY_ID;
}

QString AndroidDeployConfigurationFactory::displayNameForId(const QString &id) const
{
    if (id != Android::Internal::ANDROID_DEPLOY_ID)
        return QString();
    return tr("Deploy to a Android Device");
}

ProjectExplorer::DeployConfiguration *AndroidDeployConfigurationFactory::create(ProjectExplorer::Target *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    AndroidDeployConfiguration *dc = new AndroidDeployConfiguration(parent, id);
    if (!dc)
        return 0;
    if (parent->id() == QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
        dc->setDefaultDisplayName(tr("Deploy to Android device"));
    dc->stepList()->insertStep(0, new AndroidPackageInstallationStep(dc->stepList()));
    dc->stepList()->insertStep(1, new AndroidPackageCreationStep(dc->stepList()));
    dc->stepList()->insertStep(2, new AndroidDeployStep(dc->stepList()));
    return dc;
}

bool AndroidDeployConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const QString& id) const
{
    AndroidTarget *t = qobject_cast<AndroidTarget *>(parent);
    if (!t || id != QLatin1String(Android::Internal::ANDROID_DEPLOY_ID))
        return false;
    return true;
}

bool AndroidDeployConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap& map) const
{
    AndroidTarget *t = qobject_cast<AndroidTarget *>(parent);
    return t && ProjectExplorer::idFromMap(map) == QLatin1String(Android::Internal::ANDROID_DEPLOY_ID);
}

ProjectExplorer::DeployConfiguration *AndroidDeployConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    AndroidDeployConfiguration *dc = new AndroidDeployConfiguration(parent, ProjectExplorer::idFromMap(map));
    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

bool AndroidDeployConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source) const
{
    if (!qobject_cast<AndroidTarget *>(parent))
        return false;
    return source->id() == QLatin1String(ANDROID_DEPLOY_ID);
}

ProjectExplorer::DeployConfiguration *AndroidDeployConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return new AndroidDeployConfiguration(parent, source);
}
