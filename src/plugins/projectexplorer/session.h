/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef SESSION_H
#define SESSION_H

#include "projectexplorer_export.h"

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QTimer;
QT_END_NAMESPACE

namespace Core {
class ICore;
class IMode;
class IEditor;
class IFile;
}

namespace ProjectExplorer {

class Project;
class Node;
class SessionNode;
class SessionManager;

namespace Internal {
class SessionFile;
class SessionNodeImpl;
} // namespace Internal

// TODO the interface of this class is not really great

// The implementation suffers that all the functions from the
// public interface just wrap around functions which do the actual work

// This could be improved.
class PROJECTEXPLORER_EXPORT SessionManager : public QObject
{
    Q_OBJECT

public:
    explicit SessionManager(QObject *parent = 0);
    ~SessionManager();

    // higher level session management
    QString activeSession() const;
    QString lastSession() const;
    QStringList sessions() const;

    // creates a new default session and switches to it
    void createAndLoadNewDefaultSession();

    // Just creates a new session (Does not actually create the file)
    bool createSession(const QString &session);

    // delete session name from session list
    // delete file from disk
    bool deleteSession(const QString &session);

    bool cloneSession(const QString &original, const QString &clone);
    bool renameSession(const QString &original, const QString &newName);

    // loads a session, takes a session name (not filename)
    bool loadSession(const QString &session);

    bool save();
    bool clear();

    void addProject(Project *project);
    void addProjects(const QList<Project*> &projects);
    void removeProject(Project *project);
    void removeProjects(QList<Project *> remove);

    void setStartupProject(Project *startupProject);

    QList<Project *> dependencies(const Project *project) const;
    bool hasDependency(const Project *project, const Project *depProject) const;
    bool canAddDependency(const Project *project, const Project *depProject) const;
    bool addDependency(Project *project, Project *depProject);
    void removeDependency(Project *project, Project *depProject);

    QString currentSession() const;
    Project *startupProject() const;

    const QList<Project *> &projects() const;

    bool isDefaultVirgin() const;
    bool isDefaultSession(const QString &session) const;

    // Let other plugins store persistent values within the session file
    void setValue(const QString &name, const QVariant &value);
    QVariant value(const QString &name);

    // NBS rewrite projectOrder (dependency management)
    QList<Project *> projectOrder(Project *project = 0) const;
    QAbstractItemModel *model(const QString &modelId) const;

    SessionNode *sessionNode() const;

    Project *projectForNode(ProjectExplorer::Node *node) const;
    Node *nodeForFile(const QString &fileName, Project *project = 0) const;
    Project *projectForFile(const QString &fileName) const;


    void reportProjectLoadingProgress();

signals:
    void projectAdded(ProjectExplorer::Project *project);
    void singleProjectAdded(ProjectExplorer::Project *project);
    void aboutToRemoveProject(ProjectExplorer::Project *project);

    void projectRemoved(ProjectExplorer::Project *project);

    void startupProjectChanged(ProjectExplorer::Project *project);

    void sessionLoaded();
    void aboutToUnloadSession();
    void aboutToSaveSession();
    void dependencyChanged(ProjectExplorer::Project *a, ProjectExplorer::Project *b);

private slots:
    void saveActiveMode(Core::IMode *mode);
    void clearProjectFileCache();
    void setEditorCodec(Core::IEditor *editor, const QString &fileName);
    void updateWindowTitle();

    void markSessionFileDirty(bool makeDefaultVirginDirty = true);

private:
    bool loadImpl(const QString &fileName);
    bool createImpl(const QString &fileName);
    QString sessionNameToFileName(const QString &session) const;
    bool projectContainsFile(Project *p, const QString &fileName) const;

    bool recursiveDependencyCheck(const QString &newDep, const QString &checkDep) const;
    QStringList dependencies(const QString &proName) const;
    QStringList dependenciesOrder() const;

    void updateName(const QString &session);

    Core::ICore *m_core;

    Internal::SessionFile *m_file;
    Internal::SessionNodeImpl *m_sessionNode;
    QString m_sessionName;
    bool m_virginSession;

    mutable QStringList m_sessions;

    mutable QHash<Project *, QStringList> m_projectFileCache;
    QTimer *m_autoSaveSessionTimer;
};

} // namespace ProjectExplorer

#endif // SESSION_H
