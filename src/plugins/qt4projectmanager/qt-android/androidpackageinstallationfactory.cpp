#include "androidpackageinstallationfactory.h"

#include "androidpackageinstallationstep.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <QtCore/QCoreApplication>

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

AndroidPackageInstallationFactory::AndroidPackageInstallationFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QStringList AndroidPackageInstallationFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        && parent->target()->id() == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID)
        && !parent->contains(AndroidPackageInstallationStep::Id))
        return QStringList() << AndroidPackageInstallationStep::Id;
    return QStringList();
}

QString AndroidPackageInstallationFactory::displayNameForId(const QString &id) const
{
    if (id == AndroidPackageInstallationStep::Id)
        return QCoreApplication::translate("Qt4ProjectManager::Internal::AndroidPackageInstallationFactory",
                                           "Deploy to device");
    return QString();
}

bool AndroidPackageInstallationFactory::canCreate(BuildStepList *parent, const QString &id) const
{
    return parent->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
            && id == QLatin1String(AndroidPackageInstallationStep::Id)
            && parent->target()->id() == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID)
            && !parent->contains(AndroidPackageInstallationStep::Id);
}

BuildStep *AndroidPackageInstallationFactory::create(BuildStepList *parent, const QString &id)
{
    Q_ASSERT(canCreate(parent, id));
    return new AndroidPackageInstallationStep(parent);
}

bool AndroidPackageInstallationFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *AndroidPackageInstallationFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));
    AndroidPackageInstallationStep * const step = new AndroidPackageInstallationStep(parent);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool AndroidPackageInstallationFactory::canClone(BuildStepList *parent, BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *AndroidPackageInstallationFactory::clone(BuildStepList *parent, BuildStep *product)
{
    Q_ASSERT(canClone(parent, product));
    return new AndroidPackageInstallationStep(parent, static_cast<AndroidPackageInstallationStep*>(product));
}

} // namespace Internal
} // namespace Qt4ProjectManager
