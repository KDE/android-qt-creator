/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef BRANCHDIALOG_H
#define BRANCHDIALOG_H

#include <QtGui/QDialog>
#include <QtGui/QItemSelection>

QT_BEGIN_NAMESPACE
class QPushButton;
class QModelIndex;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

namespace Ui {
class BranchDialog;
}

class GitClient;
class LocalBranchModel;
class RemoteBranchModel;

/**
 * Branch dialog. Displays a list of local branches at the top and remote
 * branches below. Offers to checkout/delete local branches.
 *
 */
class BranchDialog : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(BranchDialog)
public:
    explicit BranchDialog(QWidget *parent = 0);
    virtual ~BranchDialog();

public slots:
    void refresh(const QString &repository, bool force);

private slots:
    void slotEnableButtons(const QItemSelection &selected = QItemSelection());
    void slotCheckoutSelectedBranch();
    void slotDeleteSelectedBranch();
    void slotDiffSelected();
    void slotLog();
    void slotRefresh();
    void slotLocalBranchActivated();
    void slotRemoteBranchActivated(const QModelIndex &);
    void slotCreateLocalBranch(const QString &branchName);

protected:
    virtual void changeEvent(QEvent *e);

private:
    bool ask(const QString &title, const QString &what, bool defaultButton);
    void selectLocalBranch(const QString &b);

    int selectedLocalBranchIndex() const;
    int selectedRemoteBranchIndex() const;

    Ui::BranchDialog *m_ui;
    QPushButton *m_checkoutButton;
    QPushButton *m_diffButton;
    QPushButton *m_logButton;
    QPushButton *m_refreshButton;
    QPushButton *m_deleteButton;

    LocalBranchModel *m_localModel;
    RemoteBranchModel *m_remoteModel;
    QString m_repository;
};

} // namespace Internal
} // namespace Git

#endif // BRANCHDIALOG_H
