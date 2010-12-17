/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "session.h"
#include "sessionnodeimpl.h"

#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "nodesvisitor.h"
#include "editorconfiguration.h"
#include "persistentsettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/modemanager.h>

#include <texteditor/itexteditor.h>

#include <utils/listutils.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFuture>
#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtCore/QTextCodec>

namespace {
    bool debug = false;
}

/* SessionFile definitions */
namespace ProjectExplorer {
namespace Internal {

class SessionFile : QObject
{
    Q_OBJECT

public:
    SessionFile();

    bool load(const QString &fileName);
    bool save(const QString &fileName = QString());

    QString fileName() const;
    void setFileName(const QString &fileName);

    QStringList failedProjectFileNames() const;
    void clearFailedProjectFileNames();

public slots:
    void sessionLoadingProgress();

private:
    Core::ICore *m_core;

    QString m_fileName;
    QList<Project *> m_projects;
    Project *m_startupProject;
    QStringList m_failedProjects;
    QMap<QString, QStringList> m_depMap;

    QMap<QString, QVariant> m_values;

    QFutureInterface<void> future;
    friend class ProjectExplorer::SessionManager;
};

} // namespace Internal
} // namespace ProjectExplorer

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

void SessionFile::sessionLoadingProgress()
{
    future.setProgressValue(future.progressValue() + 1);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

SessionFile::SessionFile()
  : m_core(Core::ICore::instance()),
    m_startupProject(0)
{
}

bool SessionFile::load(const QString &fileName)
{
    Q_ASSERT(!fileName.isEmpty());

    if (debug)
        qDebug() << "SessionFile::load " << fileName;

    m_fileName = fileName;

    // NPE: Load the session in the background?
    // NPE: Let FileManager monitor filename
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    PersistentSettingsReader reader;
    if (!reader.load(m_fileName)) {
        qWarning() << "SessionManager::load failed!" << fileName;
        QApplication::restoreOverrideCursor();
        return false;
    }

    m_core->progressManager()->addTask(future.future(), tr("Session"),
       QLatin1String("ProjectExplorer.SessionFile.Load"));

    const QStringList &keys = reader.restoreValue(QLatin1String("valueKeys")).toStringList();
    foreach (const QString &key, keys) {
        QVariant value = reader.restoreValue("value-" + key);
        m_values.insert(key, value);
    }

    QStringList fileList =
        reader.restoreValue(QLatin1String("ProjectList")).toStringList();
    int openEditorsCount = reader.restoreValue(QLatin1String("OpenEditors")).toInt();

    future.setProgressRange(0, fileList.count() + openEditorsCount + 2);
    future.setProgressValue(1);

    // indirectly adds projects to session
    // Keep projects that failed to load in the session!
    m_failedProjects = fileList;
    if (!fileList.isEmpty()) {
        QList<Project *> projects = ProjectExplorerPlugin::instance()->openProjects(fileList);
        foreach (Project *p, projects)
            m_failedProjects.removeAll(p->file()->fileName());
    }

    sessionLoadingProgress();

    // convert the relative paths in the dependency map to absolute paths
    QMap<QString, QVariant> depMap = reader.restoreValue(QLatin1String("ProjectDependencies")).toMap();
    QMap<QString, QVariant>::const_iterator i = depMap.constBegin();
    while (i != depMap.constEnd()) {
        const QString &key = i.key();
        QStringList values;
        foreach (const QString &value, i.value().toStringList()) {
            values << value;
        }
        m_depMap.insert(key, values);
        ++i;
    }

    // find and set the startup project
    const QString startupProject = reader.restoreValue(QLatin1String("StartupProject")).toString();
    if (!startupProject.isEmpty()) {
        const QString startupProjectPath = startupProject;
        foreach (Project *pro, m_projects) {
            if (QDir::cleanPath(pro->file()->fileName()) == startupProjectPath) {
                m_startupProject = pro;
                break;
            }
        }
        if (!m_startupProject)
            qWarning() << "Could not find startup project" << startupProjectPath;
    }

    const QVariant &editorsettings = reader.restoreValue(QLatin1String("EditorSettings"));
    if (editorsettings.isValid()) {
        connect(m_core->editorManager(), SIGNAL(editorOpened(Core::IEditor *)),
                this, SLOT(sessionLoadingProgress()));
        m_core->editorManager()->restoreState(
            QByteArray::fromBase64(editorsettings.toByteArray()));
        disconnect(m_core->editorManager(), SIGNAL(editorOpened(Core::IEditor *)),
                   this, SLOT(sessionLoadingProgress()));
    }

    if (debug)
        qDebug() << "SessionFile::load finished" << fileName;


    future.reportFinished();
    QApplication::restoreOverrideCursor();
    return true;
}

bool SessionFile::save(const QString &fileName)
{
    if (!fileName.isEmpty())
        m_fileName = fileName;

    Q_ASSERT(!m_fileName.isEmpty());

    if (debug)
        qDebug() << "SessionFile - saving " << m_fileName;

    PersistentSettingsWriter writer;

    // save the startup project
    if (m_startupProject) {
        writer.saveValue(QLatin1String("StartupProject"), m_startupProject->file()->fileName());
    }

    QStringList projectFiles;
    foreach (Project *pro, m_projects) {
        projectFiles << pro->file()->fileName();
    }
    // Restore infromation on projects that failed to load:
    projectFiles.append(m_failedProjects);

    writer.saveValue(QLatin1String("ProjectList"), projectFiles);

    QMap<QString, QVariant> depMap;
    QMap<QString, QStringList>::const_iterator i = m_depMap.constBegin();
    while (i != m_depMap.constEnd()) {
        QString key = i.key();
        QStringList values;
        foreach (const QString &value, i.value()) {
            values << value;
        }
        depMap.insert(key, values);
        ++i;
    }
    writer.saveValue(QLatin1String("ProjectDependencies"), QVariant(depMap));


    int editorCount = 0;
    QList<Core::IEditor *> editors = m_core->editorManager()->openedEditors();
    foreach (Core::IEditor *editor, editors) {
        Q_ASSERT(editor);
        if (!editor->isTemporary())
            ++editorCount;
    }
    writer.saveValue(QLatin1String("OpenEditors"), editorCount);
    writer.saveValue(QLatin1String("EditorSettings"),
                     m_core->editorManager()->saveState().toBase64());

    QMap<QString, QVariant>::const_iterator it, end;
    end = m_values.constEnd();
    QStringList keys;
    for (it = m_values.constBegin(); it != end; ++it) {
        writer.saveValue("value-" + it.key(), it.value());
        keys << it.key();
    }

    writer.saveValue("valueKeys", keys);


    if (writer.save(m_fileName, "QtCreatorSession"))
        return true;

    return false;
}

QString SessionFile::fileName() const
{
    return m_fileName;
}

void SessionFile::setFileName(const QString &fileName)
{
    m_fileName = fileName;
}

QStringList SessionFile::failedProjectFileNames() const
{
    return m_failedProjects;
}

void SessionFile::clearFailedProjectFileNames()
{
    m_failedProjects.clear();
}

/* --------------------------------- */

SessionManager::SessionManager(QObject *parent)
  : QObject(parent),
    m_core(Core::ICore::instance()),
    m_file(new SessionFile),
    m_sessionNode(new Internal::SessionNodeImpl(this)),
    m_virginSession(true)
{
    connect(m_core->modeManager(), SIGNAL(currentModeChanged(Core::IMode*)),
            this, SLOT(saveActiveMode(Core::IMode*)));

    Core::EditorManager *em = m_core->editorManager();

    connect(em, SIGNAL(editorCreated(Core::IEditor *, QString)),
            this, SLOT(setEditorCodec(Core::IEditor *, QString)));
    connect(ProjectExplorerPlugin::instance(), SIGNAL(currentProjectChanged(ProjectExplorer::Project *)),
            this, SLOT(updateWindowTitle()));
    connect(em, SIGNAL(editorOpened(Core::IEditor*)),
            this, SLOT(markSessionFileDirty()));
    connect(em, SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this, SLOT(markSessionFileDirty()));


    m_autoSaveSessionTimer = new QTimer(this);
    m_autoSaveSessionTimer->setSingleShot(true);
    m_autoSaveSessionTimer->setInterval(10000);
    connect(m_autoSaveSessionTimer, SIGNAL(timeout()),
            m_core, SIGNAL(saveSettingsRequested()));
}

SessionManager::~SessionManager()
{
    emit aboutToUnloadSession();
    delete m_file;
}


bool SessionManager::isDefaultVirgin() const
{
    return isDefaultSession(m_sessionName)
            && m_virginSession;
}

bool SessionManager::isDefaultSession(const QString &session) const
{
    return session == QLatin1String("default");
}


void SessionManager::saveActiveMode(Core::IMode *mode)
{
    setValue(QLatin1String("ActiveMode"), mode->id());
}

void SessionManager::clearProjectFileCache()
{
    // If triggered by the fileListChanged signal of one project
    // only invalidate cache for this project
    Project *pro = qobject_cast<Project*>(sender());
    if (pro)
        m_projectFileCache.remove(pro);
    else
        m_projectFileCache.clear();
}

bool SessionManager::recursiveDependencyCheck(const QString &newDep, const QString &checkDep) const
{
    if (newDep == checkDep)
        return false;

    foreach (const QString &dependency, m_file->m_depMap.value(checkDep)) {
        if (!recursiveDependencyCheck(newDep, dependency))
            return false;
    }

    return true;
}

/*
 * TODO: The dependency management exposes an interface based on projects, but
 * is internally purely string based. This is suboptimal. Probably it would be
 * nicer to map the filenames to projects on load and only map it back to
 * filenames when saving.
 */

QList<Project *> SessionManager::dependencies(const Project *project) const
{
    const QString &proName = project->file()->fileName();
    const QStringList &proDeps = m_file->m_depMap.value(proName);

    QList<Project *> projects;
    foreach (const QString &dep, proDeps) {
        if (Project *pro = projectForFile(dep))
            projects += pro;
    }

    return projects;
}

bool SessionManager::hasDependency(const Project *project, const Project *depProject) const
{
    const QString &proName = project->file()->fileName();
    const QString &depName = depProject->file()->fileName();

    const QStringList &proDeps = m_file->m_depMap.value(proName);
    return proDeps.contains(depName);
}

bool SessionManager::canAddDependency(const Project *project, const Project *depProject) const
{
    const QString &newDep = project->file()->fileName();
    const QString &checkDep = depProject->file()->fileName();

    return recursiveDependencyCheck(newDep, checkDep);
}

bool SessionManager::addDependency(Project *project, Project *depProject)
{
    const QString &proName = project->file()->fileName();
    const QString &depName = depProject->file()->fileName();

    // check if this dependency is valid
    if (!recursiveDependencyCheck(proName, depName))
        return false;

    QStringList proDeps = m_file->m_depMap.value(proName);
    if (!proDeps.contains(depName)) {
        proDeps.append(depName);
        m_file->m_depMap[proName] = proDeps;
    }
    emit dependencyChanged(project, depProject);

    return true;
}

void SessionManager::removeDependency(Project *project, Project *depProject)
{
    const QString &proName = project->file()->fileName();
    const QString &depName = depProject->file()->fileName();

    QStringList proDeps = m_file->m_depMap.value(proName);
    proDeps.removeAll(depName);
    if (proDeps.isEmpty()) {
        m_file->m_depMap.remove(proName);
    } else {
        m_file->m_depMap[proName] = proDeps;
    }
    emit dependencyChanged(project, depProject);
}

void SessionManager::setStartupProject(Project *startupProject)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << (startupProject ? startupProject->displayName() : "0");

    if (startupProject) {
        Q_ASSERT(m_file->m_projects.contains(startupProject));
    }

    if (m_file->m_startupProject == startupProject)
        return;

    m_file->m_startupProject = startupProject;
    emit startupProjectChanged(startupProject);
}

