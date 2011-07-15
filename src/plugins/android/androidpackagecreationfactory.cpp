/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidpackagecreationfactory.h"

#include "androidpackagecreationstep.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <QtCore/QCoreApplication>

using ProjectExplorer::BuildStepList;
using ProjectExplorer::BuildStep;

namespace Android  {
namespace Internal {

AndroidPackageCreationFactory::AndroidPackageCreationFactory(QObject *parent)
    : ProjectExplorer::IBuildStepFactory(parent)
{
}

QStringList AndroidPackageCreationFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        && parent->target()->id() == QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID)
        && !parent->contains(AndroidPackageCreationStep::CreatePackageId))
        return QStringList() << AndroidPackageCreationStep::CreatePackageId;
    return QStringList();
}

QString AndroidPackageCreationFactory::displayNameForId(const QString &id) const
{
    if (id == AndroidPackageCreationStep::CreatePackageId)
        return QCoreApplication::translate("Qt4ProjectManager::Internal::AndroidPackageCreationFactory",
                                           "Create Debian Package");
    return QString();
}

bool AndroidPackageCreationFactory::canCreate(ProjectExplorer::BuildStepList *parent, const QString &id) const
{
    return parent->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        && id == QLatin1String(AndroidPackageCreationStep::CreatePackageId)
        && parent->target()->id() == QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID)
        && !parent->contains(AndroidPackageCreationStep::CreatePackageId);
}

BuildStep *AndroidPackageCreationFactory::create(ProjectExplorer::BuildStepList *parent, const QString &id)
{
    Q_ASSERT(canCreate(parent, id));
    return new AndroidPackageCreationStep(parent);
}

bool AndroidPackageCreationFactory::canRestore(ProjectExplorer::BuildStepList *parent,
                                             const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

BuildStep *AndroidPackageCreationFactory::restore(ProjectExplorer::BuildStepList *parent,
                                                const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));
    AndroidPackageCreationStep * const step
        = new AndroidPackageCreationStep(parent);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool AndroidPackageCreationFactory::canClone(ProjectExplorer::BuildStepList *parent,
                                           ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *AndroidPackageCreationFactory::clone(ProjectExplorer::BuildStepList *parent,
                                              ProjectExplorer::BuildStep *product)
{
    Q_ASSERT(canClone(parent, product));
    return new AndroidPackageCreationStep(parent, static_cast<AndroidPackageCreationStep *>(product));
}

} // namespace Internal
} // namespace Android
