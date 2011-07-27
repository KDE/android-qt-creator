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

#include "branchdialog.h"
#include "branchadddialog.h"
#include "branchmodel.h"
#include "gitclient.h"
#include "gitplugin.h"
#include "ui_branchdialog.h"
#include "stashdialog.h" // Label helpers

#include <utils/checkablemessagebox.h>
#include <vcsbase/vcsbaseoutputwindow.h>

#include <QtGui/QItemSelectionModel>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

#include <QtCore/QDebug>

namespace Git {
namespace Internal {

BranchDialog::BranchDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::BranchDialog),
    m_model(new BranchModel(GitPlugin::instance()->gitClient(), this))
{
    setModal(false);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, true); // Do not update unnecessarily

    m_ui->setupUi(this);

    connect(m_ui->refreshButton, SIGNAL(clicked()), this, SLOT(refresh()));
    connect(m_ui->addButton, SIGNAL(clicked()), this, SLOT(add()));
    connect(m_ui->checkoutButton, SIGNAL(clicked()), this, SLOT(checkout()));
    connect(m_ui->removeButton, SIGNAL(clicked()), this, SLOT(remove()));
    connect(m_ui->diffButton, SIGNAL(clicked()), this, SLOT(diff()));
    connect(m_ui->logButton, SIGNAL(clicked()), this, SLOT(log()));

    m_ui->branchView->setModel(m_model);

    connect(m_ui->branchView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(enableButtons()));

    enableButtons();
}

BranchDialog::~BranchDialog()
{
    delete m_ui;
    delete m_model;
    m_model = 0;
}

void BranchDialog::refresh(const QString &repository, bool force)
{
    if (m_repository == repository && !force)
        return;

    m_repository = repository;
    m_ui->repositoryLabel->setText(StashDialog::msgRepositoryLabel(m_repository));
    QString errorMessage;
    if (!m_model->refresh(m_repository, &errorMessage))
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);

    m_ui->branchView->expandAll();
}

void BranchDialog::enableButtons()
{
    QModelIndex idx = selectedIndex();
    const bool hasSelection = idx.isValid();
    const bool currentSelected = hasSelection && idx == m_model->currentBranch();
    const bool isLocal = m_model->isLocal(idx);
    const bool isLeaf = m_model->isLeaf(idx);

    m_ui->removeButton->setEnabled(hasSelection && !currentSelected && isLocal && isLeaf);
    m_ui->logButton->setEnabled(hasSelection && isLeaf);
    m_ui->diffButton->setEnabled(hasSelection && isLeaf);
    m_ui->checkoutButton->setEnabled(hasSelection && !currentSelected && isLocal && isLeaf);
}

void BranchDialog::refresh()
{
    refresh(m_repository, true);
}

void BranchDialog::add()
{
    QString trackedBranch = m_model->branchName(selectedIndex());
    bool isLocal = m_model->isLocal(selectedIndex());
    if (trackedBranch.isEmpty()) {
        trackedBranch = m_model->branchName(m_model->currentBranch());
        isLocal = true;
    }

    QStringList localNames = m_model->localBranchNames();

    QString suggestedNameBase = trackedBranch.mid(trackedBranch.lastIndexOf(QLatin1Char('/')) + 1);
    QString suggestedName = suggestedNameBase;
    int i = 2;
    while (localNames.contains(suggestedName)) {
        suggestedName = suggestedNameBase + QString::number(i);
        ++i;
    }

    BranchAddDialog branchAddDialog;
    branchAddDialog.setBranchName(suggestedName);
    branchAddDialog.setTrackedBranchName(trackedBranch, !isLocal);

    if (branchAddDialog.exec() == QDialog::Accepted && m_model) {
        QModelIndex idx = m_model->addBranch(branchAddDialog.branchName(), branchAddDialog.track(), trackedBranch);
        m_ui->branchView->selectionModel()->select(idx, QItemSelectionModel::Clear
                                                        | QItemSelectionModel::Select
                                                        | QItemSelectionModel::Current);
        m_ui->branchView->scrollTo(idx);
    }
}

void BranchDialog::checkout()
{
    QModelIndex idx = selectedIndex();
    Q_ASSERT(m_model->isLocal(idx));

    m_model->checkoutBranch(idx);
    enableButtons();
}

/* Prompt to delete a local branch and do so. */
void BranchDialog::remove()
{
    QModelIndex selected = selectedIndex();
    Q_ASSERT(selected != m_model->currentBranch()); // otherwise the button would not be enabled!

    QString branchName = m_model->branchName(selected);
    if (branchName.isEmpty())
        return;

    QString message = tr("Would you like to delete the branch '%1'?").arg(branchName);
    bool wasMerged = m_model->branchIsMerged(selected);
    if (!wasMerged)
        message = tr("Would you like to delete the <b>unmerged</b> branch '%1'?").arg(branchName);

    if (QMessageBox::question(this, tr("Delete Branch"), message, QMessageBox::Yes|QMessageBox::No,
                              wasMerged ? QMessageBox::Yes : QMessageBox::No) == QMessageBox::Yes)
        m_model->removeBranch(selected);
}

void BranchDialog::diff()
{
    QString branchName = m_model->branchName(selectedIndex());
    if (branchName.isEmpty())
        return;
    GitPlugin::instance()->gitClient()->diffBranch(m_repository, QStringList(), branchName);
}

void BranchDialog::log()
{
    QString branchName = m_model->branchName(selectedIndex());
    if (branchName.isEmpty())
        return;
    GitPlugin::instance()->gitClient()->graphLog(m_repository, branchName);
}

void BranchDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

QModelIndex BranchDialog::selectedIndex()
{
    QModelIndexList selected = m_ui->branchView->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
        return QModelIndex();
    return selected.at(0);
}

} // namespace Internal
} // namespace Git
