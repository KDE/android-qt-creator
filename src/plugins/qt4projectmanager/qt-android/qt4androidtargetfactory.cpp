/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "qt4androidtargetfactory.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "qt-android/androiddeploystep.h"
#include "androidglobal.h"
#include "qt-android/androidpackagecreationstep.h"
#include "qt-android/androidrunconfiguration.h"
#include "qt-android/qt4androidtarget.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <buildconfigurationinfo.h>

#include <QtGui/QApplication>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::idFromMap;

// -------------------------------------------------------------------------
// Qt4AndroidTargetFactory
// -------------------------------------------------------------------------
Qt4AndroidTargetFactory::Qt4AndroidTargetFactory(QObject *parent) :
    Qt4BaseTargetFactory(parent)
{
    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>)),
            this, SIGNAL(supportedTargetIdsChanged()));
}

Qt4AndroidTargetFactory::~Qt4AndroidTargetFactory()
{
}

bool Qt4AndroidTargetFactory::supportsTargetId(const QString &id) const
{
    return id == Constants::ANDROID_DEVICE_TARGET_ID;
}

QStringList Qt4AndroidTargetFactory::supportedTargetIds(ProjectExplorer::Project *parent) const
{
    QStringList targetIds;
    if (parent && !qobject_cast<Qt4Project *>(parent))
        return targetIds;
    if (QtVersionManager::instance()->supportsTargetId(QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID)))
        targetIds << QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID);
    return targetIds;
}

QString Qt4AndroidTargetFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID))
        return Qt4AndroidTarget::defaultDisplayName();
    return QString();
}

QIcon Qt4AndroidTargetFactory::iconForId(const QString &id) const
{
    Q_UNUSED(id)
    return QIcon(":/projectexplorer/images/AndroidDevice.png");
}

bool Qt4AndroidTargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return false;
    return supportsTargetId(id);
}

bool Qt4AndroidTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

Qt4BaseTarget *Qt4AndroidTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    const QString id = idFromMap(map);
    Qt4AndroidTarget *target = 0;
    Qt4Project *qt4project = static_cast<Qt4Project *>(parent);
    if (id == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID))
        target = new Qt4AndroidTarget(qt4project, QLatin1String("transient ID"));
    if (target && target->fromMap(map))
        return target;
    delete target;
    return 0;
}

QString Qt4AndroidTargetFactory::defaultShadowBuildDirectory(const QString &projectLocation, const QString &id)
{
    QString suffix;
    if (id == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID))
        suffix = QLatin1String("android");
    else
        return QString();

    // currently we can't have the build directory to be deeper than the source directory
    // since that is broken in qmake
    // Once qmake is fixed we can change that to have a top directory and
    // subdirectories per build. (Replacing "QChar('-')" with "QChar('/') )
    return projectLocation + QLatin1Char('-') + suffix;
}

QList<BuildConfigurationInfo> Qt4AndroidTargetFactory::availableBuildConfigurations(const QString &proFilePath)
{
    return QList<BuildConfigurationInfo>()
        << availableBuildConfigurations(proFilePath, QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID));
}

QList<BuildConfigurationInfo> Qt4AndroidTargetFactory::availableBuildConfigurations(const QString &proFilePath,
    const QString &id)
{
    QList<BuildConfigurationInfo> infos;
    QList<QtVersion *> knownVersions = QtVersionManager::instance()->versionsForTargetId(id);

    foreach (QtVersion *version, knownVersions) {
        bool buildAll = version->defaultBuildConfig() & QtVersion::BuildAll;
        QtVersion::QmakeBuildConfigs config = buildAll ? QtVersion::BuildAll : QtVersion::QmakeBuildConfig(0);
        QString dir = defaultShadowBuildDirectory(Qt4Project::defaultTopLevelBuildDirectory(proFilePath), id);
        infos.append(BuildConfigurationInfo(version, config, QString(), dir));
        infos.append(BuildConfigurationInfo(version, config | QtVersion::DebugBuild, QString(), dir));
    }
    return infos;
}

Qt4BaseTarget *Qt4AndroidTargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    QList<QtVersion *> knownVersions = QtVersionManager::instance()->versionsForTargetId(id);
    if (knownVersions.isEmpty())
        return 0;

    QtVersion *qtVersion = knownVersions.first();
    bool buildAll = qtVersion->isValid() && (qtVersion->defaultBuildConfig() & QtVersion::BuildAll);
    QtVersion::QmakeBuildConfigs config = buildAll ? QtVersion::BuildAll : QtVersion::QmakeBuildConfig(0);

    QList<BuildConfigurationInfo> infos;
    infos.append(BuildConfigurationInfo(qtVersion, config | QtVersion::DebugBuild, QString(), QString()));
    infos.append(BuildConfigurationInfo(qtVersion, config, QString(), QString()));

    return create(parent, id, infos);
}

Qt4BaseTarget *Qt4AndroidTargetFactory::create(ProjectExplorer::Project *parent,
    const QString &id, const QList<BuildConfigurationInfo> & infos)
{
    if (!canCreate(parent, id))
        return 0;

    Qt4AndroidTarget *target = 0;
    if (id == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID))
        target = new Qt4AndroidTarget(static_cast<Qt4Project *>(parent), id);
    Q_ASSERT(target);

    foreach (const BuildConfigurationInfo &info, infos) {
        QString displayName = info.version->displayName() + QLatin1Char(' ');
        displayName += (info.buildConfig & QtVersion::DebugBuild) ? tr("Debug") : tr("Release");
        target->addQt4BuildConfiguration(displayName,
                                    info.version,
                                    info.buildConfig,
                                    info.additionalArguments,
                                    info.directory);
    }

    target->addDeployConfiguration(target->deployConfigurationFactory()->create(target, ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID));
    target->createApplicationProFiles();
    if (target->runConfigurations().isEmpty())
        target->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(target));
    return target;
}

QList<BuildConfigurationInfo> Qt4AndroidTargetFactory::availableBuildConfigurations(const QString &id, const QString &proFilePath, const QtVersionNumber &minimumQtVersion)
{
    QList<BuildConfigurationInfo> infos;
    QList<QtVersion *> knownVersions = QtVersionManager::instance()->versionsForTargetId(id, minimumQtVersion);

    foreach (QtVersion *version, knownVersions) {
        if (!version->isValid())
            continue;
        QtVersion::QmakeBuildConfigs config = version->defaultBuildConfig();
        QString dir = defaultShadowBuildDirectory(Qt4Project::defaultTopLevelBuildDirectory(proFilePath), id);
        infos.append(BuildConfigurationInfo(version, config, QString(), dir));
        infos.append(BuildConfigurationInfo(version, config ^ QtVersion::DebugBuild, QString(), dir));
    }
    return infos;
}

bool Qt4AndroidTargetFactory::isMobileTarget(const QString &/*id*/)
{
    return true;
}
