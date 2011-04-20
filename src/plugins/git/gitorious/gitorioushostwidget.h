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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef GITORIOUSHOSTWIDGET_H
#define GITORIOUSHOSTWIDGET_H

#include <QtGui/QWizardPage>
#include <QtGui/QStandardItemModel>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
class QStandardItem;
class QModelIndex;
class QTimer;

QT_END_NAMESPACE

namespace Gitorious {
namespace Internal {

namespace Ui {
    class GitoriousHostWidget;
}

/* A page listing gitorious hosts with browse/add options. isValid() and the
 * related change signals are provided for use within a QWizardPage.
 * Connects to the signals of Gitorious and updates the project count as the
 * it receives the projects. As soon as there are projects, isValid() becomes
 * true. */

class GitoriousHostWidget : public QWidget {
    Q_OBJECT
public:
    GitoriousHostWidget(QWidget *parent = 0);
    ~GitoriousHostWidget();

    // Has a host selected that has projects.
    bool isValid() const;
    int selectedRow() const;
    // hosts modified?
    bool isHostListDirty() const;

signals:
    void validChanged();

public slots:
    void selectRow(int);

protected:
    void changeEvent(QEvent *e);

private slots:
    void slotBrowse();
    void slotDelete();
    void slotCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
    void slotItemEdited(QStandardItem *item);
    void slotProjectListPageReceived(int row);
    void slotClearError();
    void slotError(const QString &e);

private:
    void appendNewDummyEntry();
    void checkValid(const QModelIndex &current);
    QStandardItem *currentItem() const;

    const QString m_newHost;

    Ui::GitoriousHostWidget *ui;
    QStandardItemModel *m_model;
    QTimer *m_errorClearTimer;
    bool m_isValid;
    bool m_isHostListDirty;
};

} // namespace Internal
} // namespace Gitorious
#endif // GITORIOUSHOSTWIDGET_H
