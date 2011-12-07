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

#ifndef AUTOTOOLSPROJECTNODE_H
#define AUTOTOOLSPROJECTNODE_H

#include <projectexplorer/projectnodes.h>
#include <coreplugin/ifile.h>

namespace AutotoolsProjectManager {
namespace Internal {

class AutotoolsProject;

/**
 * @brief Implementation of the ProjectExplorer::ProjectNode interface.
 *
 * A project node represents a file or a folder of the project tree.
 * No special operations (addFiles(), removeFiles(), renameFile(), ..)
 * are offered.
 *
 * @see AutotoolsProject
 */
class AutotoolsProjectNode : public ProjectExplorer::ProjectNode
{
    Q_OBJECT

public:
    AutotoolsProjectNode(AutotoolsProject *project, Core::IFile *projectFile);

    bool hasBuildTargets() const;
    QList<ProjectExplorer::ProjectNode::ProjectAction> supportedActions(Node *node) const;
    bool canAddSubProject(const QString &proFilePath) const;
    bool addSubProjects(const QStringList &proFilePaths);
    bool removeSubProjects(const QStringList &proFilePaths);
    bool addFiles(const ProjectExplorer::FileType fileType,
                           const QStringList &filePaths,
                           QStringList *notAdded = 0);
    bool removeFiles(const ProjectExplorer::FileType fileType,
                              const QStringList &filePaths,
                              QStringList *notRemoved = 0);
    bool deleteFiles(const ProjectExplorer::FileType fileType,
                             const QStringList &filePaths);
    bool renameFile(const ProjectExplorer::FileType fileType,
                             const QString &filePath,
                             const QString &newFilePath);
    QList<ProjectExplorer::RunConfiguration *> runConfigurationsFor(Node *node);

private:
    AutotoolsProject *m_project;
    Core::IFile *m_projectFile;

    // TODO: AutotoolsProject calls the protected method addFileNodes() from AutotoolsProjectNode.
    // Instead of this friend declaration, a public interface might be preferable.
    friend class AutotoolsProject;
};

} // namespace Internal
} // namespace AutotoolsProjectManager

#endif // AUTOTOOLSPROJECTNODE_H