Project *SessionManager::startupProject() const
{
    return m_file->m_startupProject;
}

void SessionManager::addProject(Project *project)
{
    addProjects(QList<Project*>() << project);
}

void SessionManager::addProjects(const QList<Project*> &projects)
{
    m_virginSession = false;
    QList<Project*> clearedList;
    foreach (Project *pro, projects) {
        if (!m_file->m_projects.contains(pro)) {
            clearedList.append(pro);
            m_file->m_projects.append(pro);
            m_sessionNode->addProjectNode(pro->rootProjectNode());

            connect(pro, SIGNAL(fileListChanged()),
                    this, SLOT(clearProjectFileCache()));

            if (debug)
                qDebug() << "SessionManager - adding project " << pro->displayName();
        }
    }

    foreach (Project *pro, clearedList) {
        emit projectAdded(pro);
    }

    if (clearedList.count() == 1)
        emit singleProjectAdded(clearedList.first());

    // maybe we have a new startup project?
    if (!startupProject())
        if (!m_file->m_projects.isEmpty())
            setStartupProject(m_file->m_projects.first());
}

void SessionManager::removeProject(Project *project)
{
    m_virginSession = false;
    if (project == 0) {
        qDebug() << "SessionManager::removeProject(0) ... THIS SHOULD NOT HAPPEN";
        return;
    }
    removeProjects(QList<Project*>() << project);
}

