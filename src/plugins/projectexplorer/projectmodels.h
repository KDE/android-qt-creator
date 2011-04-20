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

#ifndef PROJECTMODELS_H
#define PROJECTMODELS_H

#include <QtCore/QAbstractItemModel>
#include <QtCore/QSet>

namespace ProjectExplorer {

class Node;
class FileNode;
class FolderNode;
class ProjectNode;
class SessionNode;

namespace Internal {

class FlatModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    FlatModel(SessionNode *rootNode, QObject *parent);

    // QAbstractItemModel
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    bool hasChildren(const QModelIndex & parent = QModelIndex()) const;

    bool canFetchMore(const QModelIndex & parent) const;
    void fetchMore(const QModelIndex & parent);

    void reset();

    void setStartupProject(ProjectNode *projectNode);

    ProjectExplorer::Node *nodeForIndex(const QModelIndex &index) const;
    QModelIndex indexForNode(const Node *node);

    bool projectFilterEnabled();
    bool generatedFilesFilterEnabled();

public slots:
    void setProjectFilterEnabled(bool filter);
    void setGeneratedFilesFilterEnabled(bool filter);

private slots:
    void aboutToHasBuildTargetsChanged(ProjectExplorer::ProjectNode *node);
    void hasBuildTargetsChanged(ProjectExplorer::ProjectNode *node);
    void foldersAboutToBeAdded(FolderNode *parentFolder, const QList<FolderNode*> &newFolders);
    void foldersAdded();

    void foldersAboutToBeRemoved(FolderNode *parentFolder, const QList<FolderNode*> &staleFolders);
    void foldersRemoved();

    // files
    void filesAboutToBeAdded(FolderNode *folder, const QList<FileNode*> &newFiles);
    void filesAdded();

    void filesAboutToBeRemoved(FolderNode *folder, const QList<FileNode*> &staleFiles);
    void filesRemoved();

private:
    void added(FolderNode* folderNode, const QList<Node*> &newNodeList);
    void removed(FolderNode* parentNode, const QList<Node*> &newNodeList);
    void removeFromCache(QList<FolderNode *> list);
    void fetchMore(FolderNode *foldernode) const;

    void recursiveAddFolderNodes(FolderNode *startNode, QList<Node *> *list, const QSet<Node *> &blackList = QSet<Node*>()) const;
    void recursiveAddFolderNodesImpl(FolderNode *startNode, QList<Node *> *list, const QSet<Node *> &blackList = QSet<Node*>()) const;
    void recursiveAddFileNodes(FolderNode *startNode, QList<Node *> *list, const QSet<Node *> &blackList = QSet<Node*>()) const;
    QList<Node*> childNodes(FolderNode *parentNode, const QSet<Node*> &blackList = QSet<Node*>()) const;

    FolderNode *visibleFolderNode(FolderNode *node) const;
    bool filter(Node *node) const;

    bool m_filterProjects;
    bool m_filterGeneratedFiles;

    SessionNode *m_rootNode;
    mutable QHash<FolderNode*, QList<Node*> > m_childNodes;
    ProjectNode *m_startupProject;

    FolderNode *m_parentFolderForChange;

    friend class FlatModelManager;
};

} // namespace Internal
} // namespace ProjectExplorer


#endif // PROJECTMODELS_H
