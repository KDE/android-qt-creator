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

#include "gitoriousclonewizard.h"
#include "gitorioushostwizardpage.h"
#include "gitoriousprojectwizardpage.h"
#include "gitoriousrepositorywizardpage.h"
#include "clonewizardpage.h"

#include <vcsbase/checkoutjobs.h>
#include <vcsbase/vcsbaseconstants.h>
#include <utils/qtcassert.h>

#include <QtCore/QUrl>
#include <QtGui/QIcon>

namespace Gitorious {
namespace Internal {

//  GitoriousCloneWizardPage: A git clone page taking its URL from the
//  projects page.

class GitoriousCloneWizardPage : public Git::CloneWizardPage {
public:
    explicit GitoriousCloneWizardPage(const GitoriousRepositoryWizardPage *rp, QWidget *parent = 0);
    virtual void initializePage();

private:
    const GitoriousRepositoryWizardPage *m_repositoryPage;
};

GitoriousCloneWizardPage::GitoriousCloneWizardPage(const GitoriousRepositoryWizardPage *rp, QWidget *parent) :
    Git::CloneWizardPage(parent),
    m_repositoryPage(rp)
{
}

void GitoriousCloneWizardPage::initializePage()
{
    setRepository(m_repositoryPage->repositoryURL().toString());
}

// -------- GitoriousCloneWizard
GitoriousCloneWizard::GitoriousCloneWizard(QObject *parent) :
        VCSBase::BaseCheckoutWizard(parent)
{
    setId(QLatin1String(VCSBase::Constants::VCS_ID_GIT));
}

QIcon GitoriousCloneWizard::icon() const
{
    return QIcon(QLatin1String(":/git/images/gitorious.png"));
}

QString GitoriousCloneWizard::description() const
{
    return tr("Clones a Gitorious repository and tries to load the contained project.");
}

QString GitoriousCloneWizard::displayName() const
{
    return tr("Gitorious Repository Clone");
}

QList<QWizardPage*> GitoriousCloneWizard::createParameterPages(const QString &path)
{
    GitoriousHostWizardPage *hostPage = new GitoriousHostWizardPage;
    GitoriousProjectWizardPage *projectPage = new GitoriousProjectWizardPage(hostPage);
    GitoriousRepositoryWizardPage *repoPage = new GitoriousRepositoryWizardPage(projectPage);
    GitoriousCloneWizardPage *clonePage = new GitoriousCloneWizardPage(repoPage);
    clonePage->setPath(path);

    QList<QWizardPage*> rc;
    rc << hostPage << projectPage << repoPage << clonePage;
    return rc;
}

QSharedPointer<VCSBase::AbstractCheckoutJob> GitoriousCloneWizard::createJob(const QList<QWizardPage*> &parameterPages,
                                                                    QString *checkoutPath)
{
    const Git::CloneWizardPage *cwp = qobject_cast<const Git::CloneWizardPage *>(parameterPages.back());
    QTC_ASSERT(cwp, return QSharedPointer<VCSBase::AbstractCheckoutJob>())
    return cwp->createCheckoutJob(checkoutPath);
}

} // namespace Internal
} // namespace Gitorius
