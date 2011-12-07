/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "remotelinuxprocessesdialog.h"
#include "ui_remotelinuxprocessesdialog.h"

#include "remotelinuxprocesslist.h"

#include <QtGui/QMessageBox>
#include <QtGui/QSortFilterProxyModel>

namespace RemoteLinux {
namespace Internal {
class RemoteLinuxProcessesDialogPrivate
{
public:
    RemoteLinuxProcessesDialogPrivate(AbstractRemoteLinuxProcessList *processList)
        : processList(processList)
    {
    }

    Ui::RemoteLinuxProcessesDialog ui;
    AbstractRemoteLinuxProcessList * const processList;
    QSortFilterProxyModel proxyModel;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxProcessesDialog::RemoteLinuxProcessesDialog(AbstractRemoteLinuxProcessList *processList,
        QWidget *parent)
    : QDialog(parent), d(new RemoteLinuxProcessesDialogPrivate(processList))
{
    processList->setParent(this);

    d->ui.setupUi(this);
    d->ui.tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->proxyModel.setSourceModel(d->processList);
    d->proxyModel.setDynamicSortFilter(true);
    d->proxyModel.setFilterKeyColumn(1);
    d->ui.tableView->setModel(&d->proxyModel);
    connect(d->ui.processFilterLineEdit, SIGNAL(textChanged(QString)),
        &d->proxyModel, SLOT(setFilterRegExp(QString)));

    connect(d->ui.tableView->selectionModel(),
        SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        SLOT(handleSelectionChanged()));
    connect(d->ui.updateListButton, SIGNAL(clicked()),
        SLOT(updateProcessList()));
    connect(d->ui.killProcessButton, SIGNAL(clicked()), SLOT(killProcess()));
    connect(d->processList, SIGNAL(error(QString)),
        SLOT(handleRemoteError(QString)));
    connect(d->processList, SIGNAL(modelReset()),
        SLOT(handleProcessListUpdated()));
    connect(d->processList, SIGNAL(processKilled()),
        SLOT(handleProcessKilled()), Qt::QueuedConnection);
    connect(&d->proxyModel, SIGNAL(layoutChanged()),
        SLOT(handleProcessListUpdated()));
    handleSelectionChanged();
    updateProcessList();
}

RemoteLinuxProcessesDialog::~RemoteLinuxProcessesDialog()
{
    delete d;
}

void RemoteLinuxProcessesDialog::handleRemoteError(const QString &errorMsg)
{
    QMessageBox::critical(this, tr("Remote Error"), errorMsg);
    d->ui.updateListButton->setEnabled(true);
    handleSelectionChanged();
}

void RemoteLinuxProcessesDialog::handleProcessListUpdated()
{
    d->ui.updateListButton->setEnabled(true);
    d->ui.tableView->resizeRowsToContents();
    handleSelectionChanged();
}

void RemoteLinuxProcessesDialog::updateProcessList()
{
    d->ui.updateListButton->setEnabled(false);
    d->ui.killProcessButton->setEnabled(false);
    d->processList->update();
}

void RemoteLinuxProcessesDialog::killProcess()
{
    const QModelIndexList &indexes
        = d->ui.tableView->selectionModel()->selectedIndexes();
    if (indexes.empty())
        return;
    d->ui.updateListButton->setEnabled(false);
    d->ui.killProcessButton->setEnabled(false);
    d->processList->killProcess(d->proxyModel.mapToSource(indexes.first()).row());
}

void RemoteLinuxProcessesDialog::handleProcessKilled()
{
    updateProcessList();
}

void RemoteLinuxProcessesDialog::handleSelectionChanged()
{
    d->ui.killProcessButton->setEnabled(d->ui.tableView->selectionModel()->hasSelection());
}

} // namespace RemoteLinux
