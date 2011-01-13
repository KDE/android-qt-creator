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

#include "qt4target.h"

#include "makestep.h"
#include "profilereader.h"
#include "qmakestep.h"
#include "qt4deployconfiguration.h"
#include "qt4project.h"
#include "qt4runconfiguration.h"
#include "qt4projectmanagerconstants.h"
#include "qt-maemo/maemodeploystep.h"
#include "qt-maemo/maemopackagecreationstep.h"
#include "qt-maemo/maemorunconfiguration.h"
#include "qt-s60/s60deployconfiguration.h"
#include "qt-s60/s60devicerunconfiguration.h"
#include "qt-s60/s60emulatorrunconfiguration.h"
#include "qt-s60/s60createpackagestep.h"
#include "qt-s60/s60deploystep.h"
#include "qt4projectconfigwidget.h"
#include "qt-android/androiddeploystep.h"
#include "qt-android/androidpackagecreationstep.h"
#include "qt-android/androidrunconfiguration.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/toolchaintype.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <coreplugin/coreconstants.h>
#include <symbianutils/symbiandevicemanager.h>

#include <QtGui/QApplication>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {

QString displayNameForId(const QString &id) {
    if (id == QLatin1String(Qt4ProjectManager::Constants::DESKTOP_TARGET_ID))
        return QApplication::translate("Qt4ProjectManager::Internal::Qt4Target", "Desktop", "Qt4 Desktop target display name");
    if (id == QLatin1String(Qt4ProjectManager::Constants::S60_EMULATOR_TARGET_ID))
        return QApplication::translate("Qt4ProjectManager::Internal::Qt4Target", "Symbian Emulator", "Qt4 Symbian Emulator target display name");
    if (id == QLatin1String(Qt4ProjectManager::Constants::S60_DEVICE_TARGET_ID))
        return QApplication::translate("Qt4ProjectManager::Internal::Qt4Target", "Symbian Device", "Qt4 Symbian Device target display name");
    if (id == QLatin1String(Qt4ProjectManager::Constants::MAEMO_DEVICE_TARGET_ID))
        return QApplication::translate("Qt4ProjectManager::Internal::Qt4Target", "Maemo", "Qt4 Maemo target display name");
    if (id == QLatin1String(Qt4ProjectManager::Constants::QT_SIMULATOR_TARGET_ID))
        return QApplication::translate("Qt4ProjectManager::Internal::Qt4Target", "Qt Simulator", "Qt4 Simulator target display name");
    if (id == QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
        return QApplication::translate("Qt4ProjectManager::Internal::Qt4Target", "Android", "Qt4 Android target display name");
    return QString();
}

QIcon iconForId(const QString &id) {
    if (id == QLatin1String(Qt4ProjectManager::Constants::DESKTOP_TARGET_ID))
        return QIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));
    if (id == QLatin1String(Qt4ProjectManager::Constants::S60_EMULATOR_TARGET_ID))
        return QIcon(QLatin1String(":/projectexplorer/images/SymbianEmulator.png"));
    if (id == QLatin1String(Qt4ProjectManager::Constants::S60_DEVICE_TARGET_ID))
        return QIcon(QLatin1String(":/projectexplorer/images/SymbianDevice.png"));
    if (id == QLatin1String(Qt4ProjectManager::Constants::MAEMO_DEVICE_TARGET_ID))
        return QIcon(QLatin1String(":/projectexplorer/images/MaemoDevice.png"));
    if (id == QLatin1String(Qt4ProjectManager::Constants::QT_SIMULATOR_TARGET_ID))
        return QIcon(QLatin1String(":/projectexplorer/images/SymbianEmulator.png"));
    if (id == QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
        return QIcon(QLatin1String(":/projectexplorer/images/AndroidDevice.png"));
    return QIcon();
}

} // namespace

// -------------------------------------------------------------------------
// Qt4TargetFactory
// -------------------------------------------------------------------------

Qt4TargetFactory::Qt4TargetFactory(QObject *parent) :
    ITargetFactory(parent)
{
    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>)),
            this, SIGNAL(availableCreationIdsChanged()));
}

