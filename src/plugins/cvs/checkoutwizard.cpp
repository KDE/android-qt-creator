/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "checkoutwizard.h"
#include "checkoutwizardpage.h"
#include "cvsplugin.h"

#include <coreplugin/iversioncontrol.h>
#include <vcsbase/checkoutjobs.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsconfigurationpage.h>
#include <utils/qtcassert.h>

#include <QIcon>

namespace Cvs {
namespace Internal {

CheckoutWizard::CheckoutWizard(QObject *parent) :
        VcsBase::BaseCheckoutWizard(parent)
{
    setId(QLatin1String(VcsBase::Constants::VCS_ID_CVS));
}

QIcon CheckoutWizard::icon() const
{
    return QIcon(QLatin1String(":/cvs/images/cvs.png"));
}

QString CheckoutWizard::description() const
{
    return tr("Checks out a CVS repository and tries to load the contained project.");
}

QString CheckoutWizard::displayName() const
{
    return tr("CVS Checkout");
}

QList<QWizardPage*> CheckoutWizard::createParameterPages(const QString &path)
{
    QList<QWizardPage*> rc;
    const Core::IVersionControl *vc = CvsPlugin::instance()->versionControl();
    if (!vc->isConfigured())
        rc.append(new VcsBase::VcsConfigurationPage(vc));
    CheckoutWizardPage *cwp = new CheckoutWizardPage;
    cwp->setPath(path);
    rc.push_back(cwp);
    return rc;
}

QSharedPointer<VcsBase::AbstractCheckoutJob> CheckoutWizard::createJob(const QList<QWizardPage*> &parameterPages,
                                                                    QString *checkoutPath)
{
    // Collect parameters for the checkout command.
    // CVS does not allow for checking out into a different directory.
    const CheckoutWizardPage *cwp = qobject_cast<const CheckoutWizardPage *>(parameterPages.front());
    QTC_ASSERT(cwp, return QSharedPointer<VcsBase::AbstractCheckoutJob>());
    const CvsSettings settings = CvsPlugin::instance()->settings();
    const QString binary = settings.cvsCommand;
    QStringList args;
    const QString repository = cwp->repository();
    args << QLatin1String("checkout") << repository;
    const QString workingDirectory = cwp->path();
    *checkoutPath = workingDirectory + QLatin1Char('/') + repository;

    VcsBase::ProcessCheckoutJob *job = new VcsBase::ProcessCheckoutJob;
    job->addStep(binary, settings.addOptions(args), workingDirectory);
    return QSharedPointer<VcsBase::AbstractCheckoutJob>(job);
}

} // namespace Internal
} // namespace Cvs