bool SessionManager::createImpl(const QString &fileName)
{
    Q_ASSERT(!fileName.isEmpty());

    if (debug)
        qDebug() << "SessionManager - creating new session " << fileName << " ...";

    bool success = true;

    if (!m_file->fileName().isEmpty()) {
        if (!save() || !clear())
            success = false;
    }

    if (success) {
        emit aboutToUnloadSession();
        delete m_file;
        m_file = new SessionFile;
        m_file->setFileName(fileName);
        setStartupProject(0);

        if (!isDefaultVirgin()) {
            m_core->modeManager()->activateMode(Core::Constants::MODE_EDIT);
            m_core->modeManager()->setFocusToCurrentMode();
        }
    }

    m_virginSession = true;

    if (debug)
        qDebug() << "SessionManager - creating new session returns " << success;

    if (success)
        emit sessionLoaded();

    return success;
}

bool SessionManager::loadImpl(const QString &fileName)
{
    Q_ASSERT(!fileName.isEmpty());

    if (debug)
        qDebug() << "SessionManager - restoring session " << fileName << " ...";

    bool success = true;

    if (!m_file->fileName().isEmpty()) {
        if (isDefaultVirgin()) {
            // do not save initial and virgin default session
        } else if (!save() || !clear()) {
            success = false;
        }
    }

    m_virginSession = false;

    if (success) {
        emit aboutToUnloadSession();
        delete m_file;
        m_file = new SessionFile;
        if (!m_file->load(fileName)) {
            QMessageBox::warning(0, tr("Error while restoring session"),
                                    tr("Could not restore session %1").arg(fileName));
            success = false;
        }
        // m_file->load() sets the m_file->startupProject
        // but doesn't emit this signal, so we do it here
        emit startupProjectChanged(m_file->m_startupProject);

        QStringList failedProjects = m_file->failedProjectFileNames();
        if (!failedProjects.isEmpty()) {
            QString fileList =
                QDir::toNativeSeparators(failedProjects.join(QLatin1String("<br>")));
            QMessageBox * box = new QMessageBox(QMessageBox::Warning,
                                                tr("Failed to restore project files"),
                                                tr("Could not restore the following project files:<br><b>%1</b>").
                                                arg(fileList));
            QPushButton * keepButton = new QPushButton(tr("Keep projects in Session"), box);
            QPushButton * removeButton = new QPushButton(tr("Remove projects from Session"), box);
            box->addButton(keepButton, QMessageBox::AcceptRole);
            box->addButton(removeButton, QMessageBox::DestructiveRole);

            box->exec();

            if (box->clickedButton() == removeButton)
                m_file->clearFailedProjectFileNames();
        }
    }

    if (success) {
        // restore the active mode
        QString modeIdentifier = value(QLatin1String("ActiveMode")).toString();
        if (modeIdentifier.isEmpty())
            modeIdentifier = Core::Constants::MODE_EDIT;

        m_core->modeManager()->activateMode(modeIdentifier);
        m_core->modeManager()->setFocusToCurrentMode();
    }

    if (debug)
        qDebug() << "SessionManager - restoring session returned " << success;

    if (success)
        emit sessionLoaded();

    return success;
}