Qt4TargetFactory::~Qt4TargetFactory()
{
}

bool Qt4TargetFactory::supportsTargetId(const QString &id) const
{
    QSet<QString> ids;
    ids << QLatin1String("Qt4ProjectManager.Target.DesktopTarget")
        << QLatin1String("Qt4ProjectManager.Target.S60EmulatorTarget")
        << QLatin1String("Qt4ProjectManager.Target.S60DeviceTarget")
        << QLatin1String("Qt4ProjectManager.Target.MaemoDeviceTarget")
        << QLatin1String("Qt4ProjectManager.Target.QtSimulatorTarget");
    return ids.contains(id);
}

QStringList Qt4TargetFactory::availableCreationIds(ProjectExplorer::Project *parent) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return QStringList();

    QSet<QString> ids;
    ids << QLatin1String("Qt4ProjectManager.Target.DesktopTarget")
        << QLatin1String("Qt4ProjectManager.Target.S60EmulatorTarget")
        << QLatin1String("Qt4ProjectManager.Target.S60DeviceTarget")
        << QLatin1String("Qt4ProjectManager.Target.MaemoDeviceTarget")
        << QLatin1String("Qt4ProjectManager.Target.QtSimulatorTarget");


    return parent->possibleTargetIds().intersect(ids).toList();
}

QString Qt4TargetFactory::displayNameForId(const QString &id) const
{
    return ::displayNameForId(id);
}

bool Qt4TargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return false;

    return parent->canAddTarget(id);
}

Qt4Target *Qt4TargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    QList<QtVersion *> knownVersions = QtVersionManager::instance()->versionsForTargetId(id);
    if (knownVersions.count() > 1)
        knownVersions = knownVersions.mid(0, 1);
    return create(parent, id, knownVersions);
}

Qt4Target *Qt4TargetFactory::create(ProjectExplorer::Project *parent, const QString &id, QList<QtVersion *> versions)
{
    QList<BuildConfigurationInfo> infos;
    foreach (QtVersion *version, versions) {
        bool buildAll = false;
        if (version && version->isValid() && (version->defaultBuildConfig() & QtVersion::BuildAll))
            buildAll = true;

        if (buildAll) {
            infos.append(BuildConfigurationInfo(version, QtVersion::BuildAll | QtVersion::DebugBuild));
            infos.append(BuildConfigurationInfo(version, QtVersion::BuildAll));
        } else {
            infos.append(BuildConfigurationInfo(version, QtVersion::DebugBuild));
            infos.append(BuildConfigurationInfo(version, QtVersion::QmakeBuildConfig(0)));
        }
    }

    return create(parent, id, infos);
}

Qt4Target *Qt4TargetFactory::create(ProjectExplorer::Project *parent, const QString &id, QList<BuildConfigurationInfo> infos)
{
    if (!canCreate(parent, id))
        return 0;

    Qt4Project *qt4project = static_cast<Qt4Project *>(parent);
    Qt4Target *t = new Qt4Target(qt4project, id);

    QList<QtVersion *> knownVersions(QtVersionManager::instance()->versionsForTargetId(id));
    if (knownVersions.isEmpty())
        return t;

    // count Qt versions:
    int qtVersionCount = 0;
    {
        QSet<QtVersion *> differentVersions;
        foreach (const BuildConfigurationInfo &info, infos) {
            if (knownVersions.contains(info.version))
                differentVersions.insert(info.version);
        }
        qtVersionCount = differentVersions.count();
    }

    // Create Buildconfigurations:
    foreach (const BuildConfigurationInfo &info, infos) {
        if (!info.version || !knownVersions.contains(info.version))
            continue;

        QString displayName;

        if (qtVersionCount > 1)
            displayName = info.version->displayName() + QChar(' ');
        displayName.append((info.buildConfig & QtVersion::DebugBuild) ? tr("Debug") : tr("Release"));

        // Skip release builds for the symbian emulator.
        if (id == QLatin1String(Constants::S60_EMULATOR_TARGET_ID) &&
            !(info.buildConfig & QtVersion::DebugBuild))
            continue;

        t->addQt4BuildConfiguration(displayName, info.version, info.buildConfig, info.additionalArguments, info.directory);
    }

    t->addDeployConfiguration(t->deployConfigurationFactory()->create(t, ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID));

    // create RunConfigurations:
    QStringList pathes = qt4project->applicationProFilePathes();
    foreach (const QString &path, pathes)
        t->addRunConfigurationForPath(path);

    if (t->runConfigurations().isEmpty())
        t->addRunConfiguration(new CustomExecutableRunConfiguration(t));

    return t;
}

