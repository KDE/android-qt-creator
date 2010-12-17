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

#ifndef GITORIOUSPROJECTWIZARDPAGE_H
#define GITORIOUSPROJECTWIZARDPAGE_H

#include <QtCore/QSharedPointer>
#include <QtGui/QWizardPage>

QT_BEGIN_NAMESPACE
class QStackedWidget;
QT_END_NAMESPACE

namespace Gitorious {
namespace Internal {

class GitoriousHostWizardPage;
struct GitoriousProject;
class GitoriousProjectWidget;

namespace Ui {
    class GitoriousProjectWizardPage;
}

/* GitoriousProjectWizardPage: Let the user select a project via
 * GitoriousProjectWidget. As switching back and forth hosts (repopulating
 * the sorting projects model/treeviews) might get slow when the host has
 * lots of projects, it manages a stack of project widgets and activates
 * the one selected in the host page (or creates a new one) in
 * initializePage.  */

class GitoriousProjectWizardPage : public QWizardPage {
    Q_OBJECT
public:
    explicit GitoriousProjectWizardPage(const GitoriousHostWizardPage *hostPage,
                                        QWidget *parent = 0);

    virtual void initializePage();
    virtual bool isComplete() const;

    QSharedPointer<GitoriousProject> project() const;
    int selectedHostIndex() const;
    QString selectedHostName() const;

private slots:
    void slotCheckValid();

private:
    GitoriousProjectWidget *projectWidgetAt(int index) const;
    GitoriousProjectWidget *currentProjectWidget() const;
    int stackIndexOf(const QString &hostName) const;

    const GitoriousHostWizardPage *m_hostPage;
    QStackedWidget *m_stackedWidget;
    bool m_isValid;

};

} // namespace Internal
} // namespace Gitorious
#endif // GITORIOUSPROJECTWIZARDPAGE_H
