/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cmakebuildconfiguration.h"

#include "cmakeopenprojectwizard.h"
#include "cmakeproject.h"
#include "cmaketarget.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildsteplist.h>
#include <utils/qtcassert.h>

#include <QtGui/QInputDialog>

using namespace CMakeProjectManager;
using namespace Internal;

namespace {
const char * const CMAKE_BC_ID("CMakeProjectManager.CMakeBuildConfiguration");

const char * const MSVC_VERSION_KEY("CMakeProjectManager.CMakeBuildConfiguration.MsvcVersion");
const char * const BUILD_DIRECTORY_KEY("CMakeProjectManager.CMakeBuildConfiguration.BuildDirectory");
} // namespace

CMakeBuildConfiguration::CMakeBuildConfiguration(CMakeTarget *parent) :
    BuildConfiguration(parent, QLatin1String(CMAKE_BC_ID)),
    m_toolChain(0)
{
    m_buildDirectory = cmakeTarget()->defaultBuildDirectory();
}

CMakeBuildConfiguration::CMakeBuildConfiguration(CMakeTarget *parent, CMakeBuildConfiguration *source) :
    BuildConfiguration(parent, source),
    m_toolChain(0),
    m_buildDirectory(source->m_buildDirectory),
    m_msvcVersion(source->m_msvcVersion)
{
    Q_ASSERT(parent);
    cloneSteps(source);
}

QVariantMap CMakeBuildConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::BuildConfiguration::toMap());
    map.insert(QLatin1String(MSVC_VERSION_KEY), m_msvcVersion);
    map.insert(QLatin1String(BUILD_DIRECTORY_KEY), m_buildDirectory);
    return map;
}

bool CMakeBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    m_msvcVersion = map.value(QLatin1String(MSVC_VERSION_KEY)).toString();
    m_buildDirectory = map.value(QLatin1String(BUILD_DIRECTORY_KEY), cmakeTarget()->defaultBuildDirectory()).toString();

    return true;
}

CMakeBuildConfiguration::~CMakeBuildConfiguration()
{
    delete m_toolChain;
}

CMakeTarget *CMakeBuildConfiguration::cmakeTarget() const
{
    return static_cast<CMakeTarget *>(target());
}

QString CMakeBuildConfiguration::buildDirectory() const
{
    return m_buildDirectory;
}

ProjectExplorer::ToolChainType CMakeBuildConfiguration::toolChainType() const
{
    if (m_toolChain)
        return m_toolChain->type();
    return ProjectExplorer::ToolChain_UNKNOWN;
}

ProjectExplorer::ToolChain *CMakeBuildConfiguration::toolChain() const
{
    updateToolChain();
    return m_toolChain;
}

void CMakeBuildConfiguration::updateToolChain() const
{
    ProjectExplorer::ToolChain *newToolChain = 0;
    if (msvcVersion().isEmpty()) {
#ifdef Q_OS_WIN
        newToolChain = ProjectExplorer::ToolChain::createMinGWToolChain("gcc", QString());
#else
        newToolChain = ProjectExplorer::ToolChain::createGccToolChain("gcc");
#endif
    } else { // msvc
        newToolChain = ProjectExplorer::ToolChain::createMSVCToolChain(m_msvcVersion, false);
    }

    if (ProjectExplorer::ToolChain::equals(newToolChain, m_toolChain)) {
        delete newToolChain;
        newToolChain = 0;
    } else {
        delete m_toolChain;
        m_toolChain = newToolChain;
    }
}

void CMakeBuildConfiguration::setBuildDirectory(const QString &buildDirectory)
{
    if (m_buildDirectory == buildDirectory)
        return;
    m_buildDirectory = buildDirectory;
    emit buildDirectoryChanged();
    emit environmentChanged();
}

QString CMakeBuildConfiguration::msvcVersion() const
{
    return m_msvcVersion;
}