bool Qt4TargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

Qt4Target *Qt4TargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Qt4Project *qt4project(static_cast<Qt4Project *>(parent));
    Qt4Target *t(new Qt4Target(qt4project, QLatin1String("transient ID")));
    if (t->fromMap(map))
        return t;
    delete t;
    return 0;
}

// -------------------------------------------------------------------------
// Qt4Target
// -------------------------------------------------------------------------

Qt4Target::Qt4Target(Qt4Project *parent, const QString &id) :
    ProjectExplorer::Target(parent, id),
    m_connectedPixmap(QLatin1String(":/projectexplorer/images/ConnectionOn.png")),
    m_disconnectedPixmap(QLatin1String(":/projectexplorer/images/ConnectionOff.png")),
    m_buildConfigurationFactory(new Qt4BuildConfigurationFactory(this)),
    m_deployConfigurationFactory(new Qt4DeployConfigurationFactory(this))
{
    connect(project(), SIGNAL(supportedTargetIdsChanged()),
            this, SLOT(updateQtVersion()));

    connect(this, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(emitProFileEvaluateNeeded()));
    connect(this, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SIGNAL(environmentChanged()));
    connect(this, SIGNAL(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(onAddedBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
    connect(this, SIGNAL(addedDeployConfiguration(ProjectExplorer::DeployConfiguration*)),
            this, SLOT(onAddedDeployConfiguration(ProjectExplorer::DeployConfiguration*)));
    connect(this, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
            this, SLOT(updateToolTipAndIcon()));

    setDefaultDisplayName(displayNameForId(id));
    setIcon(iconForId(id));
}

Qt4Target::~Qt4Target()
{
}

ProjectExplorer::BuildConfigWidget *Qt4Target::createConfigWidget()
{
    return new Qt4ProjectConfigWidget(this);
}

Qt4BuildConfiguration *Qt4Target::activeBuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(Target::activeBuildConfiguration());
}

Qt4Project *Qt4Target::qt4Project() const
{
    return static_cast<Qt4Project *>(project());
}

Qt4BuildConfiguration *Qt4Target::addQt4BuildConfiguration(QString displayName, QtVersion *qtversion,
                                                           QtVersion::QmakeBuildConfigs qmakeBuildConfiguration,
                                                           QString additionalArguments,
                                                           QString directory)
{
    Q_ASSERT(qtversion);
    bool debug = qmakeBuildConfiguration & QtVersion::DebugBuild;

    // Add the buildconfiguration
    Qt4BuildConfiguration *bc = new Qt4BuildConfiguration(this);
    bc->setDefaultDisplayName(displayName);

    BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    Q_ASSERT(buildSteps);
    Q_ASSERT(cleanSteps);

    QMakeStep *qmakeStep = new QMakeStep(buildSteps);
    buildSteps->insertStep(0, qmakeStep);

    MakeStep *makeStep = new MakeStep(buildSteps);
    buildSteps->insertStep(1, makeStep);

    MakeStep* cleanStep = new MakeStep(cleanSteps);
    cleanStep->setClean(true);
    cleanStep->setUserArguments("clean");
    cleanSteps->insertStep(0, cleanStep);
    if (!additionalArguments.isEmpty())
        qmakeStep->setUserArguments(additionalArguments);

    // set some options for qmake and make
    if (qmakeBuildConfiguration & QtVersion::BuildAll) // debug_and_release => explicit targets
        makeStep->setUserArguments(debug ? "debug" : "release");

    bc->setQMakeBuildConfiguration(qmakeBuildConfiguration);

    // Finally set the qt version & ToolChain
    bc->setQtVersion(qtversion);
    ProjectExplorer::ToolChainType defaultTc = preferredToolChainType(filterToolChainTypes(bc->qtVersion()->possibleToolChainTypes()));
    bc->setToolChainType(defaultTc);
    if (!directory.isEmpty())
        bc->setShadowBuildAndDirectory(directory != project()->projectDirectory(), directory);
    addBuildConfiguration(bc);

    return bc;
}

Qt4BuildConfigurationFactory *Qt4Target::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

ProjectExplorer::DeployConfigurationFactory *Qt4Target::deployConfigurationFactory() const
{
    return m_deployConfigurationFactory;
}

void Qt4Target::addRunConfigurationForPath(const QString &proFilePath)
{
    if (id() == QLatin1String(Constants::DESKTOP_TARGET_ID) ||
        id() == QLatin1String(Constants::QT_SIMULATOR_TARGET_ID))
        addRunConfiguration(new Qt4RunConfiguration(this, proFilePath));
    else if (id() == QLatin1String(Constants::S60_EMULATOR_TARGET_ID))
        addRunConfiguration(new S60EmulatorRunConfiguration(this, proFilePath));
    else if (id() == QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        addRunConfiguration(new S60DeviceRunConfiguration(this, proFilePath));
    else if (id() == QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID))
        addRunConfiguration(new MaemoRunConfiguration(this, proFilePath));
    else if (id() == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID))
        addRunConfiguration(new AndroidRunConfiguration(this, proFilePath));
}

QList<ProjectExplorer::ToolChainType> Qt4Target::filterToolChainTypes(const QList<ProjectExplorer::ToolChainType> &candidates) const
{
    QList<ProjectExplorer::ToolChainType> tmp(candidates);
    if (id() == QLatin1String(Constants::S60_EMULATOR_TARGET_ID)) {
        if (tmp.contains(ProjectExplorer::ToolChain_WINSCW))
            return QList<ProjectExplorer::ToolChainType>() << ProjectExplorer::ToolChain_WINSCW;
        else
            return QList<ProjectExplorer::ToolChainType>();
    } else if (id() == QLatin1String(Constants::S60_DEVICE_TARGET_ID)) {
        tmp.removeAll(ProjectExplorer::ToolChain_WINSCW);
        return tmp;
    }
    return tmp;
}

ProjectExplorer::ToolChainType Qt4Target::preferredToolChainType(const QList<ProjectExplorer::ToolChainType> &candidates) const
{
    ProjectExplorer::ToolChainType preferredType = ProjectExplorer::ToolChain_INVALID;
    if (id() == QLatin1String(Constants::S60_EMULATOR_TARGET_ID) &&
        candidates.contains(ProjectExplorer::ToolChain_WINSCW))
        preferredType = ProjectExplorer::ToolChain_WINSCW;
    if (!candidates.isEmpty())
        preferredType = candidates.at(0);
    return preferredType;
}

QString Qt4Target::defaultBuildDirectory() const
{
    if (id() == QLatin1String(Constants::S60_DEVICE_TARGET_ID)
        || id() == QLatin1String(Constants::S60_EMULATOR_TARGET_ID)
#if defined(Q_OS_WIN)
        || id() == QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID)
#endif
        )
        return project()->projectDirectory();

    return defaultShadowBuildDirectory(qt4Project()->defaultTopLevelBuildDirectory(), id());
}

