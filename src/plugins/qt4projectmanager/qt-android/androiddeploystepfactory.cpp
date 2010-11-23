#include "androiddeploystepfactory.h"

#include "androiddeploystep.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <QtCore/QCoreApplication>

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

AndroidDeployStepFactory::AndroidDeployStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QStringList AndroidDeployStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        && parent->target()->id() == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID)
        && !parent->contains(AndroidDeployStep::Id))
        return QStringList() << AndroidDeployStep::Id;
    return QStringList();
}

QString AndroidDeployStepFactory::displayNameForId(const QString &id) const
{
    if (id == AndroidDeployStep::Id)
        return QCoreApplication::translate("Qt4ProjectManager::Internal::AndroidDeployStepFactory",
                                           "Deploy to device");
    return QString();
}

bool AndroidDeployStepFactory::canCreate(BuildStepList *parent, const QString &id) const
{
    return parent->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
            && id == QLatin1String(AndroidDeployStep::Id)
            && parent->target()->id() == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID)
            && !parent->contains(AndroidDeployStep::Id);
}

BuildStep *AndroidDeployStepFactory::create(BuildStepList *parent, const QString &id)
{
    Q_ASSERT(canCreate(parent, id));
    return new AndroidDeployStep(parent);
}

bool AndroidDeployStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *AndroidDeployStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));
    AndroidDeployStep * const step = new AndroidDeployStep(parent);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool AndroidDeployStepFactory::canClone(BuildStepList *parent, BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *AndroidDeployStepFactory::clone(BuildStepList *parent, BuildStep *product)
{
    Q_ASSERT(canClone(parent, product));
    return new AndroidDeployStep(parent, static_cast<AndroidDeployStep*>(product));
}

} // namespace Internal
} // namespace Qt4ProjectManager
