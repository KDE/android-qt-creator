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

#ifndef GITORIOUSPROJECTWIDGET_H
#define GITORIOUSPROJECTWIDGET_H

#include <QtCore/QSharedPointer>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
class QStandardItem;
class QModelIndex;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Gitorious {
namespace Internal {

class GitoriousHostWizardPage;
struct GitoriousProject;

namespace Ui {
    class GitoriousProjectWidget;
}

/* Let the user select a project from a host. Displays name and description
 * with tooltip and info button that opens URLs contained in the description.
 * Connects to the signals of Gitorious and updates the project list as the
 * it receives the projects. isValid() and signal validChanged are
 * provided for use in a QWizardPage. Host matching happens via name as the
 * hostIndex might change due to deleting hosts. */
class GitoriousProjectWidget : public QWidget {
    Q_OBJECT
public:
    explicit GitoriousProjectWidget(int hostIndex,
                                    QWidget *parent = 0);
    ~GitoriousProjectWidget();

    virtual bool isValid() const;

    QSharedPointer<GitoriousProject> project() const;

    QString hostName() const { return m_hostName; }
    int hostIndex() const;

    // Utility to set description column and tooltip for a row from a free
    // format/HTMLish gitorious description. Make sure the description is
    // just one row for the item and set a tooltip with full contents.
    // If desired, extract an URL.
    static void setDescription(const QString &description,
                               int descriptionColumn,
                               QList<QStandardItem *> *items,
                               QString *url = 0);

signals:
    void validChanged();

public slots:
    void grabFocus();

private slots:
    void slotCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
    void slotInfo();
    void slotUpdateProjects(int hostIndex);
    void slotUpdateCheckBoxChanged(int);

protected:
    void changeEvent(QEvent *e);

private:
    QStandardItem *itemFromIndex(const QModelIndex &idx) const;
    QStandardItem *currentItem() const;

    const QString m_hostName;

    Ui::GitoriousProjectWidget *ui;
    const GitoriousHostWizardPage *m_hostPage;
    QStandardItemModel *m_model;
    QSortFilterProxyModel *m_filterModel;
    bool m_valid;
};


} // namespace Internal
} // namespace Gitorious
#endif // GITORIOUSPROJECTWIDGET_H
