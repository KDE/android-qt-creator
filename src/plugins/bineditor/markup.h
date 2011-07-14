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

#ifndef MARKUP_H
#define MARKUP_H

#include <QtGui/QColor>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QMetaType>

namespace BINEditor {
/*!
    \class BINEditor::Markup
    \brief Markup range of the binary editor.

    Used for displaying class layouts by the debugger.

    \note Must not have linkage - used for soft dependencies.

    \sa Debugger::Internal::MemoryAgent
*/

class Markup
{
public:
    Markup(quint64 a = 0, quint64 l = 0, QColor c = Qt::yellow, const QString &tt = QString()) :
        address(a), length(l), color(c), toolTip(tt) {}
    bool covers(quint64 a) const { return a >= address && a < (address + length); }

    quint64 address;
    quint64 length;
    QColor color;
    QString toolTip;
};
} // namespace BINEditor

Q_DECLARE_METATYPE(QList<BINEditor::Markup>)

#endif // MARKUP_H
