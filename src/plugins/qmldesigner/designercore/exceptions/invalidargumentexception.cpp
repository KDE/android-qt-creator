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

#include "invalidargumentexception.h"
#include <QString>
#include <QCoreApplication>
/*!
\class QmlDesigner::InvalidArgumentException
\ingroup CoreExceptions
\brief Exception for a invalid argument

*/
namespace QmlDesigner {

/*!
\brief Constructor

\param line use the __LINE__ macro
\param function use the __FUNCTION__ or the Q_FUNC_INFO macro
\param file use the __FILE__ macro
*/
InvalidArgumentException::InvalidArgumentException(int line,
                                                   const QString &function,
                                                   const QString &file,
                                                   const QString &argument)
 : Exception(line, function, file), m_argument(argument)
{

}

QString InvalidArgumentException::description() const
{
    if (function() == "createNode")
        return QCoreApplication::translate("QmlDesigner::InvalidArgumentException", "Failed to create item of type %1").arg(m_argument);

    return Exception::description();
}

/*!
\brief Returns the type of this exception

\returns the type as a string
*/
QString InvalidArgumentException::type() const
{
    return "InvalidArgumentException";
}

/*!
\brief Returns the argument of this exception

\returns the argument as a string
*/
QString InvalidArgumentException::argument() const
{
    return m_argument;
}

}
