/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include "publishingwizardselectiondialog.h"
#include "ui_publishingwizardselectiondialog.h"

#include "ipublishingwizardfactory.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/project.h>
#include <utils/qtcassert.h>

#include <QtGui/QPushButton>

namespace ProjectExplorer {
namespace Internal {

PublishingWizardSelectionDialog::PublishingWizardSelectionDialog(const Project *project,
        QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PublishingWizardSelectionDialog),
    m_project(project)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Start Wizard"));
    const QList<IPublishingWizardFactory *> &factories
        = ExtensionSystem::PluginManager::instance()->getObjects<IPublishingWizardFactory>();
    foreach (const IPublishingWizardFactory * const factory, factories) {
        if (factory->canCreateWizard(project)) {
            m_factories << factory;
            ui->serviceComboBox->addItem(factory->displayName());
        }
    }
    if (!m_factories.isEmpty()) {
        connect(ui->serviceComboBox, SIGNAL(currentIndexChanged(int)),
            SLOT(handleWizardIndexChanged(int)));
        ui->serviceComboBox->setCurrentIndex(0);
        handleWizardIndexChanged(ui->serviceComboBox->currentIndex());
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->descriptionTextArea->appendHtml(QLatin1String("<font color=\"red\">")
            + tr("Publishing is currently not possible for project '%1'.")
                  .arg(project->displayName())
            + QLatin1String("</font>"));
    }
}

PublishingWizardSelectionDialog::~PublishingWizardSelectionDialog()
{
    delete ui;
}

QWizard *PublishingWizardSelectionDialog::createSelectedWizard() const
{
    return m_factories.at(ui->serviceComboBox->currentIndex())->createWizard(m_project);
}

void PublishingWizardSelectionDialog::handleWizardIndexChanged(int index)
{
    ui->descriptionTextArea->setPlainText(m_factories.at(index)->description());
}

} // namespace Internal
} // namespace ProjectExplorer
