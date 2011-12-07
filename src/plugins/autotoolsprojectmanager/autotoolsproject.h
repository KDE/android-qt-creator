/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010-2011 Openismus GmbH.
**   Authors: Peter Penz (ppenz@openismus.com)
**            Patricia Santana Cruz (patriciasantanacruz@gmail.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef AUTOTOOLSPROJECT_H
#define AUTOTOOLSPROJECT_H

#include "autotoolstarget.h"

#include <coreplugin/editormanager/ieditor.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectnodes.h>
#include <utils/filesystemwatcher.h>

#include <QtCore/QPointer>
#include <QtCore/QDir>

namespace ProjectExplorer {
class ToolChain;
}

namespace AutotoolsProjectManager {
namespace Internal {
class AutotoolsConfigurationFactory;
class AutotoolsProjectFile;
class AutotoolsProjectNode;
class AutotoolsManager;
class MakefileParserThread;
class AutotoolsTarget;

/**
 * @brief Implementation of the ProjectExplorer::Project interface.
 *
 * Loads the autotools project and embeds it into the QtCreator project tree.
 * The class AutotoolsProject is the core of the autotools project plugin.
 * It is responsible to parse the Makefile.am files and do trigger project
 * updates if a Makefile.am file or a configure.ac file has been changed.
 */
class AutotoolsProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    AutotoolsProject(AutotoolsManager *manager, const QString &fileName);
    ~AutotoolsProject();

    QString displayName() const;
    QString id() const;
    Core::IFile *file() const;
    ProjectExplorer::IProjectManager *projectManager() const;
    AutotoolsTarget *activeTarget() const;
    QList<ProjectExplorer::BuildConfigWidget*> subConfigWidgets();
    ProjectExplorer::ProjectNode *rootProjectNode() const;
    QStringList files(FilesMode fileMode) const;
    QList<ProjectExplorer::Project *> dependsOn();
    QString defaultBuildDirectory() const;
    QString sourceDirectory() const;
    QStringList buildTargets() const;
    ProjectExplorer::ToolChain *toolChain() const;
    void setToolChain(ProjectExplorer::ToolChain *tc);
    QVariantMap toMap() const;

signals:
    void toolChainChanged(ProjectExplorer::ToolChain *tc);

protected:
    bool fromMap(const QVariantMap &map);

private slots:
    /**
     *  Loads the project tree by parsing the makefiles.
     */
    void loadProjectTree();

    /**
     * Is invoked when the makefile parsing by m_makefileParserThread has
     * been started. Turns the mouse cursor into a busy cursor.
     */
    void makefileParsingStarted();

    /**
     * Is invoked when the makefile parsing by m_makefileParserThread has
     * been finished. Adds all sources and files into the project tree and
     * takes care listen to file changes for Makefile.am and configure.ac
     * files.
     */
    void makefileParsingFinished();

    /**
     * Is invoked, if a file of the project tree has been changed by the user.
     * If a Makefile.am or a configure.ac file has been changed, the project
     * configuration must be updated.
     */
    void onFileChanged(const QString &file);

private:
    /**
     * Creates folder-nodes and file-nodes for the project tree.
     */
    void buildFileNodeTree(const QDir &directory,
                           const QStringList &files);

    /**
     * Helper method for buildFileNodeTree(): Inserts a new folder-node for
     * the directory \p nodeDir and inserts it into \p nodes. If no parent
     * folder exists, it will be created recursively.
     */
    ProjectExplorer::FolderNode *insertFolderNode(const QDir &nodeDir,
                                                  QHash<QString, ProjectExplorer::Node *> &nodes);

    /**
     * @return All nodes (including sub-folder- and file-nodes) for the given parent folder.
     */
    QList<ProjectExplorer::Node *> nodes(ProjectExplorer::FolderNode *parent) const;

    /**
     * This function is in charge of the code completion.
     */
    void updateCppCodeModel();

private:
    /// Project manager that has been passed in the constructor
    AutotoolsManager *m_manager;

    /// File name of the makefile that has been passed in the constructor
    QString m_fileName;
    QString m_projectName;

    /// Return value for AutotoolsProject::files()
    QStringList m_files;

    /// Return value for AutotoolsProject::file()
    AutotoolsProjectFile *m_file;

    /// Return value for AutotoolsProject::rootProjectNode()
    AutotoolsProjectNode *m_rootNode;

    /// Watches project files for changes.
    Utils::FileSystemWatcher *m_fileWatcher;
    QStringList m_watchedFiles;

    /// Responsible for parsing the makefiles asynchronously in a thread
    MakefileParserThread *m_makefileParserThread;

    ProjectExplorer::ToolChain *m_toolChain;
};

} // namespace Internal
} // namespace AutotoolsProjectManager

#endif // AUTOTOOLSPROJECT_H
