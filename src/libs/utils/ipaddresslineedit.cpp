/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "ipaddresslineedit.h"

#include <QtGui/QRegExpValidator>

/*!
  \class Utils::IpAddressLineEdit

  \brief A QLineEdit widget that validates the IP address inserted.

  The valid address example is 192.168.1.12 or 192.168.1.12:8080.
*/

namespace Utils {

// ------------------ IpAddressLineEditPrivate

class IpAddressLineEditPrivate
{
public:
    IpAddressLineEditPrivate();

    QValidator *m_ipAddressValidator;
    QColor m_validColor;
};

IpAddressLineEditPrivate::IpAddressLineEditPrivate()
{
}

IpAddressLineEdit::IpAddressLineEdit(QWidget* parent) :
    BaseValidatingLineEdit(parent),
    d(new IpAddressLineEditPrivate())
{
    const char * ipAddressRegExpPattern = "^\\b(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
            "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
            "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
            "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b"
            "((:)(6553[0-5]|655[0-2]\\d|65[0-4]\\d\\d|6[0-4]\\d{3}|[1-5]\\d{4}|[1-9]\\d{0,3}|0))?$";

    QRegExp ipAddressRegExp(ipAddressRegExpPattern);
    d->m_ipAddressValidator = new QRegExpValidator(ipAddressRegExp, this);
}

IpAddressLineEdit::~IpAddressLineEdit()
{
    delete d;
}

bool IpAddressLineEdit::validate(const QString &value, QString *errorMessage) const
{
    QString copy = value;
    int offset = 0;
    bool isValid = d->m_ipAddressValidator->validate(copy, offset) == QValidator::Acceptable;
    if (!isValid) {
        *errorMessage =  tr("The IP address is not valid.");
        return false;
    }
    return true;
}

void IpAddressLineEdit::slotChanged(const QString &t)
{
    Utils::BaseValidatingLineEdit::slotChanged(t);
    if (isValid())
        emit validAddressChanged(t);
    else
        emit invalidAddressChanged();
}

} // namespace Utils
