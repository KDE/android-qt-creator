/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "androidpackagecreationfactory.h"

#include "androidpackagecreationstep.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qt4projectmanagerconstants.h>

#include <QtCore/QCoreApplication>

using ProjectExplorer::BuildStepList;
using ProjectExplorer::BuildStep;

namespace Qt4ProjectManager {
namespace Internal {

AndroidPackageCreationFactory::AndroidPackageCreationFactory(QObject *parent)
    : ProjectExplorer::IBuildStepFactory(parent)
{
}

QStringList AndroidPackageCreationFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        && parent->target()->id() == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID)
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
        && parent->target()->id() == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID)
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
} // namespace Qt4ProjectManager
