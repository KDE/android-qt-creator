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

#include "autogenstep.h"
#include "autotoolsproject.h"
#include "autotoolstarget.h"
#include "autotoolsbuildconfiguration.h"
#include "autotoolsprojectconstants.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcprocess.h>

#include <QtCore/QVariantMap>
#include <QtCore/QDateTime>
#include <QtGui/QLineEdit>
#include <QtGui/QFormLayout>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;
using namespace ProjectExplorer;

const char AUTOGEN_ADDITIONAL_ARGUMENTS_KEY[] = "AutotoolsProjectManager.AutogenStep.AdditionalArguments";
const char AUTOGEN_STEP_ID[] = "AutotoolsProjectManager.AutogenStep";

/////////////////////////////
// AutogenStepFactory class
/////////////////////////////
AutogenStepFactory::AutogenStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{
}

QStringList AutogenStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->target()->project()->id() == QLatin1String(Constants::AUTOTOOLS_PROJECT_ID))
        return QStringList() << QLatin1String(AUTOGEN_STEP_ID);
    return QStringList();
}

QString AutogenStepFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(AUTOGEN_STEP_ID))
        return tr("Autogen", "Display name for AutotoolsProjectManager::AutogenStep id.");
    return QString();
}

bool AutogenStepFactory::canCreate(BuildStepList *parent, const QString &id) const
{
    if (parent->target()->project()->id() != QLatin1String(Constants::AUTOTOOLS_PROJECT_ID))
        return false;

    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return false;

    return QLatin1String(AUTOGEN_STEP_ID) == id;
}

BuildStep *AutogenStepFactory::create(BuildStepList *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    return new AutogenStep(parent);
}

bool AutogenStepFactory::canClone(BuildStepList *parent, BuildStep *source) const
{
    return canCreate(parent, source->id());
}

BuildStep *AutogenStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    return new AutogenStep(parent, static_cast<AutogenStep *>(source));
}

bool AutogenStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
}

BuildStep *AutogenStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    AutogenStep *bs = new AutogenStep(parent);
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

////////////////////////
// AutogenStep class
////////////////////////
AutogenStep::AutogenStep(ProjectExplorer::BuildStepList *bsl) :
    AbstractProcessStep(bsl, QLatin1String(AUTOGEN_STEP_ID)),
    m_runAutogen(false)
{
    ctor();
}

AutogenStep::AutogenStep(ProjectExplorer::BuildStepList *bsl, const QString &id) :
    AbstractProcessStep(bsl, id)
{
    ctor();
}

AutogenStep::AutogenStep(ProjectExplorer::BuildStepList *bsl, AutogenStep *bs) :
    AbstractProcessStep(bsl, bs),
    m_additionalArguments(bs->additionalArguments())
{
    ctor();
}

void AutogenStep::ctor()
{
    setDefaultDisplayName(tr("Autogen"));
}

AutotoolsBuildConfiguration *AutogenStep::autotoolsBuildConfiguration() const
{
    return static_cast<AutotoolsBuildConfiguration *>(buildConfiguration());
}

bool AutogenStep::init()
{
    AutotoolsBuildConfiguration *bc = autotoolsBuildConfiguration();

    setEnabled(true);

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setEnvironment(bc->environment());
    pp->setWorkingDirectory(bc->buildDirectory());
    pp->setCommand(QLatin1String("autogen.sh"));
    pp->setArguments(additionalArguments());

    return AbstractProcessStep::init();
}

void AutogenStep::run(QFutureInterface<bool> &interface)
{
    AutotoolsBuildConfiguration *bc = autotoolsBuildConfiguration();

    // Check whether we need to run autogen.sh
    const QFileInfo configureInfo(bc->buildDirectory() + QLatin1String("/configure"));
    const QFileInfo configureAcInfo(bc->buildDirectory() + QLatin1String("/configure.ac"));
    const QFileInfo makefileAmInfo(bc->buildDirectory() + QLatin1String("/Makefile.am"));

    if (!configureInfo.exists()
        || configureInfo.lastModified() < configureAcInfo.lastModified()
        || configureInfo.lastModified() < makefileAmInfo.lastModified()) {
        m_runAutogen = true;
    }

    if (!m_runAutogen) {
        emit addOutput(tr("Configuration unchanged, skipping autogen step."), BuildStep::MessageOutput);
        interface.reportResult(true);
        return;
    }

    m_runAutogen = false;
    AbstractProcessStep::run(interface);
}

ProjectExplorer::BuildStepConfigWidget* AutogenStep::createConfigWidget()
{
    return new AutogenStepConfigWidget(this);
}

bool AutogenStep::immutable() const
{
    return false;
}

void AutogenStep::setAdditionalArguments(const QString &list)
{
    if (list == m_additionalArguments)
        return;

    m_additionalArguments = list;
    m_runAutogen = true;

    emit additionalArgumentsChanged(list);
}

QString AutogenStep::additionalArguments() const
{
    return m_additionalArguments;
}

QVariantMap AutogenStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(QLatin1String(AUTOGEN_ADDITIONAL_ARGUMENTS_KEY), m_additionalArguments);
    return map;
}

bool AutogenStep::fromMap(const QVariantMap &map)
{
    m_additionalArguments = map.value(QLatin1String(AUTOGEN_ADDITIONAL_ARGUMENTS_KEY)).toString();

    return BuildStep::fromMap(map);
}

//////////////////////////////////
// AutogenStepConfigWidget class
//////////////////////////////////
AutogenStepConfigWidget::AutogenStepConfigWidget(AutogenStep *autogenStep) :
    m_autogenStep(autogenStep),
    m_summaryText(),
    m_additionalArguments(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    m_additionalArguments = new QLineEdit(this);
    fl->addRow(tr("Arguments:"), m_additionalArguments);
    m_additionalArguments->setText(m_autogenStep->additionalArguments());

    updateDetails();

    connect(m_additionalArguments, SIGNAL(textChanged(QString)),
            autogenStep, SLOT(setAdditionalArguments(QString)));
    connect(autogenStep, SIGNAL(additionalArgumentsChanged(QString)),
            this, SLOT(updateDetails()));
}

QString AutogenStepConfigWidget::displayName() const
{
    return tr("Autogen", "AutotoolsProjectManager::AutogenStepConfigWidget display name.");
}

QString AutogenStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void AutogenStepConfigWidget::updateDetails()
{
    AutotoolsBuildConfiguration *bc = m_autogenStep->autotoolsBuildConfiguration();

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setEnvironment(bc->environment());
    param.setWorkingDirectory(bc->buildDirectory());
    param.setCommand("autogen.sh");
    param.setArguments(m_autogenStep->additionalArguments());
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}
