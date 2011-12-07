/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidtargetfactory.h"
#include "qt4projectmanager/qt4project.h"
#include "qt4projectmanager/qt4projectmanagerconstants.h"
#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidpackagecreationstep.h"
#include "androidrunconfiguration.h"
#include "androidtarget.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <qt4projectmanager/buildconfigurationinfo.h>

#include <qtsupport/qtversionmanager.h>

#include <QtGui/QApplication>

using namespace Qt4ProjectManager;
using namespace Android::Internal;
using ProjectExplorer::idFromMap;

// -------------------------------------------------------------------------
// Qt4AndroidTargetFactory
// -------------------------------------------------------------------------
AndroidTargetFactory::AndroidTargetFactory(QObject *parent) :
    Qt4BaseTargetFactory(parent)
{
    connect(QtSupport::QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>)),
            this, SIGNAL(supportedTargetIdsChanged()));
}

AndroidTargetFactory::~AndroidTargetFactory()
{
}

bool AndroidTargetFactory::supportsTargetId(const QString &id) const
{
    return id == Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID;
}

QSet<QString> AndroidTargetFactory::targetFeatures(const QString &/*id*/) const
{
    QSet<QString> features;
    features << Qt4ProjectManager::Constants::MOBILE_TARGETFEATURE_ID;
    features << Qt4ProjectManager::Constants::SHADOWBUILD_TARGETFEATURE_ID;
    // how to check check whether they component set is really installed?
    features << Qt4ProjectManager::Constants::QTQUICKCOMPONENTS_MEEGO_TARGETFEATURE_ID;
    return features;
}

QStringList AndroidTargetFactory::supportedTargetIds(ProjectExplorer::Project *parent) const
{
    QStringList targetIds;
    if (parent && !qobject_cast<Qt4Project *>(parent))
        return targetIds;
    if (QtSupport::QtVersionManager::instance()->supportsTargetId(QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID)))
        targetIds << QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID);
    return targetIds;
}

QString AndroidTargetFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
        return AndroidTarget::defaultDisplayName();
    return QString();
}

QIcon AndroidTargetFactory::iconForId(const QString &id) const
{
    Q_UNUSED(id)
    return QIcon(Constants::ANDROID_SETTINGS_CATEGORY_ICON);
}

bool AndroidTargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return false;
    return supportsTargetId(id);
}

bool AndroidTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

Qt4BaseTarget *AndroidTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    const QString id = idFromMap(map);
    AndroidTarget *target = 0;
    Qt4Project *qt4project = static_cast<Qt4Project *>(parent);
    if (id == QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
        target = new AndroidTarget(qt4project, QLatin1String("transient ID"));
    if (target && target->fromMap(map))
        return target;
    delete target;
    return 0;
}

Qt4BaseTarget *AndroidTargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    QList<QtSupport::BaseQtVersion *> knownVersions = QtSupport::QtVersionManager::instance()->versionsForTargetId(id);
    if (knownVersions.isEmpty())
        return 0;

    QtSupport::BaseQtVersion *qtVersion = knownVersions.first();
    bool buildAll = qtVersion->isValid() && (qtVersion->defaultBuildConfig() & QtSupport::BaseQtVersion::BuildAll);
    QtSupport::BaseQtVersion::QmakeBuildConfigs config = buildAll ? QtSupport::BaseQtVersion::BuildAll : QtSupport::BaseQtVersion::QmakeBuildConfig(0);

    QList<BuildConfigurationInfo> infos;
    infos.append(BuildConfigurationInfo(qtVersion, config | QtSupport::BaseQtVersion::DebugBuild, QString(), QString()));
    infos.append(BuildConfigurationInfo(qtVersion, config, QString(), QString()));

    return create(parent, id, infos);
}

Qt4BaseTarget *AndroidTargetFactory::create(ProjectExplorer::Project *parent,
    const QString &id, const QList<BuildConfigurationInfo> & infos)
{
    if (!canCreate(parent, id))
        return 0;

    AndroidTarget *target = 0;
    if (id == QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
        target = new AndroidTarget(static_cast<Qt4Project *>(parent), id);
    Q_ASSERT(target);

    foreach (const BuildConfigurationInfo &info, infos) {
        QString displayName = info.version->displayName() + QLatin1Char(' ');
        displayName += (info.buildConfig & QtSupport::BaseQtVersion::DebugBuild) ? tr("Debug") : tr("Release");
        target->addQt4BuildConfiguration(displayName,QString(),
                                    info.version,
                                    info.buildConfig,
                                    info.additionalArguments,
                                    info.directory,
                                    info.importing);
    }

    target->addDeployConfiguration(target->createDeployConfiguration(ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID));

    target->createApplicationProFiles();
    if (target->runConfigurations().isEmpty())
        target->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(target));
    return target;
}

QString AndroidTargetFactory::buildNameForId(const QString &id) const
{
    if (id == QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
        return QLatin1String("android");
    return QString();
}
