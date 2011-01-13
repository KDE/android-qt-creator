/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef BINEDITOR_H
#define BINEDITOR_H

#include <QtCore/QBasicTimer>
#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QStack>
#include <QtCore/QString>

#include <QtGui/QAbstractScrollArea>
#include <QtGui/QTextDocument>
#include <QtGui/QTextFormat>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Core {
class IEditor;
}

namespace TextEditor {
class FontSettings;
}

namespace BINEditor {

class BinEditor : public QAbstractScrollArea
{
    Q_OBJECT
    Q_PROPERTY(bool modified READ isModified WRITE setModified DESIGNABLE false)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE false)

public:
    BinEditor(QWidget *parent = 0);
    ~BinEditor();

    void setData(const QByteArray &data);
    QByteArray data() const;

    inline int dataSize() const { return m_size; }
    quint64 baseAddress() const { return m_baseAddr; }

    inline bool inLazyMode() const { return m_inLazyMode; }
    Q_INVOKABLE void setLazyData(quint64 startAddr, int range, int blockSize = 4096);
    inline int lazyDataBlockSize() const { return m_blockSize; }
    Q_INVOKABLE void addLazyData(quint64 block, const QByteArray &data);
    Q_INVOKABLE void setNewWindowRequestAllowed();
    Q_INVOKABLE void updateContents();
    bool save(const QString &oldFileName, const QString &newFileName);

    void zoomIn(int range = 1);
    void zoomOut(int range = 1);

    enum MoveMode {
        MoveAnchor,
        KeepAnchor
    };

    int cursorPosition() const;
    Q_INVOKABLE void setCursorPosition(int pos, MoveMode moveMode = MoveAnchor);
    void jumpToAddress(quint64 address);

    void setModified(bool);
    bool isModified() const;

    void setReadOnly(bool);
    bool isReadOnly() const;

    int find(const QByteArray &pattern, int from = 0,
             QTextDocument::FindFlags findFlags = 0);

    void selectAll();
    void clear();

    void undo();
    void redo();

    Core::IEditor *editorInterface() const { return m_ieditor; }
    void setEditorInterface(Core::IEditor *ieditor) { m_ieditor = ieditor; }

    bool hasSelection() const { return m_cursorPosition != m_anchorPosition; }
    int selectionStart() const { return qMin(m_anchorPosition, m_cursorPosition); }
    int selectionEnd() const { return qMax(m_anchorPosition, m_cursorPosition); }

    bool event(QEvent*);

    bool isUndoAvailable() const { return m_undoStack.size(); }
    bool isRedoAvailable() const { return m_redoStack.size(); }

    QString addressString(quint64 address);

    static const int SearchStride = 1024 * 1024;

public Q_SLOTS:
    void setFontSettings(const TextEditor::FontSettings &fs);
    void highlightSearchResults(const QByteArray &pattern,
        QTextDocument::FindFlags findFlags = 0);
    void copy(bool raw = false);

Q_SIGNALS:
    void modificationChanged(bool modified);
    void undoAvailable(bool);
    void redoAvailable(bool);
    void copyAvailable(bool);
    void cursorPositionChanged(int position);

    void lazyDataRequested(Core::IEditor *editor, quint64 block, bool synchronous);
    void newWindowRequested(quint64 address);
    void newRangeRequested(Core::IEditor *, quint64 address);
    void startOfFileRequested(Core::IEditor *);
    void endOfFileRequested(Core::IEditor *);

protected:
    void scrollContentsBy(int dx, int dy);
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *);
    void changeEvent(QEvent *);
    void wheelEvent(QWheelEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void focusInEvent(QFocusEvent *);
    void focusOutEvent(QFocusEvent *);
    void timerEvent(QTimerEvent *);
    void contextMenuEvent(QContextMenuEvent *event);

private:
    bool m_inLazyMode;
    QByteArray m_data;
    QMap<int, QByteArray> m_lazyData;
    QMap<int, QByteArray> m_oldLazyData;
    int m_blockSize;
    QMap<int, QByteArray> m_modifiedData;
    mutable QSet<int> m_lazyRequests;
    QByteArray m_emptyBlock;
    QByteArray m_lowerBlock;
    int m_size;

    int dataIndexOf(const QByteArray &pattern, int from, bool caseSensitive = true) const;
    int dataLastIndexOf(const QByteArray &pattern, int from, bool caseSensitive = true) const;

    bool requestDataAt(int pos, bool synchronous = false) const;
    bool requestOldDataAt(int pos) const;
    char dataAt(int pos, bool old = false) const;
    char oldDataAt(int pos) const;
    void changeDataAt(int pos, char c);
    QByteArray dataMid(int from, int length, bool old = false) const;
    QByteArray blockData(int block, bool old = false) const;

    QPoint offsetToPos(int offset);
    void asIntegers(int offset, int count, quint64 &beValue, quint64 &leValue,
        bool old = false);

    int m_unmodifiedState;
    int m_readOnly;
    int m_margin;
    int m_descent;
    int m_ascent;
    int m_lineHeight;
    int m_charWidth;
    int m_labelWidth;
    int m_textWidth;
    int m_columnWidth;
    int m_numLines;
    int m_numVisibleLines;

    quint64 m_baseAddr;

    bool m_cursorVisible;
    int m_cursorPosition;
    int m_anchorPosition;
    bool m_hexCursor;
    bool m_lowNibble;
    bool m_isMonospacedFont;

    QByteArray m_searchPattern;
    QByteArray m_searchPatternHex;
    bool m_caseSensitiveSearch;

    QBasicTimer m_cursorBlinkTimer;

    void init();
    int posAt(const QPoint &pos) const;
    bool inTextArea(const QPoint &pos) const;
    QRect cursorRect() const;
    void updateLines();
    void updateLines(int fromPosition, int toPosition);
    void ensureCursorVisible();
    void setBlinkingCursorEnabled(bool enable);

    void changeData(int position, uchar character, bool highNibble = false);

    int findPattern(const QByteArray &data, const QByteArray &dataHex,
        int from, int offset, int *match);
    void drawItems(QPainter *painter, int x, int y, const QString &itemString);
    void drawChanges(QPainter *painter, int x, int y, const char *changes);

    void setupJumpToMenuAction(QMenu *menu, QAction *actionHere, QAction *actionNew,
                               quint64 addr);

    struct BinEditorEditCommand {
        int position;
        uchar character;
        bool highNibble;
    };
    QStack<BinEditorEditCommand> m_undoStack, m_redoStack;

    QBasicTimer m_autoScrollTimer;
    Core::IEditor *m_ieditor;
    QString m_addressString;
    int m_addressBytes;
    bool m_canRequestNewWindow;
};

} // namespace BINEditor

#endif // BINEDITOR_H
