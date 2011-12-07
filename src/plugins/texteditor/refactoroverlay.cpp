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

#include "refactoroverlay.h"
#include "basetextdocumentlayout.h"
#include "basetexteditor.h"

#include <QtGui/QPainter>
#include <QtGui/QTextBlock>

#include <QtCore/QDebug>

namespace TextEditor {

RefactorOverlay::RefactorOverlay(TextEditor::BaseTextEditorWidget *editor) :
    QObject(editor),
    m_editor(editor),
    m_maxWidth(0),
    m_icon(QLatin1String(":/texteditor/images/refactormarker.png"))
{
}

void RefactorOverlay::paint(QPainter *painter, const QRect &clip)
{
    m_maxWidth = 0;
    for (int i = 0; i < m_markers.size(); ++i) {
        paintMarker(m_markers.at(i), painter, clip);
    }

    if (BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(m_editor->document()->documentLayout())) {
        documentLayout->setRequiredWidth(m_maxWidth);
    }

}

RefactorMarker RefactorOverlay::markerAt(const QPoint &pos) const
{
    QPointF offset = m_editor->contentOffset();
    foreach(const RefactorMarker &marker, m_markers) {
        if (marker.rect.translated(offset.toPoint()).contains(pos))
            return marker;
    }
    return RefactorMarker();
}

void RefactorOverlay::paintMarker(const RefactorMarker& marker, QPainter *painter, const QRect &clip)
{
    QPointF offset = m_editor->contentOffset();
    QRectF geometry = m_editor->blockBoundingGeometry(marker.cursor.block()).translated(offset);

    if (geometry.top() > clip.bottom() + 10 || geometry.bottom() < clip.top() - 10)
        return; // marker not visible

    QTextCursor cursor = marker.cursor;

    QRect r = m_editor->cursorRect(cursor);

    QIcon icon = marker.icon;
    if (icon.isNull())
        icon = m_icon;

    QSize sz = icon.actualSize(QSize(m_editor->fontMetrics().width(QLatin1Char(' '))+2, r.height()));

    int x = r.right();
    marker.rect = QRect(x, r.top(), sz.width(), sz.height());

    icon.paint(painter, marker.rect);
    m_maxWidth = qMax((qreal)m_maxWidth, x + sz.width() - offset.x());
}

} // namespace TextEditor
