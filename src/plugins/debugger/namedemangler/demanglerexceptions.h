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
#ifndef DEMANGLEREXCEPTIONS_H
#define DEMANGLEREXCEPTIONS_H

#include <QtGlobal>
#include <QString>

namespace Debugger {
namespace Internal {

class ParseTreeNode;

class ParseException
{
public:
    ParseException(const QString &error) : error(error) {}

    const QString error;
};

class InternalDemanglerException
{
public:
    InternalDemanglerException(const QString &func, const QString &file, int line)
            : func(func), file(file), line(line) {}

    QString func;
    QString file;
    int line;
};

#define DEMANGLER_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            throw InternalDemanglerException(Q_FUNC_INFO, __FILE__, __LINE__); \
        } \
    } while (0)

template <typename T> T *demanglerCast(ParseTreeNode *node, const QString &func,
        const QString &file, int line)
{
    T * const out = dynamic_cast<T *>(node);
    if (!out)
        throw InternalDemanglerException(func, file, line);
    return out;
}

#define DEMANGLER_CAST(type, input) demanglerCast<type>(input, QLatin1String(Q_FUNC_INFO), \
        QLatin1String(__FILE__), __LINE__)

} // namespace Internal
} // namespace Debugger

#endif // DEMANGLEREXCEPTIONS_H
