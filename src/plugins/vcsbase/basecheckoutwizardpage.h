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

#ifndef VCSBASE_CHECKOUTWIZARDPAGE_H
#define VCSBASE_CHECKOUTWIZARDPAGE_H

#include "vcsbase_global.h"

#include <QtGui/QWizardPage>

namespace VCSBase {

namespace Internal {
namespace Ui {
class BaseCheckoutWizardPage;
} // namespace Ui
} // namespace Internal

struct BaseCheckoutWizardPagePrivate;

class VCSBASE_EXPORT BaseCheckoutWizardPage : public QWizardPage
{
    Q_OBJECT
    Q_PROPERTY(bool isBranchSelectorVisible READ isBranchSelectorVisible
        WRITE setBranchSelectorVisible)

public:
    BaseCheckoutWizardPage(QWidget *parent = 0);
    ~BaseCheckoutWizardPage();

    QString path() const;
    void setPath(const QString &);

    QString directory() const;
    void setDirectory(const QString &d);

    QString repository() const;
    void setRepository(const QString &r);

    bool isRepositoryReadOnly() const;
    void setRepositoryReadOnly(bool v);

    QString branch() const;
    void setBranch(const QString &);

    virtual bool isComplete() const;

    bool isBranchSelectorVisible() const;

protected:
    void changeEvent(QEvent *e);

    void setRepositoryLabel(const QString &l);
    void setDirectoryVisible(bool v);
    void setBranchSelectorVisible(bool v);

    // Determine a checkout directory name from
    // repository URL, that is, "protocol:/project" -> "project".
    virtual QString directoryFromRepository(const QString &r) const;

    // Return list of branches of that repository, defaults to empty.
    virtual QStringList branches(const QString &repository, int *current);

    // Add additional controls.
    void addLocalControl(QWidget *w);
    void addLocalControl(QString &description, QWidget *w);

    void addRepositoryControl(QWidget *w);
    void addRepositoryControl(QString &description, QWidget *w);

    // Override validity information.
    virtual bool checkIsValid() const;

private slots:
    void slotRepositoryChanged(const QString &url);
    void slotDirectoryEdited();
    void slotChanged();
    void slotRefreshBranches();

private:
    BaseCheckoutWizardPagePrivate *d;
};

} // namespace VCSBase

#endif // VCSBASE_CHECKOUTWIZARDPAGE_H
