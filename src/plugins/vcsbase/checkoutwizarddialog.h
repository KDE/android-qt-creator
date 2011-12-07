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

#ifndef CHECKOUTWIZARDDIALOG_H
#define CHECKOUTWIZARDDIALOG_H

#include <QtCore/QSharedPointer>
#include <QtCore/QList>
#include <utils/wizard.h>

namespace VCSBase {
class AbstractCheckoutJob;

namespace Internal {
class CheckoutProgressWizardPage;

class CheckoutWizardDialog : public Utils::Wizard {
    Q_OBJECT
public:
    explicit CheckoutWizardDialog(const QList<QWizardPage *> &parameterPages,
                                  QWidget *parent = 0);

    void start(const QSharedPointer<AbstractCheckoutJob> &job);

signals:
    void progressPageShown();

private slots:
    void slotPageChanged(int id);
    void slotTerminated(bool success);
    virtual void reject();

private:
    CheckoutProgressWizardPage *m_progressPage;
    int m_progressPageId;
};

} // namespace Internal
} // namespace VCSBase
#endif // CHECKOUTWIZARDDIALOG_H
