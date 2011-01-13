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

#include "saveitemsdialog.h"
#include "mainwindow.h"
#include "vcsmanager.h"

#include <coreplugin/ifile.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QPushButton>
#include <QtGui/QTreeWidget>
#include <QtGui/QHeaderView>
#include <QtGui/QCheckBox>

Q_DECLARE_METATYPE(Core::IFile*)

using namespace Core;
using namespace Core::Internal;

SaveItemsDialog::SaveItemsDialog(QWidget *parent,
                                 QList<IFile *> items)
    : QDialog(parent)
{
    m_ui.setupUi(this);
    QPushButton *discardButton = m_ui.buttonBox->addButton(tr("Do not Save"), QDialogButtonBox::DestructiveRole);
    m_ui.buttonBox->button(QDialogButtonBox::Save)->setDefault(true);
    m_ui.buttonBox->button(QDialogButtonBox::Save)->setFocus(Qt::TabFocusReason);
    m_ui.buttonBox->button(QDialogButtonBox::Save)->setMinimumWidth(130); // bad magic number to avoid resizing of button

    m_ui.saveBeforeBuildCheckBox->setVisible(false);

    foreach (IFile *file, items) {
        QString visibleName;
        QString directory;
        QString fileName = file->fileName();
        if (fileName.isEmpty()) {
            visibleName = file->suggestedFileName();
        } else {
            QFileInfo info = QFileInfo(fileName);
            directory = info.absolutePath();
            visibleName = info.fileName();
        }
        QTreeWidgetItem *item = new QTreeWidgetItem(m_ui.treeWidget, QStringList()
                                                    << visibleName << QDir::toNativeSeparators(directory));
        item->setData(0, Qt::UserRole, qVariantFromValue(file));
    }

    m_ui.treeWidget->resizeColumnToContents(0);
    m_ui.treeWidget->selectAll();
    updateSaveButton();

    connect(m_ui.buttonBox->button(QDialogButtonBox::Save), SIGNAL(clicked()),
            this, SLOT(collectItemsToSave()));
    connect(discardButton, SIGNAL(clicked()), this, SLOT(discardAll()));
    connect(m_ui.treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(updateSaveButton()));
}

void SaveItemsDialog::setMessage(const QString &msg)
{
    m_ui.msgLabel->setText(msg);
}

void SaveItemsDialog::updateSaveButton()
{
    int count = m_ui.treeWidget->selectedItems().count();
    QPushButton *button = m_ui.buttonBox->button(QDialogButtonBox::Save);
    if (count == m_ui.treeWidget->topLevelItemCount()) {
        button->setEnabled(true);
        button->setText(tr("Save All"));
    } else if (count == 0) {
        button->setEnabled(false);
        button->setText(tr("Save"));
    } else {
        button->setEnabled(true);
        button->setText(tr("Save Selected"));
    }
}

void SaveItemsDialog::collectItemsToSave()
{
    m_itemsToSave.clear();
    foreach (QTreeWidgetItem *item, m_ui.treeWidget->selectedItems()) {
        m_itemsToSave.append(item->data(0, Qt::UserRole).value<IFile*>());
    }
    accept();
}

void SaveItemsDialog::discardAll()
{
    m_ui.treeWidget->clearSelection();
    collectItemsToSave();
}

QList<IFile*> SaveItemsDialog::itemsToSave() const
{
    return m_itemsToSave;
}

void SaveItemsDialog::setAlwaysSaveMessage(const QString &msg)
{
    m_ui.saveBeforeBuildCheckBox->setText(msg);
    m_ui.saveBeforeBuildCheckBox->setVisible(true);
}

bool SaveItemsDialog::alwaysSaveChecked()
{
    return m_ui.saveBeforeBuildCheckBox->isChecked();
}
