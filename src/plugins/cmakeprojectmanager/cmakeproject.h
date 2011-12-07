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

#ifndef CMAKEPROJECT_H
#define CMAKEPROJECT_H

#include "cmakeprojectmanager.h"
#include "cmakeprojectnodes.h"
#include "cmakebuildconfiguration.h"
#include "cmaketarget.h"
#include "makestep.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <coreplugin/ifile.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QtCore/QXmlStreamReader>
#include <QtCore/QFileSystemWatcher>
#include <QtGui/QPushButton>
#include <QtGui/QLineEdit>

namespace CMakeProjectManager {
namespace Internal {

class CMakeFile;
class CMakeBuildSettingsWidget;
class CMakeUiCodeModelSupport;

struct CMakeBuildTarget
{
    QString title;
    QString executable; // TODO: rename to output?
    bool library;
    QString workingDirectory;
    QString makeCommand;
    QString makeCleanCommand;
    void clear();
};

class CMakeProject : public ProjectExplorer::Project
{
    Q_OBJECT
    // for changeBuildDirectory
    friend class CMakeBuildSettingsWidget;
public:
    CMakeProject(CMakeManager *manager, const QString &filename);
    ~CMakeProject();

    QString displayName() const;
    QString id() const;
    Core::IFile *file() const;
    CMakeManager *projectManager() const;

    CMakeTarget *activeTarget() const;

    QList<ProjectExplorer::Project *> dependsOn(); //NBS TODO implement dependsOn

    QList<ProjectExplorer::BuildConfigWidget*> subConfigWidgets();

    ProjectExplorer::ProjectNode *rootProjectNode() const;

    QStringList files(FilesMode fileMode) const;
    QStringList buildTargetTitles() const;
    QList<CMakeBuildTarget> buildTargets() const;
    bool hasBuildTarget(const QString &title) const;

    CMakeBuildTarget buildTargetForTitle(const QString &title);

    QString defaultBuildDirectory() const;

    bool parseCMakeLists();

    QString uicCommand() const;

    bool isProjectFile(const QString &fileName);

signals:
    /// emitted after parsing
    void buildTargetsChanged();

protected:
    bool fromMap(const QVariantMap &map);

    // called by CMakeBuildSettingsWidget
    void changeBuildDirectory(CMakeBuildConfiguration *bc, const QString &newBuildDirectory);

private slots:
    void fileChanged(const QString &fileName);
    void changeActiveBuildConfiguration(ProjectExplorer::BuildConfiguration*);
    void targetAdded(ProjectExplorer::Target *);

    void editorChanged(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);
    void uiEditorContentsChanged();
    void buildStateChanged(ProjectExplorer::Project *project);
private:
    void buildTree(CMakeProjectNode *rootNode, QList<ProjectExplorer::FileNode *> list);
    void gatherFileNodes(ProjectExplorer::FolderNode *parent, QList<ProjectExplorer::FileNode *> &list);
    ProjectExplorer::FolderNode *findOrCreateFolder(CMakeProjectNode *rootNode, QString directory);
    void updateCodeModelSupportFromEditor(const QString &uiFileName, const QString &contents);
    void createUiCodeModelSupport();
    QString uiHeaderFile(const QString &uiFile);

    CMakeManager *m_manager;
    QString m_fileName;
    CMakeFile *m_file;
    QString m_projectName;
    QString m_uicCommand;

    // TODO probably need a CMake specific node structure
    CMakeProjectNode *m_rootNode;
    QStringList m_files;
    QList<CMakeBuildTarget> m_buildTargets;
    QFileSystemWatcher *m_watcher;
    QSet<QString> m_watchedFiles;
    QFuture<void> m_codeModelFuture;

    QMap<QString, CMakeUiCodeModelSupport *> m_uiCodeModelSupport;
    Core::IEditor *m_lastEditor;
    bool m_dirtyUic;
};

class CMakeCbpParser : public QXmlStreamReader
{
public:
    bool parseCbpFile(const QString &fileName);
    QList<ProjectExplorer::FileNode *> fileList();
    QList<ProjectExplorer::FileNode *> cmakeFileList();
    QStringList includeFiles();
    QList<CMakeBuildTarget> buildTargets();
    QString projectName() const;
    QString compilerName() const;
    bool hasCMakeFiles();
private:
    void parseCodeBlocks_project_file();
    void parseProject();
    void parseBuild();
    void parseOption();
    void parseBuildTarget();
    void parseBuildTargetOption();
    void parseMakeCommand();
    void parseBuildTargetBuild();
    void parseBuildTargetClean();
    void parseCompiler();
    void parseAdd();
    void parseUnit();
    void parseUnitOption();
    void parseUnknownElement();

    QList<ProjectExplorer::FileNode *> m_fileList;
    QList<ProjectExplorer::FileNode *> m_cmakeFileList;
    QSet<QString> m_processedUnits;
    bool m_parsingCmakeUnit;
    QStringList m_includeFiles;

    CMakeBuildTarget m_buildTarget;
    bool m_buildTargetType;
    QList<CMakeBuildTarget> m_buildTargets;
    QString m_projectName;
    QString m_compiler;
};

class CMakeFile : public Core::IFile
{
    Q_OBJECT
public:
    CMakeFile(CMakeProject *parent, QString fileName);

    bool save(QString *errorString, const QString &fileName, bool autoSave);
    QString fileName() const;

    QString defaultPath() const;
    QString suggestedFileName() const;
    QString mimeType() const;

    bool isModified() const;
    bool isReadOnly() const;
    bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);

    void rename(const QString &newName);

private:
    CMakeProject *m_project;
    QString m_fileName;
};

class CMakeBuildSettingsWidget : public ProjectExplorer::BuildConfigWidget
{
    Q_OBJECT
public:
    explicit CMakeBuildSettingsWidget(CMakeTarget *target);
    QString displayName() const;

    // This is called to set up the config widget before showing it
    virtual void init(ProjectExplorer::BuildConfiguration *bc);
private slots:
    void openChangeBuildDirectoryDialog();
    void runCMake();
private:
    CMakeTarget *m_target;
    QLineEdit *m_pathLineEdit;
    QPushButton *m_changeButton;
    CMakeBuildConfiguration *m_buildConfiguration;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEPROJECT_H
