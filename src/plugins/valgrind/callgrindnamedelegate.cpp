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

#include "callgrindnamedelegate.h"

#include "callgrindhelper.h"

#include <QtGui/QApplication>
#include <QtGui/QPainter>

namespace Valgrind {
namespace Internal {

NameDelegate::NameDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{

}

NameDelegate::~NameDelegate()
{
}

void NameDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    // init
    QStyleOptionViewItemV4 opt(option);
    initStyleOption(&opt, index);

    const int margin = 2;
    const int size = 10;

    const QString text = index.data().toString();
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

    // draw controls, but no text
    opt.text.clear();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter);

    // draw bar in the first few pixels
    painter->save();
    const QRectF barRect = opt.rect.adjusted(
            margin, margin, -opt.rect.width() + size - margin, -margin);
    painter->setPen(Qt::black);
    painter->setBrush(CallgrindHelper::colorForString(text));
    painter->drawRect(barRect);

    // move cell rect to right
    opt.rect.adjust(size+margin, 0, 0, 0);

    // draw text
    const QString elidedText = painter->fontMetrics().elidedText(text, Qt::ElideRight,
                                                                 opt.rect.width());

    const QBrush &textBrush = (option.state & QStyle::State_Selected)
                                ? opt.palette.highlightedText()
                                : opt.palette.text();
    painter->setBrush(Qt::NoBrush);
    painter->setPen(textBrush.color());
    painter->drawText(opt.rect, elidedText);
    painter->restore();
}

} // namespace Internal
} // namespace Valgrind
