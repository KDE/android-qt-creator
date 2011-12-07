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

#include "checkoutwizarddialog.h"
#include "basecheckoutwizard.h"
#include "checkoutjobs.h"
#include "checkoutprogresswizardpage.h"

#include <coreplugin/basefilewizard.h>

#include <QtGui/QPushButton>

/*!
    \class VCSBase::Internal::CheckoutWizardDialog

    Dialog used by \sa VCSBase::BaseCheckoutWizard. Overwrites reject() to first
    kill the checkout and then close.
 */

namespace VCSBase {
namespace Internal {

CheckoutWizardDialog::CheckoutWizardDialog(const QList<QWizardPage *> &parameterPages,
                                           QWidget *parent) :
    Utils::Wizard(parent),
    m_progressPage(new CheckoutProgressWizardPage),
    m_progressPageId(-1)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    foreach(QWizardPage *wp, parameterPages)
        addPage(wp);
    m_progressPageId = parameterPages.size();
    setPage(m_progressPageId, m_progressPage);
    connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(slotPageChanged(int)));
    connect(m_progressPage, SIGNAL(terminated(bool)), this, SLOT(slotTerminated(bool)));
    Core::BaseFileWizard::setupWizard(this);
}

void CheckoutWizardDialog::slotPageChanged(int id)
{
    if (id == m_progressPageId)
        emit progressPageShown();
}

void CheckoutWizardDialog::slotTerminated(bool success)
{
    // Allow to correct parameters
    if (!success)
        button(QWizard::BackButton)->setEnabled(true);
}

void CheckoutWizardDialog::start(const QSharedPointer<AbstractCheckoutJob> &job)
{
    // No "back" available while running.
    button(QWizard::BackButton)->setEnabled(false);
    m_progressPage->start(job);
}

void CheckoutWizardDialog::reject()
{
    // First click kills, 2nd closes
    if (currentId() == m_progressPageId && m_progressPage->isRunning()) {
        m_progressPage->terminate();
    } else {
        QWizard::reject();
    }
}

} // namespace Internal
} // namespace VCSBase
