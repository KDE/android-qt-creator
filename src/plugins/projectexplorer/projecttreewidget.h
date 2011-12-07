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

#ifndef PROJECTTREEWIDGET_H
#define PROJECTTREEWIDGET_H

#include <coreplugin/inavigationwidgetfactory.h>

#include <QtGui/QWidget>
#include <QtCore/QModelIndex>

QT_FORWARD_DECLARE_CLASS(QTreeView)

namespace ProjectExplorer {

class ProjectExplorerPlugin;
class Project;
class Node;
class FolderNode;
class FileNode;

namespace Internal {

class FlatModel;

class ProjectTreeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectTreeWidget(QWidget *parent = 0);

    bool autoSynchronization() const;
    void setAutoSynchronization(bool sync, bool syncNow = true);
    bool projectFilter();
    bool generatedFilesFilter();
    QToolButton *toggleSync();

public slots:
    void toggleAutoSynchronization();
    void editCurrentItem();
    void collapseAll();

private slots:
    void setCurrentItem(ProjectExplorer::Node *node, ProjectExplorer::Project *project);
    void setProjectFilter(bool filter);
    void setGeneratedFilesFilter(bool filter);

    void handleCurrentItemChange(const QModelIndex &current);
    void showContextMenu(const QPoint &pos);
    void openItem(const QModelIndex &mainIndex);
    void handleProjectAdded(ProjectExplorer::Project *project);
    void startupProjectChanged(ProjectExplorer::Project *project);
    void initView();

    void foldersAboutToBeRemoved(FolderNode *, const QList<FolderNode*> &);
    void filesAboutToBeRemoved(FolderNode *, const QList<FileNode*> &);

    void loadExpandData();
    void saveExpandData();
    void disableAutoExpand();

private:
    void recursiveLoadExpandData(const QModelIndex &index, const QSet<QString> &data);
    void recursiveSaveExpandData(const QModelIndex &index, QStringList *data);
    ProjectExplorerPlugin *m_explorer;
    QTreeView *m_view;
    FlatModel *m_model;
    QAction *m_filterProjectsAction;
    QAction *m_filterGeneratedFilesAction;
    QToolButton *m_toggleSync;

    QModelIndex m_subIndex;
    QString m_modelId;
    bool m_autoSync;
    bool m_autoExpand;
    friend class ProjectTreeWidgetFactory;
};

class ProjectTreeWidgetFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
public:
    ProjectTreeWidgetFactory();
    ~ProjectTreeWidgetFactory();
    QString displayName() const;
    int priority() const;
    Core::Id id() const;
    QKeySequence activationSequence() const;
    Core::NavigationView createWidget();
    void restoreSettings(int position, QWidget *widget);
    void saveSettings(int position, QWidget *widget);
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTTREEWIDGET_H
