/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#include "androidprofilesupdatedialog.h"
#include "ui_androidprofilesupdatedialog.h"

#include <qt4projectmanager/qt4nodes.h>

#include <QtGui/QTableWidgetItem>

namespace Qt4ProjectManager {
namespace Internal {

AndroidProFilesUpdateDialog::AndroidProFilesUpdateDialog(QWidget *parent)
    : QDialog(parent),
    ui(new Ui::AndroidProFilesUpdateDialog)
{
    ui->setupUi(this);
//    ui->tableWidget->setRowCount(models.count());
//    ui->tableWidget->setHorizontalHeaderItem(0,
//        new QTableWidgetItem(tr("Updateable Project Files")));
//    for (int row = 0; row < models.count(); ++row) {
//        QTableWidgetItem *const item
//            = new QTableWidgetItem(models.at(row)->proFilePath());
//        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
//        item->setCheckState(Qt::Unchecked);
//        ui->tableWidget->setItem(row, 0, item);
//    }
//    ui->tableWidget->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
//    ui->tableWidget->resizeRowsToContents();
//    connect(ui->checkAllButton, SIGNAL(clicked()), this, SLOT(checkAll()));
//    connect(ui->uncheckAllButton, SIGNAL(clicked()), this, SLOT(uncheckAll()));
}

AndroidProFilesUpdateDialog::~AndroidProFilesUpdateDialog()
{
    delete ui;
}

void AndroidProFilesUpdateDialog::checkAll()
{
    setCheckStateForAll(Qt::Checked);
}

void AndroidProFilesUpdateDialog::uncheckAll()
{
    setCheckStateForAll(Qt::Unchecked);
}

void AndroidProFilesUpdateDialog::setCheckStateForAll(Qt::CheckState checkState)
{
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)  {
        ui->tableWidget->item(row, 0)->setCheckState(checkState);
    }
}

//QList<AndroidProFilesUpdateDialog::UpdateSetting>
//AndroidProFilesUpdateDialog::getUpdateSettings() const
//{
//    QList<UpdateSetting> settings;
//    for (int row = 0; row < m_models.count(); ++row) {
//        const bool doUpdate = result() != Rejected
//            && ui->tableWidget->item(row, 0)->checkState() == Qt::Checked;
//        settings << UpdateSetting(m_models.at(row), doUpdate);
//    }
//    return settings;
//}

} // namespace Qt4ProjectManager
} // namespace Internal
