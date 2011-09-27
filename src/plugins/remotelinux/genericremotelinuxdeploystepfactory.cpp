/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "genericremotelinuxdeploystepfactory.h"

#include "genericdirectuploadstep.h"
#include "remotelinuxdeployconfigurationfactory.h"
#include "remotelinuxcustomcommanddeploymentstep.h"
#include "tarpackagecreationstep.h"
#include "uploadandinstalltarpackagestep.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

GenericRemoteLinuxDeployStepFactory::GenericRemoteLinuxDeployStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QStringList GenericRemoteLinuxDeployStepFactory::availableCreationIds(BuildStepList *parent) const
{
    QStringList ids;
    const DeployConfiguration * const dc = qobject_cast<DeployConfiguration *>(parent->parent());
    if (!dc || dc->id() != RemoteLinuxDeployConfigurationFactory::genericDeployConfigurationId())
        return ids;
    ids << TarPackageCreationStep::stepId() << UploadAndInstallTarPackageStep::stepId()
        << GenericDirectUploadStep::stepId()
        << GenericRemoteLinuxCustomCommandDeploymentStep::stepId();
    return ids;
}

QString GenericRemoteLinuxDeployStepFactory::displayNameForId(const QString &id) const
{
    if (id == TarPackageCreationStep::stepId())
        return TarPackageCreationStep::displayName();
    if (id == UploadAndInstallTarPackageStep::stepId())
        return UploadAndInstallTarPackageStep::displayName();
    if (id == GenericDirectUploadStep::stepId())
        return GenericDirectUploadStep::displayName();
    if (id == GenericRemoteLinuxCustomCommandDeploymentStep::stepId())
        return GenericRemoteLinuxCustomCommandDeploymentStep::stepDisplayName();
    return QString();
}

bool GenericRemoteLinuxDeployStepFactory::canCreate(BuildStepList *parent, const QString &id) const
{
    return availableCreationIds(parent).contains(id);
}

BuildStep *GenericRemoteLinuxDeployStepFactory::create(BuildStepList *parent, const QString &id)
{
    Q_ASSERT(canCreate(parent, id));

    if (id == TarPackageCreationStep::stepId())
        return new TarPackageCreationStep(parent);
    if (id == UploadAndInstallTarPackageStep::stepId())
        return new UploadAndInstallTarPackageStep(parent);
    if (id == GenericDirectUploadStep::stepId())
        return new GenericDirectUploadStep(parent, GenericDirectUploadStep::stepId());
    if (id == GenericRemoteLinuxCustomCommandDeploymentStep::stepId())
        return new GenericRemoteLinuxCustomCommandDeploymentStep(parent);
    return 0;
}

bool GenericRemoteLinuxDeployStepFactory::canRestore(BuildStepList *parent,
    const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *GenericRemoteLinuxDeployStepFactory::restore(BuildStepList *parent,
    const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));

    BuildStep * const step = create(parent, idFromMap(map));
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool GenericRemoteLinuxDeployStepFactory::canClone(BuildStepList *parent, BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *GenericRemoteLinuxDeployStepFactory::clone(BuildStepList *parent, BuildStep *product)
{
    if (TarPackageCreationStep * const other = qobject_cast<TarPackageCreationStep *>(product))
        return new TarPackageCreationStep(parent, other);
    if (UploadAndInstallTarPackageStep * const other = qobject_cast<UploadAndInstallTarPackageStep*>(product))
        return new UploadAndInstallTarPackageStep(parent, other);
    if (GenericDirectUploadStep * const other = qobject_cast<GenericDirectUploadStep *>(product))
        return new GenericDirectUploadStep(parent, other);
    if (GenericRemoteLinuxCustomCommandDeploymentStep * const other = qobject_cast<GenericRemoteLinuxCustomCommandDeploymentStep *>(product))
        return new GenericRemoteLinuxCustomCommandDeploymentStep(parent, other);
    return 0;
}

} // namespace Internal
} // namespace RemoteLinux
