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

#include "genericproject.h"

#include "genericbuildconfiguration.h"
#include "genericprojectconstants.h"
#include "generictarget.h"

#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <cplusplus/ModelManagerInterface.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/pathchooser.h>
#include <coreplugin/icore.h>

#include <QtCore/QDir>
#include <QtCore/QProcessEnvironment>

#include <QtGui/QFormLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QComboBox>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;
using namespace ProjectExplorer;

namespace {
const char * const TOOLCHAIN_KEY("GenericProjectManager.GenericProject.Toolchain");
} // end of anonymous namespace

////////////////////////////////////////////////////////////////////////////////////
// GenericProject
////////////////////////////////////////////////////////////////////////////////////

GenericProject::GenericProject(Manager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName),
      m_toolChain(0)
{
    QFileInfo fileInfo(m_fileName);
    QDir dir = fileInfo.dir();

    m_projectName      = fileInfo.completeBaseName();
    m_filesFileName    = QFileInfo(dir, m_projectName + QLatin1String(".files")).absoluteFilePath();
    m_includesFileName = QFileInfo(dir, m_projectName + QLatin1String(".includes")).absoluteFilePath();
    m_configFileName   = QFileInfo(dir, m_projectName + QLatin1String(".config")).absoluteFilePath();

    m_file = new GenericProjectFile(this, fileName);
    m_rootNode = new GenericProjectNode(this, m_file);

    m_manager->registerProject(this);
}

GenericProject::~GenericProject()
{
    m_codeModelFuture.cancel();
    m_manager->unregisterProject(this);

    delete m_rootNode;
    delete m_toolChain;
}

GenericTarget *GenericProject::activeTarget() const
{
    return static_cast<GenericTarget *>(Project::activeTarget());
}

QString GenericProject::filesFileName() const
{ return m_filesFileName; }

QString GenericProject::includesFileName() const
{ return m_includesFileName; }

QString GenericProject::configFileName() const
{ return m_configFileName; }

static QStringList readLines(const QString &absoluteFileName)
{
    QStringList lines;

    QFile file(absoluteFileName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);

        forever {
            QString line = stream.readLine();
            if (line.isNull())
                break;

            lines.append(line);
        }
    }

    return lines;
}

bool GenericProject::saveRawFileList(const QStringList &rawFileList)
{
    // Make sure we can open the file for writing
    QFile file(filesFileName());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream stream(&file);
    foreach (const QString &filePath, rawFileList)
        stream << filePath << QLatin1Char('\n');

    file.close();
    refresh(GenericProject::Files);
    return true;
}

bool GenericProject::addFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    QDir baseDir(QFileInfo(m_fileName).dir());
    foreach (const QString &filePath, filePaths)
        newList.append(baseDir.relativeFilePath(filePath));

    return saveRawFileList(newList);
}

bool GenericProject::removeFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    foreach (const QString &filePath, filePaths) {
        QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
        if (i != m_rawListEntries.end())
            newList.removeOne(i.value());
    }

    return saveRawFileList(newList);
}

void GenericProject::parseProject(RefreshOptions options)
{
    if (options & Files) {
        m_rawListEntries.clear();
        m_rawFileList = readLines(filesFileName());
        m_files = processEntries(m_rawFileList, &m_rawListEntries);
    }

    if (options & Configuration) {
        m_projectIncludePaths = processEntries(readLines(includesFileName()));

        // TODO: Possibly load some configuration from the project file
        //QSettings projectInfo(m_fileName, QSettings::IniFormat);

        m_defines.clear();

        QFile configFile(configFileName());
        if (configFile.open(QFile::ReadOnly))
            m_defines = configFile.readAll();
    }

    if (options & Files)
        emit fileListChanged();
}

void GenericProject::refresh(RefreshOptions options)
{
    QSet<QString> oldFileList;
    if (!(options & Configuration))
        oldFileList = m_files.toSet();

    parseProject(options);

    if (options & Files)
        m_rootNode->refresh();

    CPlusPlus::CppModelManagerInterface *modelManager =
        CPlusPlus::CppModelManagerInterface::instance();

    if (m_toolChain && modelManager) {
        const QByteArray predefinedMacros = m_toolChain->predefinedMacros();

        CPlusPlus::CppModelManagerInterface::ProjectInfo pinfo = modelManager->projectInfo(this);
        pinfo.defines = predefinedMacros;
        pinfo.defines += '\n';
        pinfo.defines += m_defines;

        QStringList allIncludePaths;
        QStringList allFrameworkPaths;

        foreach (const ProjectExplorer::HeaderPath &headerPath, m_toolChain->systemHeaderPaths()) {
            if (headerPath.kind() == ProjectExplorer::HeaderPath::FrameworkHeaderPath)
                allFrameworkPaths.append(headerPath.path());
            else
                allIncludePaths.append(headerPath.path());
        }

        allIncludePaths += this->allIncludePaths();

        pinfo.frameworkPaths = allFrameworkPaths;
        pinfo.includePaths = allIncludePaths;

        // ### add _defines.
        pinfo.sourceFiles = files();
        pinfo.sourceFiles += generated();

        QStringList filesToUpdate;

        if (options & Configuration) {
            filesToUpdate = pinfo.sourceFiles;
            filesToUpdate.append(QLatin1String("<configuration>")); // XXX don't hardcode configuration file name
            // Full update, if there's a code model update, cancel it
            m_codeModelFuture.cancel();
        } else if (options & Files) {
            // Only update files that got added to the list
            QSet<QString> newFileList = m_files.toSet();
            newFileList.subtract(oldFileList);
            filesToUpdate.append(newFileList.toList());
        }

        modelManager->updateProjectInfo(pinfo);
        m_codeModelFuture = modelManager->updateSourceFiles(filesToUpdate);
    }
}