bool SessionManager::save()
{
    if (debug)
        qDebug() << "SessionManager - saving session" << m_sessionName;

    emit aboutToSaveSession();

    bool result = m_file->save();

    if (!result) {
        QMessageBox::warning(0, tr("Error while saving session"),
                                tr("Could not save session to file %1").arg(m_file->fileName()));
    }

    if (debug)
        qDebug() << "SessionManager - saving session returned " << result;

    return result;
}


/*!
  \fn bool SessionManager::clear()

  Returns whether the clear operation has been not cancelled & all projects could be closed.
  */
bool SessionManager::clear()
{
    if (debug)
        qDebug() << "SessionManager - clearing session ...";

    bool success = m_core->editorManager()->closeAllEditors();

    if (success) {
        if (debug)
            qDebug() << "SessionManager - Removing projects ...";

        setStartupProject(0);
        removeProjects(projects());
    }

    if (debug)
        qDebug() << "SessionManager - clearing session result is " << success;

    return success;
}

const QList<Project *> &SessionManager::projects() const
{
    return m_file->m_projects;
}

QStringList SessionManager::dependencies(const QString &proName) const
{
    QStringList result;
    foreach (const QString &dep, m_file->m_depMap.value(proName))
        result += dependencies(dep);

    result << proName;
    return result;
}