QString Qt4Target::defaultShadowBuildDirectory(const QString &projectLocation, const QString &id)
{
    QString shortName = QLatin1String("unknown");
    if (id == QLatin1String(Constants::DESKTOP_TARGET_ID))
        shortName = QLatin1String("desktop");
    else if (id == QLatin1String(Constants::S60_EMULATOR_TARGET_ID))
        shortName = QLatin1String("symbian_emulator");
    else if (id == QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        shortName = QLatin1String("symbian");
    else if (id == QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID))
        shortName = QLatin1String("maemo");
    else if (id == QLatin1String(Constants::QT_SIMULATOR_TARGET_ID))
        shortName = QLatin1String("simulator");
    else if (id == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID))
        shortName = QLatin1String("android");

    // currently we can't have the build directory to be deeper then the source directory
    // since that is broken in qmake
    // Once qmake is fixed we can change that to have a top directory and
    // subdirectories per build. (Replacing "QChar('-')" with "QChar('/') )
    return projectLocation + QChar('-') + shortName;
}

bool Qt4Target::fromMap(const QVariantMap &map)
{
    bool success = Target::fromMap(map);
    setIcon(iconForId(id()));
    setDefaultDisplayName(displayNameForId(id()));
    return success;
}

void Qt4Target::updateQtVersion()
{
    setEnabled(project()->supportedTargetIds().contains(id()));
}