/**
 * Expands environment variables in the given \a string when they are written
 * like $$(VARIABLE).
 */
static void expandEnvironmentVariables(const QProcessEnvironment &env, QString &string)
{
    const static QRegExp candidate(QLatin1String("\\$\\$\\((.+)\\)"));

    int index = candidate.indexIn(string);
    while (index != -1) {
        const QString value = env.value(candidate.cap(1));

        string.replace(index, candidate.matchedLength(), value);
        index += value.length();

        index = candidate.indexIn(string, index);
    }
}

/**
 * Expands environment variables and converts the path from relative to the
 * project to an absolute path.
 *
 * The \a map variable is an optional argument that will map the returned
 * absolute paths back to their original \a entries.
 */
QStringList GenericProject::processEntries(const QStringList &paths,
                                           QHash<QString, QString> *map) const
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QDir projectDir(QFileInfo(m_fileName).dir());

    QStringList absolutePaths;
    foreach (const QString &path, paths) {
        QString trimmedPath = path.trimmed();
        if (trimmedPath.isEmpty())
            continue;

        expandEnvironmentVariables(env, trimmedPath);

        const QString absPath = QFileInfo(projectDir, trimmedPath).absoluteFilePath();
        absolutePaths.append(absPath);
        if (map)
            map->insert(absPath, trimmedPath);
    }
    absolutePaths.removeDuplicates();
    return absolutePaths;
}

QStringList GenericProject::allIncludePaths() const
{
    QStringList paths;
    paths += m_includePaths;
    paths += m_projectIncludePaths;
    paths.removeDuplicates();
    return paths;
}

QStringList GenericProject::projectIncludePaths() const
{ return m_projectIncludePaths; }

QStringList GenericProject::files() const
{ return m_files; }

QStringList GenericProject::generated() const
{ return m_generated; }

QStringList GenericProject::includePaths() const
{ return m_includePaths; }

void GenericProject::setIncludePaths(const QStringList &includePaths)
{ m_includePaths = includePaths; }

QByteArray GenericProject::defines() const
{ return m_defines; }

void GenericProject::setToolChainType(ProjectExplorer::ToolChainType type)
{
    using namespace ProjectExplorer;

    m_toolChainType = type;

    delete m_toolChain;
    m_toolChain = 0;

    if (type == ToolChain_MinGW) {
        const QLatin1String qmake_cxx("g++"); // ### FIXME
        const QString mingwDirectory; // ### FIXME

        m_toolChain = ToolChain::createMinGWToolChain(qmake_cxx, mingwDirectory);

    } else if (type == ToolChain_MSVC) {
        const QString msvcVersion; // ### FIXME
        m_toolChain = ToolChain::createMSVCToolChain(msvcVersion, false);

    } else if (type == ToolChain_WINCE) {
        const QString msvcVersion, wincePlatform; // ### FIXME
        m_toolChain = ToolChain::createWinCEToolChain(msvcVersion, wincePlatform);
    } else if (type == ToolChain_GCC) {
        const QLatin1String qmake_cxx("g++"); // ### FIXME
        m_toolChain = ToolChain::createGccToolChain(qmake_cxx);
    } else if (type == ToolChain_LINUX_ICC) {
        m_toolChain = ToolChain::createLinuxIccToolChain();
    }
}

ProjectExplorer::ToolChain *GenericProject::toolChain() const
{
    return m_toolChain;
}

ProjectExplorer::ToolChainType GenericProject::toolChainType() const
{ return m_toolChainType; }

QString GenericProject::displayName() const
{
    return m_projectName;
}

QString GenericProject::id() const
{
    return QLatin1String(Constants::GENERICPROJECT_ID);
}

Core::IFile *GenericProject::file() const
{
    return m_file;
}

ProjectExplorer::IProjectManager *GenericProject::projectManager() const
{
    return m_manager;
}

QList<ProjectExplorer::Project *> GenericProject::dependsOn()
{
    return QList<Project *>();
}

QList<ProjectExplorer::BuildConfigWidget*> GenericProject::subConfigWidgets()
{
    QList<ProjectExplorer::BuildConfigWidget*> list;
    list << new BuildEnvironmentWidget;
    return list;
}

GenericProjectNode *GenericProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList GenericProject::files(FilesMode fileMode) const
{
    Q_UNUSED(fileMode)
    return m_files; // ### TODO: handle generated files here.
}

