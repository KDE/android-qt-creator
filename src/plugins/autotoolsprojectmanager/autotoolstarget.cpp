/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010-2011 Openismus GmbH.
**   Authors: Peter Penz (ppenz@openismus.com)
**            Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#include "autotoolstarget.h"
#include "autotoolsproject.h"
#include "autotoolsprojectconstants.h"
#include "autotoolsbuildsettingswidget.h"
#include "autotoolsbuildconfiguration.h"
#include "makestep.h"
#include "autogenstep.h"
#include "autoreconfstep.h"
#include "configurestep.h"

#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <extensionsystem/pluginmanager.h>

#include <QtGui/QApplication>
#include <QtGui/QStyle>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;

static QString displayNameForId(const QString &id)
{
    if (id == QLatin1String(Constants::DEFAULT_AUTOTOOLS_TARGET_ID))
        return QApplication::translate("AutotoolsProjectManager::Internal::AutotoolsTarget",
                              "Desktop", "Autotools Default target display name");
    return QString();
}

//////////////////////////
// AutotoolsTarget class
//////////////////////////

AutotoolsTarget::AutotoolsTarget(AutotoolsProject *parent) :
    ProjectExplorer::Target(parent, QLatin1String(Constants::DEFAULT_AUTOTOOLS_TARGET_ID)),
    m_buildConfigurationFactory(new AutotoolsBuildConfigurationFactory(this))
{
    setDefaultDisplayName(displayNameForId(id()));
    setIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));
}

ProjectExplorer::BuildConfigWidget *AutotoolsTarget::createConfigWidget()
{
    return new AutotoolsBuildSettingsWidget(this);
}


AutotoolsProject *AutotoolsTarget::autotoolsProject() const
{
    return static_cast<AutotoolsProject *>(project());
}

AutotoolsBuildConfiguration *AutotoolsTarget::activeBuildConfiguration() const
{
    return static_cast<AutotoolsBuildConfiguration *>(Target::activeBuildConfiguration());
}

AutotoolsBuildConfigurationFactory *AutotoolsTarget::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

QString AutotoolsTarget::defaultBuildDirectory() const
{
    return autotoolsProject()->defaultBuildDirectory();
}

bool AutotoolsTarget::fromMap(const QVariantMap &map)
{
    return Target::fromMap(map);
}

/////////////////////////////////
// AutotoolsTargetFactory class
/////////////////////////////////
AutotoolsTargetFactory::AutotoolsTargetFactory(QObject *parent) :
    ITargetFactory(parent)
{
}

bool AutotoolsTargetFactory::supportsTargetId(const QString &id) const
{
    return id == QLatin1String(Constants::DEFAULT_AUTOTOOLS_TARGET_ID);
}

QStringList AutotoolsTargetFactory::supportedTargetIds(ProjectExplorer::Project *parent) const
{
    if (!qobject_cast<AutotoolsProject *>(parent))
        return QStringList();
    return QStringList() << QLatin1String(Constants::DEFAULT_AUTOTOOLS_TARGET_ID);
}

QString AutotoolsTargetFactory::displayNameForId(const QString &id) const
{
    return ::displayNameForId(id);
}

bool AutotoolsTargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<AutotoolsProject *>(parent))
        return false;
    return id == QLatin1String(Constants::DEFAULT_AUTOTOOLS_TARGET_ID);
}

AutotoolsTarget *AutotoolsTargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    AutotoolsProject *project(static_cast<AutotoolsProject *>(parent));
    AutotoolsTarget *t = new AutotoolsTarget(project);

    // Add default build configuration:
    AutotoolsBuildConfigurationFactory *bcf = ExtensionSystem::PluginManager::instance()->getObject<AutotoolsBuildConfigurationFactory>();
    AutotoolsBuildConfiguration *bc = bcf->createDefaultConfiguration(t);
    bc->setDisplayName("Default Build");

    t->addBuildConfiguration(bc);
    t->addDeployConfiguration(t->createDeployConfiguration(ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID));
    // User needs to choose where the executable file is.
    // TODO: Parse the file in *Anjuta style* to be able to add custom RunConfigurations.
    t->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(t));

    return t;
}

bool AutotoolsTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

AutotoolsTarget *AutotoolsTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    AutotoolsProject *autotoolsproject(static_cast<AutotoolsProject *>(parent));
    AutotoolsTarget *target = new AutotoolsTarget(autotoolsproject);
    if (target->fromMap(map))
        return target;
    delete target;
    return 0;
}
