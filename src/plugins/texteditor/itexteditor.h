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

#ifndef ITEXTEDITOR_H
#define ITEXTEDITOR_H

#include "texteditor_global.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QMenu;
class QTextBlock;
class QIcon;
class QRect;
class QPoint;
QT_END_NAMESPACE

namespace TextEditor {

class ITextEditor;

class TEXTEDITOR_EXPORT ITextMark : public QObject
{
    Q_OBJECT
public:
    ITextMark(QObject *parent = 0) : QObject(parent) {}
    virtual ~ITextMark() {}

    virtual QIcon icon() const = 0;

    virtual void updateLineNumber(int lineNumber) = 0;
    virtual void updateBlock(const QTextBlock &block) = 0;
    virtual void removedFromEditor() = 0;
    virtual void documentClosing() = 0;

    // determine order on markers on the same line.
    enum Priority
    {
        LowPriority,
        NormalPriority,
        HighPriority // shown on top.
    };

    virtual Priority priority() const = 0;
};

typedef QList<ITextMark *> TextMarks;


class TEXTEDITOR_EXPORT ITextMarkable : public QObject
{
    Q_OBJECT
public:
    ITextMarkable(QObject *parent = 0) : QObject(parent) {}
    virtual ~ITextMarkable() {}
    virtual bool addMark(ITextMark *mark, int line) = 0;

    virtual TextMarks marksAt(int line) const = 0;
    virtual void removeMark(ITextMark *mark) = 0;
    virtual bool hasMark(ITextMark *mark) const = 0;
    virtual void updateMark(ITextMark *mark) = 0;
};

class TEXTEDITOR_EXPORT ITextEditor : public Core::IEditor
{
    Q_OBJECT
public:
    enum PositionOperation {
        Current = 1,
        EndOfLine = 2,
        StartOfLine = 3,
        Anchor = 4,
        EndOfDoc = 5
    };

    ITextEditor() {}
    virtual ~ITextEditor() {}

    virtual int find(const QString &string) const = 0;

    virtual int position(PositionOperation posOp = Current, int at = -1) const = 0;
    virtual void convertPosition(int pos, int *line, int *column) const = 0;
    virtual QRect cursorRect(int pos = -1) const = 0;
    virtual int columnCount() const = 0;
    virtual int rowCount() const = 0;

    virtual QString contents() const = 0;
    virtual QString selectedText() const = 0;
    virtual QString textAt(int pos, int length) const = 0;
    virtual QChar characterAt(int pos) const = 0;

    /*! Removes \a length characters to the right of the cursor. */
    virtual void remove(int length) = 0;
    /*! Inserts the given string to the right of the cursor. */
    virtual void insert(const QString &string) = 0;
    /*! Replaces \a length characters to the right of the cursor with the given string. */
    virtual void replace(int length, const QString &string) = 0;
    /*! Sets current cursor position to \a pos. */
    virtual void setCursorPosition(int pos) = 0;
    /*! Selects text between current cursor position and \a toPos. */
    virtual void select(int toPos) = 0;

    virtual ITextMarkable *markableInterface() = 0;

    virtual void setContextHelpId(const QString &) = 0;

    enum TextCodecReason {
        TextCodecOtherReason,
        TextCodecFromSystemSetting,
        TextCodecFromProjectSetting
    };

    virtual void setTextCodec(QTextCodec *, TextCodecReason reason = TextCodecOtherReason) = 0;
    virtual QTextCodec *textCodec() const = 0;

    static QMap<QString, QString> openedTextEditorsContents();
    static QMap<QString, QTextCodec *> openedTextEditorsEncodings();

signals:
    void contentsChanged();
    void contentsChangedBecauseOfUndo();
    void markRequested(TextEditor::ITextEditor *editor, int line);
    void markContextMenuRequested(TextEditor::ITextEditor *editor, int line, QMenu *menu);
    void tooltipOverrideRequested(TextEditor::ITextEditor *editor, const QPoint &globalPos, int position, bool *handled);
    void tooltipRequested(TextEditor::ITextEditor *editor, const QPoint &globalPos, int position);
    void contextHelpIdRequested(TextEditor::ITextEditor *editor, int position);
};

} // namespace TextEditor

#endif // ITEXTEDITOR_H
