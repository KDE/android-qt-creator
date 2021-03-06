/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberryrunconfigurationfactory.h"
#include "qnxconstants.h"
#include "blackberryrunconfiguration.h"
#include "blackberrydeviceconfigurationfactory.h"

#include <projectexplorer/profileinformation.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4project.h>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
QString pathFromId(const Core::Id id)
{
    QString idStr = id.toString();
    if (idStr.startsWith(QLatin1String(Constants::QNX_BB_RUNCONFIGURATION_PREFIX)))
        return idStr.mid(QString::fromLatin1(Constants::QNX_BB_RUNCONFIGURATION_PREFIX).size());

    return QString();
}
}

BlackBerryRunConfigurationFactory::BlackBerryRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{
}

QList<Core::Id> BlackBerryRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> ids;
    if (!canHandle(parent))
        return ids;

    Qt4ProjectManager::Qt4Project *qt4Project = qobject_cast<Qt4ProjectManager::Qt4Project *>(parent->project());
    if (!qt4Project)
        return ids;

    QStringList proFiles = qt4Project->applicationProFilePathes(QLatin1String(Constants::QNX_BB_RUNCONFIGURATION_PREFIX));
    foreach (const QString &pf, proFiles)
        ids << Core::Id(pf);

    return ids;
}

QString BlackBerryRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    const QString path = pathFromId(id);
    if (path.isEmpty())
        return QString();

    if (id.toString().startsWith(QLatin1String(Constants::QNX_BB_RUNCONFIGURATION_PREFIX)))
        return tr("%1 on BlackBerry Device").arg(QFileInfo(path).completeBaseName());

    return QString();
}

bool BlackBerryRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;

    Qt4ProjectManager::Qt4Project *qt4Project = qobject_cast<Qt4ProjectManager::Qt4Project *>(parent->project());
    if (!qt4Project)
        return false;

    if (!id.toString().startsWith(QLatin1String(Constants::QNX_BB_RUNCONFIGURATION_PREFIX)))
        return false;

    return qt4Project->hasApplicationProFile(pathFromId(id));
}

ProjectExplorer::RunConfiguration *BlackBerryRunConfigurationFactory::create(ProjectExplorer::Target *parent,
                                                                      const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    return new BlackBerryRunConfiguration(parent, id, pathFromId(id));
}

bool BlackBerryRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent,
                                            const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;

    return ProjectExplorer::idFromMap(map).toString().startsWith(Constants::QNX_BB_RUNCONFIGURATION_PREFIX);
}

ProjectExplorer::RunConfiguration *BlackBerryRunConfigurationFactory::restore(
        ProjectExplorer::Target *parent,
        const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    ProjectExplorer::RunConfiguration *rc = 0;
    rc = new BlackBerryRunConfiguration(parent, Core::Id(Constants::QNX_BB_RUNCONFIGURATION_PREFIX), QString());
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

bool BlackBerryRunConfigurationFactory::canClone(ProjectExplorer::Target *parent,
                                          ProjectExplorer::RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::RunConfiguration *BlackBerryRunConfigurationFactory::clone(
        ProjectExplorer::Target *parent,
        ProjectExplorer::RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    BlackBerryRunConfiguration *old = static_cast<BlackBerryRunConfiguration *>(source);
    return new BlackBerryRunConfiguration(parent, old);

}

bool BlackBerryRunConfigurationFactory::canHandle(ProjectExplorer::Target *t) const
{
    if (!t->project()->supportsProfile(t->profile()))
        return false;
    if (!qobject_cast<Qt4ProjectManager::Qt4Project *>(t->project()))
        return false;

    Core::Id deviceType = ProjectExplorer::DeviceTypeProfileInformation::deviceTypeId(t->profile());
    if (deviceType != BlackBerryDeviceConfigurationFactory::deviceType())
        return false;

    return true;
}