void CMakeBuildConfiguration::setMsvcVersion(const QString &msvcVersion)
{
    if (m_msvcVersion == msvcVersion)
        return;
    m_msvcVersion = msvcVersion;
    updateToolChain();

    emit msvcVersionChanged();
}

ProjectExplorer::IOutputParser *CMakeBuildConfiguration::createOutputParser() const
{
    if (m_toolChain)
        return m_toolChain->outputParser();
    return 0;
}

Utils::Environment CMakeBuildConfiguration::baseEnvironment() const
{
    Utils::Environment env = BuildConfiguration::baseEnvironment();
    if (m_toolChain)
        m_toolChain->addToEnvironment(env);
    return env;
}

/*!
  \class CMakeBuildConfigurationFactory
*/

CMakeBuildConfigurationFactory::CMakeBuildConfigurationFactory(QObject *parent) :
    ProjectExplorer::IBuildConfigurationFactory(parent)
{
}

CMakeBuildConfigurationFactory::~CMakeBuildConfigurationFactory()
{
}

QStringList CMakeBuildConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    if (!qobject_cast<CMakeTarget *>(parent))
        return QStringList();
    return QStringList() << QLatin1String(CMAKE_BC_ID);
}

QString CMakeBuildConfigurationFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(CMAKE_BC_ID))
        return tr("Build");
    return QString();
}

bool CMakeBuildConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const QString &id) const
{
    if (!qobject_cast<CMakeTarget *>(parent))
        return false;
    if (id == QLatin1String(CMAKE_BC_ID))
        return true;
    return false;
}

CMakeBuildConfiguration *CMakeBuildConfigurationFactory::create(ProjectExplorer::Target *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    CMakeTarget *cmtarget = static_cast<CMakeTarget *>(parent);
    Q_ASSERT(cmtarget);

    //TODO configuration name should be part of the cmakeopenprojectwizard
    bool ok;
    QString buildConfigurationName = QInputDialog::getText(0,
                          tr("New Configuration"),
                          tr("New configuration name:"),
                          QLineEdit::Normal,
                          QString(),
                          &ok);
    if (!ok || buildConfigurationName.isEmpty())
        return 0;
    CMakeBuildConfiguration *bc = new CMakeBuildConfiguration(cmtarget);
    bc->setDisplayName(buildConfigurationName);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

    MakeStep *makeStep = new MakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);

    MakeStep *cleanMakeStep = new MakeStep(cleanSteps);
    cleanSteps->insertStep(0, cleanMakeStep);
    cleanMakeStep->setAdditionalArguments("clean");
    cleanMakeStep->setClean(true);

    CMakeOpenProjectWizard copw(cmtarget->cmakeProject()->projectManager(),
                                cmtarget->project()->projectDirectory(),
                                bc->buildDirectory(),
                                bc->environment());
    if (copw.exec() != QDialog::Accepted) {
        delete bc;
        return 0;
    }
    cmtarget->addBuildConfiguration(bc); // this also makes the name unique

    bc->setBuildDirectory(copw.buildDirectory());
    bc->setMsvcVersion(copw.msvcVersion());
    cmtarget->cmakeProject()->parseCMakeLists();

    // Default to all
    if (cmtarget->cmakeProject()->hasBuildTarget("all"))
        makeStep->setBuildTarget("all", true);

    return bc;
}

bool CMakeBuildConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const
{
    return canCreate(parent, source->id());
}

CMakeBuildConfiguration *CMakeBuildConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    CMakeBuildConfiguration *old = static_cast<CMakeBuildConfiguration *>(source);
    CMakeTarget *cmtarget(static_cast<CMakeTarget *>(parent));
    return new CMakeBuildConfiguration(cmtarget, old);
}

bool CMakeBuildConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
}

CMakeBuildConfiguration *CMakeBuildConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    CMakeTarget *cmtarget(static_cast<CMakeTarget *>(parent));
    CMakeBuildConfiguration *bc = new CMakeBuildConfiguration(cmtarget);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}
