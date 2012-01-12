/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef PROJECTEXPLORER_H
#define PROJECTEXPLORER_H

#include "projectexplorer_export.h"
#include "projectexplorerconstants.h"

#include <extensionsystem/iplugin.h>

#include <QtCore/QPair>

QT_BEGIN_NAMESPACE
class QPoint;
class QMenu;
class QAction;
QT_END_NAMESPACE

namespace Core {
class IMode;
}

namespace ProjectExplorer {
class BuildManager;
class RunControl;
class SessionManager;
class RunConfiguration;
class IRunControlFactory;
class Project;
class Node;
class BuildConfiguration;
class ProjectNode;

namespace Internal {
struct ProjectExplorerSettings;
}

struct ProjectExplorerPluginPrivate;

class PROJECTEXPLORER_EXPORT ProjectExplorerPlugin
    : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    ProjectExplorerPlugin();
    ~ProjectExplorerPlugin();

    static ProjectExplorerPlugin *instance();

    bool openProject(const QString &fileName, QString *error);
    QList<Project *> openProjects(const QStringList &fileNames, QString *error);
    Q_SLOT void openProjectWelcomePage(const QString &fileName);

    SessionManager *session() const;

    Project *currentProject() const;
    Node *currentNode() const;

    void setCurrentFile(Project *project, const QString &file);
    void setCurrentNode(Node *node);

    Project *startupProject() const;

    BuildManager *buildManager() const;

    bool saveModifiedFiles();

    void showContextMenu(QWidget *view, const QPoint &globalPos, Node *node);

    //PluginInterface
    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    void setProjectExplorerSettings(const Internal::ProjectExplorerSettings &pes);
    Internal::ProjectExplorerSettings projectExplorerSettings() const;

    void startRunControl(RunControl *runControl, RunMode runMode);

    // internal public for FlatModel
    void renameFile(Node *node, const QString &to);
    static QStringList projectFilePatterns();
    bool coreAboutToClose();
    QList<QPair<QString, QString> > recentProjects();

    bool canRun(Project *pro, RunMode runMode);
    QString cannotRunReason(Project *project, RunMode runMode);
    void runProject(Project *pro, RunMode, const bool forceSkipDeploy = false);
    void runRunConfiguration(ProjectExplorer::RunConfiguration *rc, RunMode runMode,
                             const bool forceSkipDeploy = false);

    void addExistingFiles(ProjectExplorer::ProjectNode *projectNode, const QStringList &filePaths);
    void addExistingFiles(const QStringList &filePaths);

    void buildProject(ProjectExplorer::Project *p);

    QList<RunControl *> runControls() const;

    static QString displayNameForStepId(const QString &stepId);

signals:
    void runControlStarted(ProjectExplorer::RunControl *rc);
    void runControlFinished(ProjectExplorer::RunControl *rc);
    void aboutToShowContextMenu(ProjectExplorer::Project *project,
                                ProjectExplorer::Node *node);

    // Is emitted when a project has been added/removed,
    // or the file list of a specific project has changed.
    void fileListChanged();

    void currentProjectChanged(ProjectExplorer::Project *project);
    void currentNodeChanged(ProjectExplorer::Node *node, ProjectExplorer::Project *project);
    void aboutToExecuteProject(ProjectExplorer::Project *project, RunMode runMode);
    void recentProjectsChanged();

    void settingsChanged();

    void updateRunActions();

public slots:
    void openOpenProjectDialog();

private slots:
    void buildStateChanged(ProjectExplorer::Project * pro);
    void buildQueueFinished(bool success);
    void buildProjectOnly();
    void buildProject();
    void buildProjectContextMenu();
    void buildSession();
    void rebuildProjectOnly();
    void rebuildProject();
    void rebuildProjectContextMenu();
    void rebuildSession();
    void deployProjectOnly();
    void deployProject();
    void deployProjectContextMenu();
    void deploySession();
    void cleanProjectOnly();
    void cleanProject();
    void cleanProjectContextMenu();
    void cleanSession();
    void cancelBuild();
    void loadAction();
    void unloadProject();
    void clearSession();
    void newProject();
    void showSessionManager();
    void populateOpenWithMenu();
    void updateSessionMenu();
    void setSession(QAction *action);

    void determineSessionToRestoreAtStartup();
    void restoreSession();
    void loadSession(const QString &session);
    void runProject();
    void runProjectWithoutDeploy();
    void runProjectContextMenu();
    void savePersistentSettings();

    void addNewFile();
    void addExistingFiles();
    void addNewSubproject();
    void removeProject();
    void openFile();
    void searchOnFileSystem();
    void showInGraphicalShell();
    void removeFile();
    void deleteFile();
    void renameFile();
    void setStartupProject();
    void setStartupProject(ProjectExplorer::Project *project);

    void updateRecentProjectMenu();
    void clearRecentProjects();
    void openRecentProject();
    void openTerminalHere();

    void invalidateProject(ProjectExplorer::Project *project);

    void setCurrentFile(const QString &filePath);

    void runControlFinished();

    void projectAdded(ProjectExplorer::Project *pro);
    void projectRemoved(ProjectExplorer::Project *pro);
    void startupProjectChanged(); // Calls updateRunAction
    void activeTargetChanged();
    void activeRunConfigurationChanged();

    void updateDeployActions();
    void slotUpdateRunActions();

    void currentModeChanged(Core::IMode *mode, Core::IMode *oldMode);
    void updateActions();
    void loadCustomWizards();
    void updateVariable(const QByteArray &variable);
    void updateRunWithoutDeployMenu();

    void publishProject();
    void updateWelcomePage();

#ifdef WITH_TESTS
    void testGccOutputParsers_data();
    void testGccOutputParsers();

    void testClangOutputParser_data();
    void testClangOutputParser();

    void testLinuxIccOutputParsers_data();
    void testLinuxIccOutputParsers();

    void testGnuMakeParserParsing_data();
    void testGnuMakeParserParsing();
    void testGnuMakeParserTaskMangling_data();
    void testGnuMakeParserTaskMangling();

    void testMsvcOutputParsers_data();
    void testMsvcOutputParsers();

    void testGccAbiGuessing_data();
    void testGccAbiGuessing();

    void testAbiOfBinary_data();
    void testAbiOfBinary();
    void testFlavorForOs();
#endif

private:
    QString directoryFor(Node *node);
    QString pathFor(Node *node);
    void deploy(QList<Project *>);
    int queue(QList<Project *>, QStringList stepIds);
    void updateContextMenuActions();
    bool parseArguments(const QStringList &arguments, QString *error);
    void executeRunConfiguration(RunConfiguration *, RunMode mode);
    bool hasBuildSettings(Project *pro);
    QPair<bool, QString> buildSettingsEnabledForSession();
    QPair<bool, QString> buildSettingsEnabled(Project *pro);
    bool hasDeploySettings(Project *pro);

    void setCurrent(Project *project, QString filePath, Node *node);

    QStringList allFilesWithDependencies(Project *pro);
    IRunControlFactory *findRunControlFactory(RunConfiguration *config, RunMode mode);

    void addToRecentProjects(const QString &fileName, const QString &displayName);

    static ProjectExplorerPlugin *m_instance;
    ProjectExplorerPluginPrivate *d;
};

} // namespace ProjectExplorer

#endif // PROJECTEXPLORER_H