QStringList SessionManager::dependenciesOrder() const
{
    QList<QPair<QString, QStringList> > unordered;
    QStringList ordered;

    // copy the map to a temporary list
    foreach (Project *pro, projects()) {
        const QString &proName = pro->file()->fileName();
        unordered << QPair<QString, QStringList>
            (proName, m_file->m_depMap.value(proName));
    }

    while (!unordered.isEmpty()) {
        for (int i=(unordered.count()-1); i>=0; --i) {
            if (unordered.at(i).second.isEmpty()) {
                ordered << unordered.at(i).first;
                unordered.removeAt(i);
            }
        }

        // remove the handled projects from the dependency lists
        // of the remaining unordered projects
        for (int i = 0; i < unordered.count(); ++i) {
            foreach (const QString &pro, ordered) {
                QStringList depList = unordered.at(i).second;
                depList.removeAll(pro);
                unordered[i].second = depList;
            }
        }
    }

    return ordered;
}

QList<Project *> SessionManager::projectOrder(Project *project) const
{
    QList<Project *> result;

    QStringList pros;
    if (project) {
        pros = dependencies(project->file()->fileName());
    } else {
        pros = dependenciesOrder();
    }

    foreach (const QString &proFile, pros) {
        foreach (Project *pro, projects()) {
            if (pro->file()->fileName() == proFile) {
                result << pro;
                break;
            }
        }
    }

    return result;
}

Project *SessionManager::projectForNode(Node *node) const
{
    if (!node)
        return 0;

    Project *project = 0;

    FolderNode *rootProjectNode = qobject_cast<FolderNode*>(node);
    if (!rootProjectNode)
        rootProjectNode = node->parentFolderNode();
    while (rootProjectNode && rootProjectNode->parentFolderNode() != m_sessionNode)
        rootProjectNode = rootProjectNode->parentFolderNode();

    Q_ASSERT(rootProjectNode);

    QList<Project *> projectList = projects();
    foreach (Project *p, projectList) {
        if (p->rootProjectNode() == rootProjectNode) {
            project = p;
            break;
        }
    }

    return project;
}

