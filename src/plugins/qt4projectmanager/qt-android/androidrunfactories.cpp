/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "androidrunfactories.h"

#include "androidconstants.h"
#include "androiddebugsupport.h"
#include "androidrunconfiguration.h"
#include "androidruncontrol.h"
#include "androidtoolchain.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <debugger/debuggerconstants.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

namespace Qt4ProjectManager {
namespace Internal {

using namespace ProjectExplorer;

namespace {

QString pathFromId(const QString &id)
{
    if (!id.startsWith(ANDROID_RC_ID_PREFIX))
        return QString();
    return id.mid(QString(ANDROID_RC_ID_PREFIX).size());
}

} // namespace

AndroidRunConfigurationFactory::AndroidRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
}

AndroidRunConfigurationFactory::~AndroidRunConfigurationFactory()
{
}

bool AndroidRunConfigurationFactory::canCreate(Target *parent,
    const QString &id) const
{
    Qt4Target *target = qobject_cast<Qt4Target *>(parent);
    if (!target
        || target->id() != QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID)) {
        return false;
    }
    return true;
}

bool AndroidRunConfigurationFactory::canRestore(Target *parent,
    const QVariantMap &map) const
{
    if (!qobject_cast<Qt4Target *>(parent))
        return false;
    return ProjectExplorer::idFromMap(map)
        .startsWith(QLatin1String(ANDROID_RC_ID));
}

bool AndroidRunConfigurationFactory::canClone(Target *parent,
    RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

QStringList AndroidRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    QStringList ids;
    if (Qt4Target *t = qobject_cast<Qt4Target *>(parent))
    {
        if (t->id() == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID))
        {
            QList<Qt4ProFileNode *> nodes = t->qt4Project()->leafProFiles();
            foreach(Qt4ProFileNode * node, nodes)
                if (node->projectType() == ApplicationTemplate || node->projectType() == LibraryTemplate)
                    ids<<node->targetInformation().target;
        }
    }
    return ids;
}

QString AndroidRunConfigurationFactory::displayNameForId(const QString &id) const
{
    return id;
}

RunConfiguration *AndroidRunConfigurationFactory::create(Target *parent,
    const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    Qt4Target *pqt4parent = static_cast<Qt4Target *>(parent);
    return new AndroidRunConfiguration(pqt4parent, pathFromId(id));

}

RunConfiguration *AndroidRunConfigurationFactory::restore(Target *parent,
    const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Qt4Target *target = static_cast<Qt4Target *>(parent);
    AndroidRunConfiguration *rc = new AndroidRunConfiguration(target, QString());
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

RunConfiguration *AndroidRunConfigurationFactory::clone(Target *parent,
    RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    AndroidRunConfiguration *old = static_cast<AndroidRunConfiguration *>(source);
    return new AndroidRunConfiguration(static_cast<Qt4Target *>(parent), old);
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
    const QString &mode) const
{
#warning FIXME Android
    return true;
}

RunControl* AndroidRunControlFactory::create(RunConfiguration *runConfig,
    const QString &mode)
{
    Q_ASSERT(mode == ProjectExplorer::Constants::RUNMODE
        || mode == Debugger::Constants::DEBUGMODE);
    Q_ASSERT(canRun(runConfig, mode));
    AndroidRunConfiguration *rc = qobject_cast<AndroidRunConfiguration *>(runConfig);
    Q_ASSERT(rc);
    if (mode == ProjectExplorer::Constants::RUNMODE)
        return new AndroidRunControl(rc);
    return AndroidDebugSupport::createDebugRunControl(rc);
}

QString AndroidRunControlFactory::displayName() const
{
    return tr("Run on device");
}

QWidget *AndroidRunControlFactory::createConfigurationWidget(RunConfiguration *config)
{
    Q_UNUSED(config)
    return 0;
}

    } // namespace Internal
} // namespace Qt4ProjectManager