void Qt4Target::onAddedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc)
{
    Q_ASSERT(bc);
    Qt4BuildConfiguration *qt4bc = qobject_cast<Qt4BuildConfiguration *>(bc);
    Q_ASSERT(qt4bc);
    connect(qt4bc, SIGNAL(buildDirectoryInitialized()),
            this, SIGNAL(buildDirectoryInitialized()));
    connect(qt4bc, SIGNAL(proFileEvaluateNeeded(Qt4ProjectManager::Qt4BuildConfiguration *)),
            this, SLOT(onProFileEvaluateNeeded(Qt4ProjectManager::Qt4BuildConfiguration *)));
}

void Qt4Target::onAddedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc)
{
    Q_ASSERT(dc);
    S60DeployConfiguration *deployConf(qobject_cast<S60DeployConfiguration *>(dc));
    if (!deployConf)
        return;
    connect(deployConf, SIGNAL(serialPortNameChanged()),
            this, SLOT(slotUpdateDeviceInformation()));
}

void Qt4Target::slotUpdateDeviceInformation()
{
    S60DeployConfiguration *dc(qobject_cast<S60DeployConfiguration *>(sender()));
    if (dc && dc == activeDeployConfiguration()) {
        updateToolTipAndIcon();
    }
}

void Qt4Target::onProFileEvaluateNeeded(Qt4ProjectManager::Qt4BuildConfiguration *bc)
{
    if (bc && bc == activeBuildConfiguration())
        emit proFileEvaluateNeeded(this);
}

void Qt4Target::emitProFileEvaluateNeeded()
{
    emit proFileEvaluateNeeded(this);
}

void Qt4Target::updateToolTipAndIcon()
{
    static const int TARGET_OVERLAY_ORIGINAL_SIZE = 32;
    if (const S60DeployConfiguration *s60DeployConf = qobject_cast<S60DeployConfiguration *>(activeDeployConfiguration()))  {
        const SymbianUtils::SymbianDeviceManager *sdm = SymbianUtils::SymbianDeviceManager::instance();
        const int deviceIndex = sdm->findByPortName(s60DeployConf->serialPortName());
        QPixmap overlay;
        if (deviceIndex == -1) {
            setToolTip(tr("<b>Device:</b> Not connected"));
            overlay = m_disconnectedPixmap;
        } else {
            // device connected
            const SymbianUtils::SymbianDevice device = sdm->devices().at(deviceIndex);
            const QString tooltip = device.additionalInformation().isEmpty() ?
                                    tr("<b>Device:</b> %1").arg(device.friendlyName()) :
                                    tr("<b>Device:</b> %1, %2").arg(device.friendlyName(), device.additionalInformation());
            setToolTip(tooltip);
            overlay = m_connectedPixmap;
        }
        double factor = Core::Constants::TARGET_ICON_SIZE / (double)TARGET_OVERLAY_ORIGINAL_SIZE;
        QSize overlaySize(overlay.size().width()*factor, overlay.size().height()*factor);
        QPixmap pixmap(Core::Constants::TARGET_ICON_SIZE, Core::Constants::TARGET_ICON_SIZE);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.drawPixmap(Core::Constants::TARGET_ICON_SIZE - overlaySize.width(),
                           Core::Constants::TARGET_ICON_SIZE - overlaySize.height(),
                           overlay.scaled(overlaySize));

        setOverlayIcon(QIcon(pixmap));
    } else {
        setToolTip(QString());
        setOverlayIcon(QIcon());
    }
}
