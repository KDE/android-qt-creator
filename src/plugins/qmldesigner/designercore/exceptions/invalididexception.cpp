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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include <QCoreApplication>
#include "invalididexception.h"

namespace QmlDesigner {

InvalidIdException::InvalidIdException(int line,
                                       const QString &function,
                                       const QString &file,
                                       const QString &id,
                                       Reason reason) :
    InvalidArgumentException(line, function, file, "id"),
    m_id(id)
{
    if (reason == InvalidCharacters) {
        m_description = QCoreApplication::translate("InvalidIdException", "Only alphanumeric characters and underscore allowed.\nIds must begin with a lowercase letter.");
    } else {
        m_description = QCoreApplication::translate("InvalidIdException", "Ids have to be unique.");
    }
}

InvalidIdException::InvalidIdException(int line,
                                       const QString &function,
                                       const QString &file,
                                       const QString &id,
                                       const QString &description) :
    InvalidArgumentException(line, function, file, "id"),
    m_id(id),
    m_description(description)
{
}

QString InvalidIdException::type() const
{
    return "InvalidIdException";
}

QString InvalidIdException::description() const
{
    return QCoreApplication::translate("InvalidIdException", "Invalid Id: %1\n%2").arg(m_id, m_description);
}

}
