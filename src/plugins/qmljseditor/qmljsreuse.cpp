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

#include "qmljsreuse.h"

#include <QtCore/QChar>
#include <QtGui/QPainter>

namespace QmlJSEditor {
namespace Internal {

bool isIdentifierChar(const QChar &c, bool atStart, bool acceptDollar)
{
    switch (c.unicode()) {
    case '_':
        return true;
    case '$':
        if (acceptDollar)
            return true;
        return false;

    default:
        if (atStart)
            return c.isLetter();
        else
            return c.isLetterOrNumber();
    }
}

bool isDelimiterChar(const QChar &c)
{
    switch (c.unicode()) {
    case '{':
    case '}':
    case '[':
    case ']':
    case ')':
    case '?':
    case '!':
    case ':':
    case ';':
    case ',':
    case '+':
    case '-':
    case '*':
    case '/':
        return true;

    default:
        return false;
    }
}

bool isActivationChar(const QChar &c)
{
    if (c == QLatin1Char('(') || c == QLatin1Char('.') || c == QLatin1Char('/'))
        return true;
    return false;
}

QIcon iconForColor(const QColor &color)
{
    QPixmap pix(6, 6);

    int pixSize = 20;
    QBrush br(color);

    QPixmap pm(2 * pixSize, 2 * pixSize);
    QPainter pmp(&pm);
    pmp.fillRect(0, 0, pixSize, pixSize, Qt::lightGray);
    pmp.fillRect(pixSize, pixSize, pixSize, pixSize, Qt::lightGray);
    pmp.fillRect(0, pixSize, pixSize, pixSize, Qt::darkGray);
    pmp.fillRect(pixSize, 0, pixSize, pixSize, Qt::darkGray);
    pmp.fillRect(0, 0, 2 * pixSize, 2 * pixSize, color);
    br = QBrush(pm);

    QPainter p(&pix);
    int corr = 1;
    QRect r = pix.rect().adjusted(corr, corr, -corr, -corr);
    p.setBrushOrigin((r.width() % pixSize + pixSize) / 2 + corr, (r.height() % pixSize + pixSize) / 2 + corr);
    p.fillRect(r, br);

    p.fillRect(r.width() / 4 + corr, r.height() / 4 + corr,
               r.width() / 2, r.height() / 2,
               QColor(color.rgb()));
    p.drawRect(pix.rect().adjusted(0, 0, -1, -1));

    return pix;
}


} // Internal
} // QmlJSEditor
