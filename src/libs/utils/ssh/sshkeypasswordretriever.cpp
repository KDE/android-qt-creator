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
#include "sshkeypasswordretriever_p.h"

#include <QtCore/QString>
#include <QtGui/QApplication>
#include <QtGui/QInputDialog>

#include <iostream>

namespace Utils {
namespace Internal {

std::string SshKeyPasswordRetriever::get_passphrase(const std::string &, const std::string &,
    UI_Result &result) const
{
    const bool hasGui = dynamic_cast<QApplication *>(QApplication::instance());
    if (hasGui) {
        bool ok;
        const QString &password = QInputDialog::getText(0,
            QCoreApplication::translate("Utils::Ssh", "Password Required"),
            QCoreApplication::translate("Utils::Ssh", "Please enter the password for your private key."),
            QLineEdit::Password, QString(), &ok);
        result = ok ? OK : CANCEL_ACTION;
        return std::string(password.toLocal8Bit().data());
    } else {
        result = OK;
        std::string password;
        std::cout << "Please enter the password for your private key (set echo off beforehand!): " << std::flush;
        std::cin >> password;
        return password;
    }
}

} // namespace Internal
} // namespace Utils
