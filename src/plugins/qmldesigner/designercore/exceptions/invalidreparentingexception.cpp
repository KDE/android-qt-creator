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

#include "invalidreparentingexception.h"
/*!
\class QmlDesigner::InvalidReparentingException
\ingroup CoreExceptions
\brief Exception for a invalid reparinting

\see ModelNode
*/
namespace QmlDesigner {
/*!
\brief Constructor

\param line use the __LINE__ macro
\param function use the __FUNCTION__ or the Q_FUNC_INFO macro
\param file use the __FILE__ macro
*/
InvalidReparentingException::InvalidReparentingException(int line,
                                                         const QString &function,
                                                         const QString &file)
 : Exception(line, function, file)
{
}

/*!
\brief Returns the type of this exception

\returns the type as a string
*/
QString InvalidReparentingException::type() const
{
    return "InvalidReparentingException";
}
} // namespace QmlDesigner
