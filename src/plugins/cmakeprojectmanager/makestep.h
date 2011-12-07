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

#ifndef MAKESTEP_H
#define MAKESTEP_H

#include <projectexplorer/abstractprocessstep.h>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QListWidget;
class QListWidgetItem;
QT_END_NAMESPACE

namespace CMakeProjectManager {
namespace Internal {

class CMakeBuildConfiguration;
class MakeStepFactory;

class MakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class MakeStepFactory;
    friend class MakeStepConfigWidget; // TODO remove
    // This is for modifying internal data

public:
    MakeStep(ProjectExplorer::BuildStepList *bsl);
    virtual ~MakeStep();

    CMakeBuildConfiguration *cmakeBuildConfiguration() const;

    virtual bool init();

    virtual void run(QFutureInterface<bool> &fi);

    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;

    QStringList buildTargets() const;
    bool buildsBuildTarget(const QString &target) const;
    void setBuildTarget(const QString &target, bool on);
    void setBuildTargets(const QStringList &targets);
    void clearBuildTargets();

    QString additionalArguments() const;
    void setAdditionalArguments(const QString &list);

    void setClean(bool clean);

    QVariantMap toMap() const;


protected:
    MakeStep(ProjectExplorer::BuildStepList *bsl, MakeStep *bs);
    MakeStep(ProjectExplorer::BuildStepList *bsl, const QString &id);

    bool fromMap(const QVariantMap &map);

    // For parsing [ 76%]
    virtual void stdOutput(const QString &line);

private:
    void ctor();

    bool m_clean;
    QRegExp m_percentProgress;
    QFutureInterface<bool> *m_futureInterface;
    QStringList m_buildTargets;
    QString m_additionalArguments;
};

class MakeStepConfigWidget :public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    MakeStepConfigWidget(MakeStep *makeStep);
    virtual QString displayName() const;
    virtual QString summaryText() const;
private slots:
    void itemChanged(QListWidgetItem*);
    void additionalArgumentsEdited();
    void updateDetails();
    void buildTargetsChanged();
private:
    MakeStep *m_makeStep;
    QListWidget *m_buildTargetsList;
    QLineEdit *m_additionalArguments;
    QString m_summaryText;
};

class MakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit MakeStepFactory(QObject *parent = 0);
    virtual ~MakeStepFactory();

    virtual bool canCreate(ProjectExplorer::BuildStepList *parent, const QString &id) const;
    virtual ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const QString &id);
    virtual bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) const;
    virtual ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source);
    virtual bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    virtual ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);

    virtual QStringList availableCreationIds(ProjectExplorer::BuildStepList *bc) const;
    virtual QString displayNameForId(const QString &id) const;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // MAKESTEP_H
