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

#ifndef BUILDMANAGER_H
#define BUILDMANAGER_H

#include "projectexplorer_export.h"
#include "buildstep.h"

#include <QtCore/QObject>
#include <QtCore/QStringList>

namespace ProjectExplorer {
class Task;
class ProjectExplorerPlugin;
class Project;

struct BuildManagerPrivate;

class PROJECTEXPLORER_EXPORT BuildManager : public QObject
{
    Q_OBJECT
public:
    explicit BuildManager(ProjectExplorerPlugin *parent);
    virtual ~BuildManager();

    void extensionsInitialized();

    bool isBuilding() const;

    bool tasksAvailable() const;

    bool buildLists(QList<BuildStepList *> bsls);
    bool buildList(BuildStepList *bsl);
    bool isBuilding(Project *p);
    bool isBuilding(BuildStep *step);

    // Append any build step to the list of build steps (currently only used to add the QMakeStep)
    void appendStep(BuildStep *step);

public slots:
    void cancel();
    // Shows without focus
    void showTaskWindow();
    void toggleTaskWindow();
    void toggleOutputWindow();
    void aboutToRemoveProject(ProjectExplorer::Project *p);

signals:
    void buildStateChanged(ProjectExplorer::Project *pro);
    void buildQueueFinished(bool success);
    void tasksChanged();
    void taskAdded(const ProjectExplorer::Task &task);
    void tasksCleared();

private slots:
    void addToTaskWindow(const ProjectExplorer::Task &task);
    void addToOutputWindow(const QString &string, ProjectExplorer::BuildStep::OutputFormat);

    void nextBuildQueue();
    void progressChanged();
    void progressTextChanged();
    void emitCancelMessage();
    void showBuildResults();
    void updateTaskCount();
    void finish();

private:
    void startBuildQueue();
    void nextStep();
    void clearBuildQueue();
    bool buildQueueAppend(QList<BuildStep *> steps);
    void incrementActiveBuildSteps(Project *pro);
    void decrementActiveBuildSteps(Project *pro);

    QScopedPointer<BuildManagerPrivate> d;
};
} // namespace ProjectExplorer

#endif // BUILDMANAGER_H
