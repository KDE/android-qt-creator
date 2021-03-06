/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "androidrunfactories.h"

#include "androidconstants.h"
#include "androiddebugsupport.h"
#include "androidrunconfiguration.h"
#include "androidruncontrol.h"
#include "androidmanager.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <debugger/debuggerconstants.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <qtsupport/qtprofileinformation.h>
#include <qtsupport/qtsupportconstants.h>


namespace Android {
namespace Internal {

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace {

QString pathFromId(const Core::Id id)
{
    QString pathStr = id.toString();
    const QString prefix = QLatin1String(ANDROID_RC_ID_PREFIX);
    if (!pathStr.startsWith(prefix))
        return QString();
    return pathStr.mid(prefix.size());
}

} // namespace

AndroidRunConfigurationFactory::AndroidRunConfigurationFactory(QObject *parent)
    : QmakeRunConfigurationFactory(parent)
{ setObjectName(QLatin1String("AndroidRunConfigurationFactory")); }

AndroidRunConfigurationFactory::~AndroidRunConfigurationFactory()
{ }

bool AndroidRunConfigurationFactory::canCreate(Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return availableCreationIds(parent).contains(id);
}

bool AndroidRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    QString id = ProjectExplorer::idFromMap(map).toString();
    return id.startsWith(QLatin1String(ANDROID_RC_ID_PREFIX));
}

bool AndroidRunConfigurationFactory::canClone(Target *parent,
    RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

QList<Core::Id> AndroidRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> ids;
    if (!AndroidManager::supportsAndroid(parent))
        return ids;
    QList<Qt4ProFileNode *> nodes = static_cast<Qt4Project *>(parent->project())->allProFiles();
    foreach (Qt4ProFileNode *node, nodes)
        if (node->projectType() == ApplicationTemplate || node->projectType() == LibraryTemplate)
            ids << Core::Id(ANDROID_RC_ID_PREFIX + node->targetInformation().target);
    return ids;
}

QString AndroidRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    return QFileInfo(pathFromId(id)).completeBaseName();
}

RunConfiguration *AndroidRunConfigurationFactory::create(Target *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new AndroidRunConfiguration(parent, id, pathFromId(id));
}

RunConfiguration *AndroidRunConfigurationFactory::restore(Target *parent,
    const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Core::Id id = ProjectExplorer::idFromMap(map);
    AndroidRunConfiguration *rc = new AndroidRunConfiguration(parent, id, pathFromId(id));
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

RunConfiguration *AndroidRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    AndroidRunConfiguration *old = static_cast<AndroidRunConfiguration *>(source);
    return new AndroidRunConfiguration(parent, old);
}

bool AndroidRunConfigurationFactory::canHandle(Target *t) const
{
    if (!t->project()->supportsProfile(t->profile()))
        return false;
    return AndroidManager::supportsAndroid(t);
}

QList<RunConfiguration *> AndroidRunConfigurationFactory::runConfigurationsForNode(Target *t, ProjectExplorer::Node *n)
{
    QList<ProjectExplorer::RunConfiguration *> result;
    foreach (ProjectExplorer::RunConfiguration *rc, t->runConfigurations())
        if (AndroidRunConfiguration *qt4c = qobject_cast<AndroidRunConfiguration *>(rc))
                if (qt4c->proFilePath() == n->path())
                    result << rc;
    return result;
}

// #pragma mark -- AndroidRunControlFactory

AndroidRunControlFactory::AndroidRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

AndroidRunControlFactory::~AndroidRunControlFactory()
{
}

bool AndroidRunControlFactory::canRun(RunConfiguration *runConfiguration,
                ProjectExplorer::RunMode mode) const
{
    if (mode != NormalRunMode && mode != DebugRunMode)
        return false;
    return qobject_cast<AndroidRunConfiguration *>(runConfiguration);
}

RunControl *AndroidRunControlFactory::create(RunConfiguration *runConfig,
                                        ProjectExplorer::RunMode mode)
{
    Q_ASSERT(canRun(runConfig, mode));
    AndroidRunConfiguration *rc = qobject_cast<AndroidRunConfiguration *>(runConfig);
    Q_ASSERT(rc);
    if (mode == NormalRunMode)
        return new AndroidRunControl(rc);
    else
        return AndroidDebugSupport::createDebugRunControl(rc);
}

QString AndroidRunControlFactory::displayName() const
{
    return tr("Run on Android device/emulator");
}

} // namespace Internal
} // namespace Qt4ProjectManager
