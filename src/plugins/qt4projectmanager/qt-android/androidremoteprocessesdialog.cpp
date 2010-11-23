/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "androidremoteprocessesdialog.h"
#include "ui_androidremoteprocessesdialog.h"

#include "androidremoteprocesslist.h"

#include <QtGui/QMessageBox>
#include <QtGui/QSortFilterProxyModel>

namespace Qt4ProjectManager {
namespace Internal {

AndroidRemoteProcessesDialog::AndroidRemoteProcessesDialog(const Core::SshConnectionParameters &params,
    QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::AndroidRemoteProcessesDialog),
    m_processList(new AndroidRemoteProcessList(params, this)),
    m_proxyModel(new QSortFilterProxyModel(this))
{
    m_ui->setupUi(this);
    m_ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_proxyModel->setSourceModel(m_processList);
    m_proxyModel->setDynamicSortFilter(true);
    m_ui->tableView->setModel(m_proxyModel);
    connect(m_ui->tableView->selectionModel(),
        SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        SLOT(handleSelectionChanged()));
    connect(m_ui->updateListButton, SIGNAL(clicked()),
        SLOT(updateProcessList()));
    connect(m_ui->killProcessButton, SIGNAL(clicked()), SLOT(killProcess()));
    connect(m_processList, SIGNAL(error(QString)),
        SLOT(handleRemoteError(QString)));
    connect(m_processList, SIGNAL(modelReset()),
        SLOT(handleProcessListUpdated()));
    connect(m_processList, SIGNAL(processKilled()),
        SLOT(handleProcessKilled()), Qt::QueuedConnection);
    connect(m_proxyModel, SIGNAL(layoutChanged()),
        SLOT(handleProcessListUpdated()));
    handleSelectionChanged();
    updateProcessList();
}

AndroidRemoteProcessesDialog::~AndroidRemoteProcessesDialog()
{
    delete m_ui;
}

void AndroidRemoteProcessesDialog::handleRemoteError(const QString &errorMsg)
{
    QMessageBox::critical(this, tr("Remote error"), errorMsg);
    m_ui->updateListButton->setEnabled(true);
    handleSelectionChanged();
}

void AndroidRemoteProcessesDialog::handleProcessListUpdated()
{
    m_ui->updateListButton->setEnabled(true);
    m_ui->tableView->resizeRowsToContents();
    handleSelectionChanged();
}

void AndroidRemoteProcessesDialog::updateProcessList()
{
    m_ui->updateListButton->setEnabled(false);
    m_ui->killProcessButton->setEnabled(false);
    m_processList->update();
}

void AndroidRemoteProcessesDialog::killProcess()
{
    const QModelIndexList &indexes
        = m_ui->tableView->selectionModel()->selectedIndexes();
    if (indexes.empty())
        return;
    m_ui->updateListButton->setEnabled(false);
    m_ui->killProcessButton->setEnabled(false);
    m_processList->killProcess(m_proxyModel->mapToSource(indexes.first()).row());
}

void AndroidRemoteProcessesDialog::handleProcessKilled()
{
    updateProcessList();
}

void AndroidRemoteProcessesDialog::handleSelectionChanged()
{
    m_ui->killProcessButton->setEnabled(m_ui->tableView->selectionModel()->hasSelection());
}

} // namespace Internal
} // namespace Qt4ProjectManager