Node *SessionManager::nodeForFile(const QString &fileName, Project *project) const
{
    Node *node = 0;
    if (!project)
        project = projectForFile(fileName);
    if (project) {
        FindNodesForFileVisitor findNodes(fileName);
        project->rootProjectNode()->accept(&findNodes);

        foreach (Node *n, findNodes.nodes()) {
            // prefer file nodes
            if (!node || (node->nodeType() != FileNodeType && n->nodeType() == FileNodeType))
                node = n;
        }
    }

    return node;
}

Project *SessionManager::projectForFile(const QString &fileName) const
{
    if (debug)
        qDebug() << "SessionManager::projectForFile(" << fileName << ")";

    const QList<Project *> &projectList = projects();

    // Check current project first
    Project *currentProject = ProjectExplorerPlugin::instance()->currentProject();
    if (currentProject && projectContainsFile(currentProject, fileName))
        return currentProject;

    foreach (Project *p, projectList)
        if (p != currentProject && projectContainsFile(p, fileName))
            return p;
    return 0;
}

bool SessionManager::projectContainsFile(Project *p, const QString &fileName) const
{
    if (!m_projectFileCache.contains(p))
        m_projectFileCache.insert(p, p->files(Project::AllFiles));

    return m_projectFileCache.value(p).contains(fileName);
}

void SessionManager::setEditorCodec(Core::IEditor *editor, const QString &fileName)
{
    if (TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor))
        if (Project *project = projectForFile(fileName)) {
            if (QTextCodec *codec = project->editorConfiguration()->defaultTextCodec())
                textEditor->setTextCodec(codec);
        }
}

QString SessionManager::currentSession() const
{
    return m_file->fileName();
}

void SessionManager::updateWindowTitle()
{
    if (isDefaultSession(m_sessionName)) {
        if (Project *currentProject = ProjectExplorerPlugin::instance()->currentProject())
            m_core->editorManager()->setWindowTitleAddition(currentProject->displayName());
        else
            m_core->editorManager()->setWindowTitleAddition("");
    } else {
        QString sessionName = m_sessionName;
        if (sessionName.isEmpty())
            sessionName = tr("Untitled");
        m_core->editorManager()->setWindowTitleAddition(sessionName);
    }
}

void SessionManager::updateName(const QString &session)
{
    m_sessionName = session;
    QString sessionName = m_sessionName;

    if (sessionName.isEmpty())
        sessionName = tr("Untitled");

    updateWindowTitle();
}


void SessionManager::removeProjects(QList<Project *> remove)
{
    QMap<QString, QStringList> resMap;

    foreach (Project *pro, remove) {
        if (debug)
            qDebug() << "SessionManager - emitting aboutToRemoveProject(" << pro->displayName() << ")";
        emit aboutToRemoveProject(pro);
    }

    // TODO: Clear m_modelProjectHash

    // Delete projects
    foreach (Project *pro, remove) {
        pro->saveSettings();
        m_file->m_projects.removeOne(pro);

        if (pro == m_file->m_startupProject)
            setStartupProject(0);

        disconnect(pro, SIGNAL(fileListChanged()),
                this, SLOT(clearProjectFileCache()));
        m_projectFileCache.remove(pro);

        if (debug)
            qDebug() << "SessionManager - emitting projectRemoved(" << pro->displayName() << ")";
        m_sessionNode->removeProjectNode(pro->rootProjectNode());
        emit projectRemoved(pro);
        delete pro;
    }

    // Refresh dependencies
    QSet<QString> projectFiles;
    foreach (Project *pro, projects()) {
        if (!remove.contains(pro))
            projectFiles.insert(pro->file()->fileName());
    }

    QSet<QString>::const_iterator i = projectFiles.begin();
    while (i != projectFiles.end()) {
        QStringList dependencies;
        foreach (const QString &dependency, m_file->m_depMap.value(*i)) {
            if (projectFiles.contains(dependency))
                dependencies << dependency;
        }
        if (!dependencies.isEmpty())
            resMap.insert(*i, dependencies);
        ++i;
    }

    m_file->m_depMap = resMap;

    if (startupProject() == 0)
        if (!m_file->m_projects.isEmpty())
            setStartupProject(m_file->m_projects.first());
}

