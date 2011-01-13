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

#include "gitoriousprojectwizardpage.h"
#include "gitoriousprojectwidget.h"
#include "gitorioushostwizardpage.h"
#include "gitorious.h"

#include <utils/qtcassert.h>

#include <QtGui/QStackedWidget>
#include <QtGui/QVBoxLayout>

namespace Gitorious {
namespace Internal {

GitoriousProjectWizardPage::GitoriousProjectWizardPage(const GitoriousHostWizardPage *hostPage,
                                                       QWidget *parent) :
    QWizardPage(parent),
    m_hostPage(hostPage),
    m_stackedWidget(new QStackedWidget),
    m_isValid(false)
{
    QVBoxLayout *lt = new QVBoxLayout;
    lt->addWidget(m_stackedWidget);
    setLayout(lt);
    setTitle(tr("Project"));
}

static inline QString msgChooseProject(const QString &h)
{
    return GitoriousProjectWizardPage::tr("Choose a project from '%1'").arg((h));
}

QString GitoriousProjectWizardPage::selectedHostName() const
{
    if (const GitoriousProjectWidget *w = currentProjectWidget())
        return w->hostName();
    return QString();
}

void GitoriousProjectWizardPage::initializePage()
{
    // Try to find the page by hostindex
    const int hostIndex = m_hostPage->selectedHostIndex();
    const QString hostName = Gitorious::instance().hostName(hostIndex);
    const int existingStackIndex = stackIndexOf(hostName);
    // Found? - pop up that page
    if (existingStackIndex != -1) {
        m_stackedWidget->setCurrentIndex(existingStackIndex);
        setSubTitle(msgChooseProject(hostName));
        return;
    }
    // Add a new page
    GitoriousProjectWidget *widget = new GitoriousProjectWidget(hostIndex);
    connect(widget, SIGNAL(validChanged()), this, SLOT(slotCheckValid()));
    m_stackedWidget->addWidget(widget);
    m_stackedWidget->setCurrentIndex(m_stackedWidget->count() - 1);
    setSubTitle(msgChooseProject(widget->hostName()));
    slotCheckValid();
}

bool GitoriousProjectWizardPage::isComplete() const
{
    return m_isValid;
}

void GitoriousProjectWizardPage::slotCheckValid()
{
    const GitoriousProjectWidget *w = currentProjectWidget();
    const bool isValid = w ? w->isValid() : false;
    if (isValid != m_isValid) {
        m_isValid = isValid;
        emit completeChanged();
    }
}

QSharedPointer<GitoriousProject> GitoriousProjectWizardPage::project() const
{
    if (const GitoriousProjectWidget *w = currentProjectWidget())
        return w->project();
    return QSharedPointer<GitoriousProject>();
}

GitoriousProjectWidget *GitoriousProjectWizardPage::projectWidgetAt(int index) const
{
    return qobject_cast<GitoriousProjectWidget *>(m_stackedWidget->widget(index));
}

GitoriousProjectWidget *GitoriousProjectWizardPage::currentProjectWidget() const
{
    const int index = m_stackedWidget->currentIndex();
    if (index < 0)
        return 0;
    return projectWidgetAt(index);
}

// Convert a host name to a stack index.
int GitoriousProjectWizardPage::stackIndexOf(const QString &hostName) const
{
    const int count = m_stackedWidget->count();
    for(int i = 0; i < count; i++)
        if (projectWidgetAt(i)->hostName() == hostName)
            return i;
    return -1;
}

} // namespace Internal
} // namespace Gitorious
