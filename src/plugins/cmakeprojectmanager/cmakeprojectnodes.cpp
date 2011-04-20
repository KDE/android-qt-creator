/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cmakeprojectnodes.h"

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

CMakeProjectNode::CMakeProjectNode(const QString &fileName)
    : ProjectExplorer::ProjectNode(fileName)
{
}

bool CMakeProjectNode::hasBuildTargets() const
{
    // TODO
    return true;
}

QList<ProjectExplorer::ProjectNode::ProjectAction> CMakeProjectNode::supportedActions(Node *node) const
{
    Q_UNUSED(node);
    return QList<ProjectAction>();
}

bool CMakeProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath)
    return false;
}

bool CMakeProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool CMakeProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool CMakeProjectNode::addFiles(const ProjectExplorer::FileType fileType, const QStringList &filePaths, QStringList *notAdded)
{
    Q_UNUSED(fileType)
    Q_UNUSED(filePaths)
    Q_UNUSED(notAdded)
    return false;
}

bool CMakeProjectNode::removeFiles(const ProjectExplorer::FileType fileType, const QStringList &filePaths,  QStringList *notRemoved)
{
    Q_UNUSED(fileType)
    Q_UNUSED(filePaths)
    Q_UNUSED(notRemoved)
    return false;
}

bool CMakeProjectNode::deleteFiles(const ProjectExplorer::FileType fileType, const QStringList &filePaths)
{
    Q_UNUSED(fileType)
    Q_UNUSED(filePaths)
    return false;
}

bool CMakeProjectNode::renameFile(const ProjectExplorer::FileType fileType, const QString &filePath, const QString &newFilePath)
{
    Q_UNUSED(fileType)
    Q_UNUSED(filePath)
    Q_UNUSED(newFilePath)
    return false;
}

QList<ProjectExplorer::RunConfiguration *> CMakeProjectNode::runConfigurationsFor(Node *node)
{
    Q_UNUSED(node)
    return QList<ProjectExplorer::RunConfiguration *>();
}
