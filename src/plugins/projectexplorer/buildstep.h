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

#ifndef BUILDSTEP_H
#define BUILDSTEP_H

#include "projectconfiguration.h"
#include "projectexplorer_export.h"

#include <QtCore/QFutureInterface>
#include <QtGui/QWidget>

namespace ProjectExplorer {
class Task;
class BuildConfiguration;
class BuildStepList;
class DeployConfiguration;
class Target;

class BuildStepConfigWidget;

// Documentation inside.
class PROJECTEXPLORER_EXPORT BuildStep : public ProjectConfiguration
{
    Q_OBJECT

protected:
    BuildStep(BuildStepList *bsl, const QString &id);
    BuildStep(BuildStepList *bsl, BuildStep *bs);

public:
    virtual ~BuildStep();

    virtual bool init() = 0;

    virtual void run(QFutureInterface<bool> &fi) = 0;

    virtual BuildStepConfigWidget *createConfigWidget() = 0;

    virtual bool immutable() const;
    virtual bool runInGuiThread() const;
    virtual void cancel();

    BuildConfiguration *buildConfiguration() const;
    DeployConfiguration *deployConfiguration() const;
    ProjectConfiguration *projectConfiguration() const;
    Target *target() const;
    Project *project() const;

    enum OutputFormat { NormalOutput, ErrorOutput, MessageOutput, ErrorMessageOutput };
    enum OutputNewlineSetting { DoAppendNewline, DontAppendNewline };

signals:
    void addTask(const ProjectExplorer::Task &task);

    void addOutput(const QString &string, ProjectExplorer::BuildStep::OutputFormat format,
        ProjectExplorer::BuildStep::OutputNewlineSetting newlineSetting = DoAppendNewline) const;

    void finished();
};

class PROJECTEXPLORER_EXPORT IBuildStepFactory :
    public QObject
{
    Q_OBJECT

public:
    explicit IBuildStepFactory(QObject *parent = 0);
    virtual ~IBuildStepFactory();

    // used to show the list of possible additons to a target, returns a list of types
    virtual QStringList availableCreationIds(BuildStepList *parent) const = 0;
    // used to translate the types to names to display to the user
    virtual QString displayNameForId(const QString &id) const = 0;

    virtual bool canCreate(BuildStepList *parent, const QString &id) const = 0;
    virtual BuildStep *create(BuildStepList *parent, const QString &id) = 0;
    // used to recreate the runConfigurations when restoring settings
    virtual bool canRestore(BuildStepList *parent, const QVariantMap &map) const = 0;
    virtual BuildStep *restore(BuildStepList *parent, const QVariantMap &map) = 0;
    virtual bool canClone(BuildStepList *parent, BuildStep *product) const = 0;
    virtual BuildStep *clone(BuildStepList *parent, BuildStep *product) = 0;
};

class PROJECTEXPLORER_EXPORT BuildConfigWidget
    : public QWidget
{
    Q_OBJECT
public:
    BuildConfigWidget()
        :QWidget(0)
        {}

    virtual QString displayName() const = 0;

    // This is called to set up the config widget before showing it
    virtual void init(BuildConfiguration *bc) = 0;

signals:
    void displayNameChanged(const QString &);
};

class PROJECTEXPLORER_EXPORT BuildStepConfigWidget
    : public QWidget
{
    Q_OBJECT
public:
    BuildStepConfigWidget()
        : QWidget()
        {}
    virtual QString summaryText() const = 0;
    virtual QString additionalSummaryText() const { return QString(); }
    virtual QString displayName() const = 0;
    virtual bool showWidget() const { return true; }

signals:
    void updateSummary();
    void updateAdditionalSummary();
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::BuildStep::OutputFormat)
Q_DECLARE_METATYPE(ProjectExplorer::BuildStep::OutputNewlineSetting)

#endif // BUILDSTEP_H
