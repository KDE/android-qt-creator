/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "generictarget.h"

#include "genericbuildconfiguration.h"
#include "genericproject.h"
#include "genericmakestep.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtGui/QApplication>
#include <QtGui/QStyle>

namespace {
const char * const GENERIC_DESKTOP_TARGET_DISPLAY_NAME("Desktop");
}

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;

////////////////////////////////////////////////////////////////////////////////////
// GenericTarget
////////////////////////////////////////////////////////////////////////////////////

GenericTarget::GenericTarget(GenericProject *parent) :
    ProjectExplorer::Target(parent, QLatin1String(GENERIC_DESKTOP_TARGET_ID)),
    m_buildConfigurationFactory(new GenericBuildConfigurationFactory(this)),
    m_deployConfigurationFactory(new ProjectExplorer::DeployConfigurationFactory(this))
{
    setDefaultDisplayName(QApplication::translate("GenericProjectManager::GenericTarget",
                                                  GENERIC_DESKTOP_TARGET_DISPLAY_NAME));
    setIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));
}

GenericTarget::~GenericTarget()
{
}

ProjectExplorer::BuildConfigWidget *GenericTarget::createConfigWidget()
{
    return new GenericBuildSettingsWidget(this);
}

GenericProject *GenericTarget::genericProject() const
{
    return static_cast<GenericProject *>(project());
}

GenericBuildConfigurationFactory *GenericTarget::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

ProjectExplorer::DeployConfigurationFactory *GenericTarget::deployConfigurationFactory() const
{
    return m_deployConfigurationFactory;
}

GenericBuildConfiguration *GenericTarget::activeBuildConfiguration() const
{
    return static_cast<GenericBuildConfiguration *>(Target::activeBuildConfiguration());
}

bool GenericTarget::fromMap(const QVariantMap &map)
{
    if (!Target::fromMap(map))
        return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////
// GenericTargetFactory
////////////////////////////////////////////////////////////////////////////////////

GenericTargetFactory::GenericTargetFactory(QObject *parent) :
    ITargetFactory(parent)
{
}

GenericTargetFactory::~GenericTargetFactory()
{
}

bool GenericTargetFactory::supportsTargetId(const QString &id) const
{
    return id == QLatin1String(GENERIC_DESKTOP_TARGET_ID);
}

QStringList GenericTargetFactory::availableCreationIds(ProjectExplorer::Project *parent) const
{
    if (!qobject_cast<GenericProject *>(parent))
        return QStringList();
    return QStringList() << QLatin1String(GENERIC_DESKTOP_TARGET_ID);
}

QString GenericTargetFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(GENERIC_DESKTOP_TARGET_ID))
        return QCoreApplication::translate("GenericProjectManager::GenericTarget",
                                           GENERIC_DESKTOP_TARGET_DISPLAY_NAME,
                                           "Generic desktop target display name");
    return QString();
}

bool GenericTargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<GenericProject *>(parent))
        return false;
    return id == QLatin1String(GENERIC_DESKTOP_TARGET_ID);
}

GenericTarget *GenericTargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    GenericProject *genericproject = static_cast<GenericProject *>(parent);
    GenericTarget *t = new GenericTarget(genericproject);

    // Set up BuildConfiguration:
    GenericBuildConfiguration *bc = new GenericBuildConfiguration(t);
    bc->setDisplayName("all");

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    GenericMakeStep *makeStep = new GenericMakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);

    makeStep->setBuildTarget("all", /* on = */ true);

    bc->setBuildDirectory(genericproject->projectDirectory());

    t->addBuildConfiguration(bc);

    t->addDeployConfiguration(t->deployConfigurationFactory()->create(t, ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID));

    // Add a runconfiguration. The CustomExecutableRC one will query the user
    // for its settings, so it is a good choice here.
    t->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(t));

    return t;
}

bool GenericTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

GenericTarget *GenericTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    GenericProject *genericproject = static_cast<GenericProject *>(parent);
    GenericTarget *target = new GenericTarget(genericproject);
    if (target->fromMap(map))
        return target;
    delete target;
    return 0;
}
