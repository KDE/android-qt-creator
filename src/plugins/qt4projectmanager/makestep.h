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

#ifndef MAKESTEP_H
#define MAKESTEP_H

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/task.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MakeStep;
}
QT_END_NAMESPACE

namespace ProjectExplorer {
class BuildStep;
class GnuMakeParser;
class IBuildStepFactory;
class Project;
}

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;

namespace Internal {

class MakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit MakeStepFactory(QObject *parent = 0);
    virtual ~MakeStepFactory();

    bool canCreate(ProjectExplorer::BuildStepList *parent, const QString &id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const QString &id);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source);
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);

    QStringList availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    QString displayNameForId(const QString &id) const;
};
} //namespace Internal

class Qt4Project;

class MakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class Internal::MakeStepFactory;
    friend class MakeStepConfigWidget; // TODO remove this
    // used to access internal stuff

public:
    explicit MakeStep(ProjectExplorer::BuildStepList *bsl);
    virtual ~MakeStep();

    Qt4BuildConfiguration *qt4BuildConfiguration() const;

    virtual bool init();
    virtual void run(QFutureInterface<bool> &);
    bool processSucceeded(int exitCode, QProcess::ExitStatus status);

    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;
    QString userArguments();
    void setUserArguments(const QString &arguments);
    void setClean(bool clean);
    bool isClean() const;

    QVariantMap toMap() const;

signals:
    void userArgumentsChanged();

protected:
    MakeStep(ProjectExplorer::BuildStepList *bsl, MakeStep *bs);
    MakeStep(ProjectExplorer::BuildStepList *bsl, const QString &id);
    virtual bool fromMap(const QVariantMap &map);

private:
    void ctor();
    bool m_clean;
    QString m_makeFileToCheck;
    QString m_userArgs;
    QString m_makeCmd;
    QList<ProjectExplorer::Task> m_tasks;
};

class MakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    explicit MakeStepConfigWidget(MakeStep *makeStep);
    virtual ~MakeStepConfigWidget();

    QString displayName() const;
    QString summaryText() const;
private slots:
    // User changes to our widgets
    void makeEdited();
    void makeArgumentsLineEdited();

    void updateMakeOverrideLabel();
    void updateDetails();
    void userArgumentsChanged();
    void qtVersionChanged();
private:
    Ui::MakeStep *m_ui;
    MakeStep *m_makeStep;
    QString m_summaryText;
    bool m_ignoreChange;
};

} // Qt4ProjectManager

#endif // MAKESTEP_H