void SessionManager::setValue(const QString &name, const QVariant &value)
{
    if (!m_file)
        return;

    if (m_file->m_values.value(name) == value)
        return;
    m_file->m_values.insert(name, value);
    markSessionFileDirty(false);
}

QVariant SessionManager::value(const QString &name)
{
    if (!m_file)
        return QVariant();
    QMap<QString, QVariant>::const_iterator it = m_file->m_values.find(name);
    return (it == m_file->m_values.constEnd()) ? QVariant() : *it;
}

QString SessionManager::activeSession() const
{
    return m_sessionName;
}

QStringList SessionManager::sessions() const
{
    if (m_sessions.isEmpty()) {
        // We are not initialized yet, so do that now
        QDir sessionDir(Core::ICore::instance()->userResourcePath());
        QList<QFileInfo> sessionFiles = sessionDir.entryInfoList(QStringList() << QLatin1String("*.qws"), QDir::NoFilter, QDir::Time);
        Q_FOREACH(const QFileInfo& fileInfo, sessionFiles) {
            if (fileInfo.completeBaseName() != "default")
                m_sessions << fileInfo.completeBaseName();
        }
        m_sessions.prepend("default");
    }
    return m_sessions;
}

QString SessionManager::sessionNameToFileName(const QString &session) const
{
    return m_core->userResourcePath() + '/' + session + ".qws";
}

void SessionManager::createAndLoadNewDefaultSession()
{
    updateName("default");
    createImpl(sessionNameToFileName(m_sessionName));
}

bool SessionManager::createSession(const QString &session)
{
    if (sessions().contains(session))
        return false;
    Q_ASSERT(m_sessions.size() > 0);
    m_sessions.insert(1, session);
    return true;
}

bool SessionManager::renameSession(const QString &original, const QString &newName)
{
    if (!cloneSession(original, newName))
        return false;
    if (original == activeSession())
        loadSession(newName);
    return deleteSession(original);
}

bool SessionManager::deleteSession(const QString &session)
{
    if (!m_sessions.contains(session))
        return false;
    m_sessions.removeOne(session);
    QFile fi(sessionNameToFileName(session));
    if (fi.exists())
        return fi.remove();
    return false;
}

bool SessionManager::cloneSession(const QString &original, const QString &clone)
{
    if (!m_sessions.contains(original))
        return false;

    QFile fi(sessionNameToFileName(original));
    // If the file does not exist, we can still clone
    if (!fi.exists() || fi.copy(sessionNameToFileName(clone))) {
        Q_ASSERT(m_sessions.size() > 0);
        m_sessions.insert(1, clone);
        return true;
    }
    return false;
}

bool SessionManager::loadSession(const QString &session)
{
    // Do nothing if we have that session already loaded,
    // exception if the session is the default virgin session
    // we still want to be able to load the default session
    if (session == m_sessionName && !isDefaultVirgin())
        return true;

    if (!sessions().contains(session))
        return false;
    QString fileName = sessionNameToFileName(session);
    if (QFileInfo(fileName).exists()) {
        if (loadImpl(fileName)) {
            updateName(session);
            return true;
        }
    } else {
        // Create a new session with that name
        if (!createImpl(sessionNameToFileName(session)))
            return false;
        updateName(session);
        return true;
    }
    return false;
}

QString SessionManager::lastSession() const
{
    QSettings *settings = m_core->settings();
    QString fileName = settings->value("ProjectExplorer/StartupSession").toString();
    return QFileInfo(fileName).completeBaseName();
}

SessionNode *SessionManager::sessionNode() const
{
    return m_sessionNode;
}

void SessionManager::reportProjectLoadingProgress()
{
    m_file->sessionLoadingProgress();
}

void SessionManager::markSessionFileDirty(bool makeDefaultVirginDirty)
{
    if (m_file && !m_file->fileName().isEmpty())
        m_autoSaveSessionTimer->start();
    if (makeDefaultVirginDirty)
        m_virginSession = false;
}

#include "session.moc"
