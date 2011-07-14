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

#include "checkoutwizardpage.h"

namespace Subversion {
namespace Internal {

CheckoutWizardPage::CheckoutWizardPage(QWidget *parent) :
    VCSBase::BaseCheckoutWizardPage(parent)
{
    setTitle(tr("Location"));
    setSubTitle(tr("Specify repository URL, checkout directory and path."));
    setRepositoryLabel(tr("Repository:"));
    setBranchSelectorVisible(false);
}

QString CheckoutWizardPage::directoryFromRepository(const QString &repoIn) const
{
    /* Try to figure out a good directory name from something like:
     * "svn://<server>/path1/project" -> project */

    QString repo = repoIn.trimmed();
    const QChar slash = QLatin1Char('/');
    // remove host
    const int slashPos = repo.lastIndexOf(slash);
    if (slashPos != -1)
        repo.remove(0, slashPos + 1);
    // fix invalid characters
    const QChar dash = QLatin1Char('-');
    repo.replace(QLatin1Char('.'), QLatin1Char('-'));
    return repo;
}

} // namespace Internal
} // namespace Subversion
