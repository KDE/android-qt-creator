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

#ifndef CALLGRINDHELPER_H
#define CALLGRINDHELPER_H

#include <QtCore/QLocale>

QT_BEGIN_NAMESPACE
class QColor;
class QString;
QT_END_NAMESPACE

namespace Valgrind {
namespace Internal {

namespace CallgrindHelper
{
    /**
     * Returns color for a specific string, the string<->color mapping is cached
     */
    QColor colorForString(const QString &text);

    /**
     * Returns color for a specific cost ratio
     * \param ratio The cost ratio, ratio should be of [0,1]
     */
    QColor colorForCostRatio(qreal ratio);

    /**
     * Returns a proper percent representation of a float limited to 5 chars
     */
    QString toPercent(float costs, const QLocale &locale = QLocale());
}

} // namespace Internal
} // namespace Valgrind

#endif // CALLGRINDHELPER_H
