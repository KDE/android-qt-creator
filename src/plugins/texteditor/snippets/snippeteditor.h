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

#ifndef SNIPPETEDITOR_H
#define SNIPPETEDITOR_H

#include <texteditor/texteditor_global.h>
#include <texteditor/basetexteditor.h>

namespace TextEditor {

class SnippetEditorWidget;
class SyntaxHighlighter;
class Indenter;

// Should not be necessary in this case, but the base text editor assumes a
// valid editable interface.
class TEXTEDITOR_EXPORT SnippetEditor : public BaseTextEditor
{
    Q_OBJECT

public:
    SnippetEditor(SnippetEditorWidget *editorWidget);

    bool duplicateSupported() const { return false; }
    Core::IEditor *duplicate(QWidget * /* parent */ ) { return 0; }
    bool isTemporary() const { return false; }
    virtual QString id() const;
};

class TEXTEDITOR_EXPORT SnippetEditorWidget : public BaseTextEditorWidget
{
    Q_OBJECT

public:
    SnippetEditorWidget(QWidget *parent);

    void setSyntaxHighlighter(SyntaxHighlighter *highlighter);

signals:
    void snippetContentChanged();

protected:
    virtual void focusOutEvent(QFocusEvent *event);

    virtual int extraAreaWidth(int * /* markWidthPtr */ = 0) const { return 0; }
    virtual BaseTextEditor *createEditor();
};

} // TextEditor

#endif // SNIPPETEDITOR_H
