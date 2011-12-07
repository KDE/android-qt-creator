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

#ifndef NEWDIALOG_H
#define NEWDIALOG_H

#include "iwizard.h"

#include <QtGui/QDialog>
#include <QtCore/QList>
#include <QtCore/QModelIndex>

QT_BEGIN_NAMESPACE
class QAbstractProxyModel;
class QPushButton;
class QStandardItem;
class QStandardItemModel;
class QStringList;
QT_END_NAMESPACE

namespace Core {

namespace Internal {

namespace Ui {
    class NewDialog;
}

class NewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewDialog(QWidget *parent);
    virtual ~NewDialog();

    void setWizards(QList<IWizard*> wizards);

    Core::IWizard *showDialog();

private slots:
    void currentCategoryChanged(const QModelIndex &);
    void currentItemChanged(const QModelIndex &);
    void okButtonClicked();
    void updateOkButton();
private:
    Core::IWizard *currentWizard() const;

    Ui::NewDialog *m_ui;
    QStandardItemModel *m_model;
    QAbstractProxyModel *m_proxyModel;
    QPushButton *m_okButton;
    QPixmap m_dummyIcon;
    QList<QStandardItem*> m_categoryItems;
};

} // namespace Internal
} // namespace Core

#endif // NEWDIALOG_H
