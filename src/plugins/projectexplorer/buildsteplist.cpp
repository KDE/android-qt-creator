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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "buildsteplist.h"

#include "buildconfiguration.h"
#include "buildmanager.h"
#include "buildstep.h"
#include "deployconfiguration.h"
#include "projectexplorer.h"
#include "target.h"

#include <extensionsystem/pluginmanager.h>

using namespace ProjectExplorer;

namespace {

IBuildStepFactory *findCloneFactory(BuildStepList *parent, BuildStep *source)
{
    QList<IBuildStepFactory *> factories = ExtensionSystem::PluginManager::instance()->getObjects<IBuildStepFactory>();
    foreach(IBuildStepFactory *factory, factories)
        if (factory->canClone(parent, source))
            return factory;
    return 0;
}

IBuildStepFactory *findRestoreFactory(BuildStepList *parent, const QVariantMap &map)
{
    QList<IBuildStepFactory *> factories = ExtensionSystem::PluginManager::instance()->getObjects<IBuildStepFactory>();
    foreach(IBuildStepFactory *factory, factories)
        if (factory->canRestore(parent, map))
            return factory;
    return 0;
}

const char * const STEPS_COUNT_KEY("ProjectExplorer.BuildStepList.StepsCount");
const char * const STEPS_PREFIX("ProjectExplorer.BuildStepList.Step.");

} // namespace

BuildStepList::BuildStepList(QObject *parent, const QString &id) :
    ProjectConfiguration(parent, id),
    m_isNull(false)
{
    Q_ASSERT(parent);
}

BuildStepList::BuildStepList(QObject *parent, BuildStepList *source) :
    ProjectConfiguration(parent, source),
    m_isNull(source->m_isNull)
{
    Q_ASSERT(parent);
    // do not clone the steps here:
    // The BC is not fully set up yet and thus some of the buildstepfactories
    // will fail to clone the buildsteps!
}

BuildStepList::BuildStepList(QObject *parent, const QVariantMap &data) :
    ProjectConfiguration(parent, QLatin1String("UNKNOWN ID"))
{
    Q_ASSERT(parent);
    m_isNull = !fromMap(data);
}

BuildStepList::~BuildStepList()
{
    qDeleteAll(m_steps);
}

QVariantMap BuildStepList::toMap() const
{
    QVariantMap map(ProjectConfiguration::toMap());
    // Save build steps
    map.insert(QString::fromLatin1(STEPS_COUNT_KEY), m_steps.count());
    for (int i = 0; i < m_steps.count(); ++i)
        map.insert(QString::fromLatin1(STEPS_PREFIX) + QString::number(i), m_steps.at(i)->toMap());

    return map;
}

bool BuildStepList::isNull() const
{
    return m_isNull;
}

int BuildStepList::count() const
{
    return m_steps.count();
}

bool BuildStepList::isEmpty() const
{
    return m_steps.isEmpty();
}

bool BuildStepList::contains(const QString &id) const
{
    foreach (BuildStep *bs, steps()) {
        if (bs->id() == id)
            return true;
    }
    return false;
}

void BuildStepList::cloneSteps(BuildStepList *source)
{
    Q_ASSERT(source);
    foreach (BuildStep *originalbs, source->steps()) {
        IBuildStepFactory *factory(findCloneFactory(this, originalbs));
        if (!factory)
            continue;
        BuildStep *clonebs(factory->clone(this, originalbs));
        if (clonebs)
            m_steps.append(clonebs);
    }
}

bool BuildStepList::fromMap(const QVariantMap &map)
{
    // We need the ID set before trying to restore the steps!
    if (!ProjectConfiguration::fromMap(map))
        return false;

    int maxSteps = map.value(QString::fromLatin1(STEPS_COUNT_KEY), 0).toInt();
    for (int i = 0; i < maxSteps; ++i) {
        QVariantMap bsData(map.value(QString::fromLatin1(STEPS_PREFIX) + QString::number(i)).toMap());
        if (bsData.isEmpty()) {
            qWarning() << "No step data found for" << i << "(continuing).";
            continue;
        }
        IBuildStepFactory *factory(findRestoreFactory(this, bsData));
        if (!factory) {
            qWarning() << "No factory for step" << i << "found (continuing).";
            continue;
        }
        BuildStep *bs(factory->restore(this, bsData));
        if (!bs) {
            qWarning() << "Restoration of step" << i << "failed (continuing).";
            continue;
        }
        insertStep(m_steps.count(), bs);
    }
    return true;
}

QList<BuildStep *> BuildStepList::steps() const
{
    return m_steps;
}

void BuildStepList::insertStep(int position, BuildStep *step)
{
    m_steps.insert(position, step);
    emit stepInserted(position);
}

bool BuildStepList::removeStep(int position)
{
    ProjectExplorer::BuildManager *bm =
            ProjectExplorer::ProjectExplorerPlugin::instance()->buildManager();
    BuildStep *bs = at(position);
    if (bm->isBuilding(bs))
        return false;

    emit aboutToRemoveStep(position);
    m_steps.removeAt(position);
    delete bs;
    emit stepRemoved(position);
    return true;
}

void BuildStepList::moveStepUp(int position)
{
    m_steps.swap(position - 1, position);
    emit stepMoved(position, position - 1);
}

BuildStep *BuildStepList::at(int position)
{
    return m_steps.at(position);
}

Target *BuildStepList::target() const
{
    Q_ASSERT(parent());
    BuildConfiguration *bc = qobject_cast<BuildConfiguration *>(parent());
    if (bc)
        return bc->target();
    DeployConfiguration *dc = qobject_cast<DeployConfiguration *>(parent());
    if (dc)
        return dc->target();
    return 0;
}