QStringList GenericProject::buildTargets() const
{
    QStringList targets;
    targets.append(QLatin1String("all"));
    targets.append(QLatin1String("clean"));
    return targets;
}

QVariantMap GenericProject::toMap() const
{
    QVariantMap map(Project::toMap());
    map.insert(QLatin1String(TOOLCHAIN_KEY), static_cast<int>(m_toolChainType));
    return map;
}

bool GenericProject::fromMap(const QVariantMap &map)
{
    if (!Project::fromMap(map))
        return false;

    // Sanity check: We need both a buildconfiguration and a runconfiguration!
    QList<Target *> targetList = targets();
    foreach (Target *t, targetList) {
        if (!t->activeBuildConfiguration()) {
            removeTarget(t);
            delete t;
            continue;
        }
        if (!t->activeRunConfiguration())
            t->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(t));
    }

    // Add default setup:
    if (targets().isEmpty()) {
        GenericTargetFactory *factory =
                ExtensionSystem::PluginManager::instance()->getObject<GenericTargetFactory>();
        addTarget(factory->create(this, QLatin1String(GENERIC_DESKTOP_TARGET_ID)));
    }

    ToolChainType type =
            static_cast<ProjectExplorer::ToolChainType>
            (map.value(QLatin1String(TOOLCHAIN_KEY), 0).toInt());

    setToolChainType(type);

    setIncludePaths(allIncludePaths());

    refresh(Everything);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////
// GenericBuildSettingsWidget
////////////////////////////////////////////////////////////////////////////////////

GenericBuildSettingsWidget::GenericBuildSettingsWidget(GenericTarget *target)
    : m_target(target), m_buildConfiguration(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // build directory
    m_pathChooser = new Utils::PathChooser(this);
    m_pathChooser->setEnabled(true);
    m_pathChooser->setBaseDirectory(m_target->genericProject()->projectDirectory());
    fl->addRow(tr("Build directory:"), m_pathChooser);
    connect(m_pathChooser, SIGNAL(changed(QString)), this, SLOT(buildDirectoryChanged()));

    // tool chain
    QComboBox *toolChainChooser = new QComboBox;
    toolChainChooser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    using namespace ProjectExplorer;
    int index = 0;
    int selectedIndex = -1;
    foreach (ToolChainType tc, ToolChain::supportedToolChains()) {
        toolChainChooser->addItem(ToolChain::toolChainName(tc), QVariant::fromValue<ToolChainType>(tc));
        if (m_target->genericProject()->toolChainType() == tc)
            selectedIndex = index;
        ++index;
    }

    toolChainChooser->setCurrentIndex(selectedIndex);
    fl->addRow(tr("Tool chain:"), toolChainChooser);
    connect(toolChainChooser, SIGNAL(activated(int)), this, SLOT(toolChainSelected(int)));
}

GenericBuildSettingsWidget::~GenericBuildSettingsWidget()
{ }

QString GenericBuildSettingsWidget::displayName() const
{ return tr("Generic Manager"); }

void GenericBuildSettingsWidget::init(BuildConfiguration *bc)
{
    m_buildConfiguration = static_cast<GenericBuildConfiguration *>(bc);
    m_pathChooser->setPath(m_buildConfiguration->rawBuildDirectory());
}

void GenericBuildSettingsWidget::buildDirectoryChanged()
{
    m_buildConfiguration->setBuildDirectory(m_pathChooser->rawPath());
}

void GenericBuildSettingsWidget::toolChainSelected(int index)
{
    using namespace ProjectExplorer;

    QComboBox *toolChainChooser = qobject_cast<QComboBox*>(sender());
    ToolChainType type = toolChainChooser->itemData(index).value<ToolChainType>();
    m_target->genericProject()->setToolChainType(type);
}

////////////////////////////////////////////////////////////////////////////////////
// GenericProjectFile
////////////////////////////////////////////////////////////////////////////////////

GenericProjectFile::GenericProjectFile(GenericProject *parent, QString fileName)
    : Core::IFile(parent),
      m_project(parent),
      m_fileName(fileName)
{ }

GenericProjectFile::~GenericProjectFile()
{ }

bool GenericProjectFile::save(const QString &)
{
    return false;
}

QString GenericProjectFile::fileName() const
{
    return m_fileName;
}

QString GenericProjectFile::defaultPath() const
{
    return QString();
}

QString GenericProjectFile::suggestedFileName() const
{
    return QString();
}

QString GenericProjectFile::mimeType() const
{
    return Constants::GENERICMIMETYPE;
}

bool GenericProjectFile::isModified() const
{
    return false;
}

bool GenericProjectFile::isReadOnly() const
{
    return true;
}

bool GenericProjectFile::isSaveAsAllowed() const
{
    return false;
}

void GenericProjectFile::rename(const QString &newName)
{
    // Can't happen
    Q_UNUSED(newName);
    Q_ASSERT(false);
}

Core::IFile::ReloadBehavior GenericProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

void GenericProjectFile::reload(ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(flag)
    Q_UNUSED(type)
}
