/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidrunfactories.h"

#include "androidconstants.h"
#include "androiddebugsupport.h"
#include "androidrunconfiguration.h"
#include "androidruncontrol.h"
#include "androidtoolchain.h"
#include "qt4androidtarget.h"

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
    const QString &/*id*/) const
{
    Qt4AndroidTarget *target = qobject_cast<Qt4AndroidTarget *>(parent);
    if (!target
        || target->id() != QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID)) {
        return false;
    }
    return true;
}

bool AndroidRunConfigurationFactory::canRestore(Target *parent,
    const QVariantMap &map) const
{
    if (!qobject_cast<Qt4AndroidTarget *>(parent))
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
    if (Qt4AndroidTarget *t = qobject_cast<Qt4AndroidTarget *>(parent))
    {
        if (t->id() == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID))
        {
            QList<Qt4ProFileNode *> nodes = t->qt4Project()->allProFiles();
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
    Qt4AndroidTarget *pqt4parent = static_cast<Qt4AndroidTarget *>(parent);
    return new AndroidRunConfiguration(pqt4parent, pathFromId(id));

}

RunConfiguration *AndroidRunConfigurationFactory::restore(Target *parent,
    const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Qt4AndroidTarget *target = static_cast<Qt4AndroidTarget *>(parent);
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
    return new AndroidRunConfiguration(static_cast<Qt4AndroidTarget *>(parent), old);
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
    const QString &/*mode*/) const
{
    return qobject_cast<AndroidRunConfiguration *>(runConfiguration);
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
