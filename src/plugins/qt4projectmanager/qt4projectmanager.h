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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QT4PROJECTMANAGER_H
#define QT4PROJECTMANAGER_H

#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/projectnodes.h>

namespace Core {
    class IEditor;
}

namespace ExtensionSystem {
class PluginManager;
}

namespace ProjectExplorer {
class Project;
class ProjectExplorerPlugin;
class Node;
}

namespace QtSupport {
class QtVersionManager;
}

namespace Qt4ProjectManager {

namespace Internal {
class Qt4Builder;
class ProFileEditorWidget;
class Qt4ProjectManagerPlugin;
}

class Qt4Project;

class Qt4Manager : public ProjectExplorer::IProjectManager
{
    Q_OBJECT

public:
    Qt4Manager(Internal::Qt4ProjectManagerPlugin *plugin);
    ~Qt4Manager();

    void init();

    void registerProject(Qt4Project *project);
    void unregisterProject(Qt4Project *project);
    void notifyChanged(const QString &name);

    ProjectExplorer::ProjectExplorerPlugin *projectExplorer() const;

    virtual QString mimeType() const;
    ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString);

    // Context information used in the slot implementations
    ProjectExplorer::Node *contextNode() const;
    void setContextNode(ProjectExplorer::Node *node);
    void setContextProject(ProjectExplorer::Project *project);
    ProjectExplorer::Project *contextProject() const;

    // Return the id string of a file
    static QString fileTypeId(ProjectExplorer::FileType type);

    enum Action { BUILD, REBUILD, CLEAN };

public slots:
    void addLibrary();
    void addLibraryContextMenu();
    void runQMake();
    void runQMakeContextMenu();
    void buildSubDirContextMenu();
    void rebuildSubDirContextMenu();
    void cleanSubDirContextMenu();

private slots:
    void editorAboutToClose(Core::IEditor *editor);
    void uiEditorContentsChanged();
    void editorChanged(Core::IEditor*);
    void updateVariable(const QString &variable);

private:
    QList<Qt4Project *> m_projects;
    void handleSubDirContexMenu(Action action);
    void addLibrary(const QString &fileName, Internal::ProFileEditorWidget *editor = 0);
    void runQMake(ProjectExplorer::Project *p, ProjectExplorer::Node *node);

    Internal::Qt4ProjectManagerPlugin *m_plugin;

    ProjectExplorer::Node *m_contextNode;
    ProjectExplorer::Project *m_contextProject;

    Core::IEditor *m_lastEditor;
    bool m_dirty;
};

} // namespace Qt4ProjectManager

#endif // QT4PROJECTMANAGER_H
