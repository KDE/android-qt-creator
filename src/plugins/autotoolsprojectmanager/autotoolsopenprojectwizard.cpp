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

#include "autotoolsopenprojectwizard.h"

#include <QtGui/QVBoxLayout>
#include <QtGui/QFormLayout>
#include <QtGui/QLabel>
#include <QtCore/QDir>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;

//////////////////////////////////////
// AutotoolsOpenProjectWizard class
//////////////////////////////////////
AutotoolsOpenProjectWizard::AutotoolsOpenProjectWizard(AutotoolsManager *manager,
                                                       const QString &sourceDirectory,
                                                       QWidget *parent)
    : Utils::Wizard(parent),
      m_manager(manager),
      m_sourceDirectory(sourceDirectory)
{
    QDir dir(m_sourceDirectory);
    m_buildDirectory = dir.absolutePath();

    setPage(BuildPathPageId, new BuildPathPage(this));

    setStartId(BuildPathPageId);
    init();
}

void AutotoolsOpenProjectWizard::init()
{
    setOption(QWizard::NoBackButtonOnStartPage);
    setWindowTitle(tr("Autotools Wizard"));
}

AutotoolsManager *AutotoolsOpenProjectWizard::autotoolsManager() const
{
    return m_manager;
}

QString AutotoolsOpenProjectWizard::buildDirectory() const
{
    return m_buildDirectory;
}

QString AutotoolsOpenProjectWizard::sourceDirectory() const
{
    return m_sourceDirectory;
}

void AutotoolsOpenProjectWizard::setBuildDirectory(const QString &directory)
{
    m_buildDirectory = directory;
}

/////////////////////////
// BuildPathPage class
/////////////////////////
BuildPathPage::BuildPathPage(AutotoolsOpenProjectWizard *wizard)
    : QWizardPage(wizard), m_wizard(wizard)
{
    QFormLayout *fl = new QFormLayout;
    this->setLayout(fl);

    QLabel *label = new QLabel(this);
    label->setWordWrap(true);
    label->setText(tr("Please enter the directory in which you want to build your project. "
                      "Qt Creator recommends to not use the source directory for building. "
                      "This ensures that the source directory remains clean and enables multiple builds "
                      "with different settings."));
    fl->addWidget(label);
    m_pc = new Utils::PathChooser(this);
    m_pc->setBaseDirectory(m_wizard->sourceDirectory());
    m_pc->setPath(m_wizard->buildDirectory());
    connect(m_pc, SIGNAL(changed(QString)), this, SLOT(buildDirectoryChanged()));
    fl->addRow(tr("Build directory:"), m_pc);
    setTitle(tr("Build Location"));
}

void BuildPathPage::buildDirectoryChanged()
{
    m_wizard->setBuildDirectory(m_pc->path());
}
