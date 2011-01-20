/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "deployconfiguration.h"

#include "buildmanager.h"
#include "buildsteplist.h"
#include "buildstepspage.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "target.h"

#include <QtCore/QStringList>

using namespace ProjectExplorer;

namespace {

const char * const BUILD_STEP_LIST_COUNT("ProjectExplorer.BuildConfiguration.BuildStepListCount");
const char * const BUILD_STEP_LIST_PREFIX("ProjectExplorer.BuildConfiguration.BuildStepList.");

} // namespace

DeployConfiguration::DeployConfiguration(Target *target, const QString &id) :
    ProjectConfiguration(target, id),
    m_stepList(0)
{
    Q_ASSERT(target);
    m_stepList = new BuildStepList(this, QLatin1String(Constants::BUILDSTEPS_DEPLOY));
    //: Display name of the deploy build step list. Used as part of the labels in the project window.
    m_stepList->setDefaultDisplayName(tr("Deploy"));
    //: Default DeployConfiguration display name
    setDefaultDisplayName(tr("No deployment"));
}

DeployConfiguration::DeployConfiguration(Target *target, DeployConfiguration *source) :
    ProjectConfiguration(target, source)
{
    Q_ASSERT(target);
    // Do not clone stepLists here, do that in the derived constructor instead
    // otherwise BuildStepFactories might reject to set up a BuildStep for us
    // since we are not yet the derived class!
}

DeployConfiguration::~DeployConfiguration()
{
    delete m_stepList;
}

BuildStepList *DeployConfiguration::stepList() const
{
    return m_stepList;
}

QVariantMap DeployConfiguration::toMap() const
{
    QVariantMap map(ProjectConfiguration::toMap());
    map.insert(QLatin1String(BUILD_STEP_LIST_COUNT), 1);
    map.insert(QLatin1String(BUILD_STEP_LIST_PREFIX) + QLatin1String("0"), m_stepList->toMap());
    return map;
}

DeployConfigurationWidget *DeployConfiguration::configurationWidget() const
{
    return 0;
}

bool DeployConfiguration::fromMap(const QVariantMap &map)
{
    if (!ProjectConfiguration::fromMap(map))
        return false;

    int maxI = map.value(QLatin1String(BUILD_STEP_LIST_COUNT), 0).toInt();
    if (maxI != 1)
        return false;
    QVariantMap data = map.value(QLatin1String(BUILD_STEP_LIST_PREFIX) + QLatin1String("0")).toMap();
    if (!data.isEmpty()) {
        m_stepList = new BuildStepList(this, data);
        if (m_stepList->isNull()) {
            qWarning() << "Failed to restore deploy step list";
            delete m_stepList;
            m_stepList = 0;
            return false;
        }
        m_stepList->setDefaultDisplayName(tr("Deploy"));
    } else {
        qWarning() << "No data for deploy step list found!";
        return false;
    }

    // TODO: We assume that we hold the deploy list
    Q_ASSERT(m_stepList && m_stepList->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY));

    return true;
}

Target *DeployConfiguration::target() const
{
    return static_cast<Target *>(parent());
}

void DeployConfiguration::cloneSteps(DeployConfiguration *source)
{
    if (source == this)
        return;
    delete m_stepList;
    m_stepList = new BuildStepList(this, source->stepList());
    m_stepList->cloneSteps(source->stepList());
}

///
// DeployConfigurationFactory
///

DeployConfigurationFactory::DeployConfigurationFactory(QObject *parent) :
    QObject(parent)
{ }

DeployConfigurationFactory::~DeployConfigurationFactory()
{ }

QStringList DeployConfigurationFactory::availableCreationIds(Target *parent) const
{
    Q_UNUSED(parent);
    return QStringList() << QLatin1String(Constants::DEFAULT_DEPLOYCONFIGURATION_ID);
}

QString DeployConfigurationFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(Constants::DEFAULT_DEPLOYCONFIGURATION_ID))
        //: Display name of the default deploy configuration
        return tr("Deploy Configuration");
    return QString();
}

bool DeployConfigurationFactory::canCreate(Target *parent, const QString &id) const
{
    Q_UNUSED(parent);
    return id == QLatin1String(Constants::DEFAULT_DEPLOYCONFIGURATION_ID);
}

DeployConfiguration *DeployConfigurationFactory::create(Target *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    return new DeployConfiguration(parent, id);
}

bool DeployConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

DeployConfiguration *DeployConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    DeployConfiguration *dc = new DeployConfiguration(parent, idFromMap(map));
    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

bool DeployConfigurationFactory::canClone(Target *parent, DeployConfiguration *product) const
{
    return canCreate(parent, product->id());
}

DeployConfiguration *DeployConfigurationFactory::clone(Target *parent, DeployConfiguration *product)
{
    if (!canClone(parent, product))
        return 0;
    return new DeployConfiguration(parent, product);
}

///
// DeployConfigurationWidget
///

DeployConfigurationWidget::DeployConfigurationWidget(QWidget *parent) : NamedWidget(parent)
{ }
