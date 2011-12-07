/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010-2011 Openismus GmbH.
**   Authors: Peter Penz (ppenz@openismus.com)
**            Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#ifndef AUTOGENSTEP_H
#define AUTOGENSTEP_H

#include <projectexplorer/abstractprocessstep.h>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace AutotoolsProjectManager {
namespace Internal {

class AutotoolsProject;
class AutotoolsBuildConfiguration;
class AutogenStep;
class AutogenStepConfigWidget;

/////////////////////////////
// AutogenStepFactory class
/////////////////////////////
/**
 * @brief Implementation of the ProjectExplorer::IBuildStepFactory interface.
 *
 * This factory is used to create instances of AutogenStep.
 */
class AutogenStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    AutogenStepFactory(QObject *parent = 0);

    QStringList availableCreationIds(ProjectExplorer::BuildStepList *bc) const;
    QString displayNameForId(const QString &id) const;
    bool canCreate(ProjectExplorer::BuildStepList *parent, const QString &id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const QString &id);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source);
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);
};

///////////////////////
// AutogenStep class
///////////////////////
/**
 * @brief Implementation of the ProjectExplorer::AbstractProcessStep interface.
 *
 * A autogen step can be configured by selecting the "Projects" button of Qt Creator
 * (in the left hand side menu) and under "Build Settings".
 *
 * It is possible for the user to specify custom arguments. The corresponding
 * configuration widget is created by AutogenStep::createConfigWidget and is
 * represented by an instance of the class AutogenStepConfigWidget.
 */

class AutogenStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class AutogenStepFactory;
    friend class AutogenStepConfigWidget;

public:
    AutogenStep(ProjectExplorer::BuildStepList *bsl);

    AutotoolsBuildConfiguration *autotoolsBuildConfiguration() const;
    bool init();
    void run(QFutureInterface<bool> &interface);
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    bool immutable() const;
    QString additionalArguments() const;
    QVariantMap toMap() const;

public slots:
    void setAdditionalArguments(const QString &list);

signals:
    void additionalArgumentsChanged(const QString &);

protected:
    AutogenStep(ProjectExplorer::BuildStepList *bsl, AutogenStep *bs);
    AutogenStep(ProjectExplorer::BuildStepList *bsl, const QString &id);

    bool fromMap(const QVariantMap &map);

private:
    void ctor();

    QString m_additionalArguments;
    bool m_runAutogen;
};

//////////////////////////////////
// AutogenStepConfigWidget class
//////////////////////////////////
/**
 * @brief Implementation of the ProjectExplorer::BuildStepConfigWidget interface.
 *
 * Allows to configure a autogen step in the GUI.
 */
class AutogenStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    AutogenStepConfigWidget(AutogenStep *autogenStep);

    QString displayName() const;
    QString summaryText() const;

private slots:
    void updateDetails();

private:
    AutogenStep *m_autogenStep;
    QString m_summaryText;
    QLineEdit *m_additionalArguments;
};

} // namespace Internal
} // namespace AutotoolsProjectManager

#endif // AUTOGENSTEP_H

