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

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <corelib_global.h>

#include <QString>
#include <QtDebug>

namespace QmlDesigner {

class CORESHARED_EXPORT Exception
{
public:
    Exception(int line,
              const QString &function,
              const QString &file);
    virtual ~Exception();

    virtual QString type() const=0;
    virtual QString description() const;

    int line() const;
    QString function() const;
    QString file() const;
    QString backTrace() const;

    static void setShouldAssert(bool assert);
    static bool shouldAssert();

private:
    int m_line;
    QString m_function;
    QString m_file;
    QString m_backTrace;
    static bool s_shouldAssert;
};

CORESHARED_EXPORT QDebug operator<<(QDebug debug, const Exception &exception);

}


#endif // EXCEPTION_H
