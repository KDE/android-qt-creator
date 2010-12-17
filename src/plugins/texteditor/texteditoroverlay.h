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

#ifndef TEXTEDITOROVERLAY_H
#define TEXTEDITOROVERLAY_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QVector>
#include <QtGui/QTextCursor>
#include <QtGui/QColor>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace TextEditor {
class BaseTextEditor;

namespace Internal {

struct OverlaySelection
{
    OverlaySelection():m_fixedLength(-1), m_dropShadow(false){}

    QTextCursor m_cursor_begin;
    QTextCursor m_cursor_end;
    QColor m_fg;
    QColor m_bg;
    int m_fixedLength;
    bool m_dropShadow;
};

class TextEditorOverlay : public QObject
{
    Q_OBJECT
public:
    TextEditorOverlay(BaseTextEditor *editor);

    QRect rect() const;
    void paint(QPainter *painter, const QRect &clip);
    void fill(QPainter *painter, const QColor &color, const QRect &clip);

    bool isVisible() const { return m_visible; }
    void setVisible(bool b);

    inline void hide() { setVisible(false); }
    inline void show() { setVisible(true); }

    void setBorderWidth(int bw) {m_borderWidth = bw; }

    void update();

    void setAlpha(bool enabled) { m_alpha = enabled; }

    void clear();

    enum OverlaySelectionFlags {
        LockSize = 1,
        DropShadow = 2,
        ExpandBegin = 4
    };

    void addOverlaySelection(const QTextCursor &cursor, const QColor &fg, const QColor &bg,
                             uint overlaySelectionFlags = 0);
    void addOverlaySelection(int begin, int end, const QColor &fg, const QColor &bg,
                             uint overlaySelectionFlags = 0);

    const QList<OverlaySelection> &selections() const { return m_selections; }

    inline bool isEmpty() const { return m_selections.isEmpty(); }

    inline int dropShadowWidth() const { return m_dropShadowWidth; }

    bool hasCursorInSelection(const QTextCursor &cursor) const;

    void mapEquivalentSelections();
    void updateEquivalentSelections(const QTextCursor &cursor);

    bool hasFirstSelectionBeginMoved() const;

private:
    QPainterPath createSelectionPath(const QTextCursor &begin, const QTextCursor &end, const QRect& clip);
    void paintSelection(QPainter *painter, const OverlaySelection &selection);
    void fillSelection(QPainter *painter, const OverlaySelection &selection, const QColor &color);
    int selectionIndexForCursor(const QTextCursor &cursor) const;
    QString selectionText(int selectionIndex) const;
    QTextCursor assembleCursorForSelection(int selectionIndex) const;

    bool m_visible;
    int m_borderWidth;
    int m_dropShadowWidth;
    bool m_alpha;
    int m_firstSelectionOriginalBegin;
    BaseTextEditor *m_editor;
    QWidget *m_viewport;
    QList<OverlaySelection> m_selections;
    QVector<QList<int> > m_equivalentSelections;
};

} // namespace Internal
} // namespace TextEditor

#endif // TEXTEDITOROVERLAY_H
