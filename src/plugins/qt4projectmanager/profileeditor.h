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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PROFILEEDITOR_H
#define PROFILEEDITOR_H

#include <texteditor/basetextdocument.h>
#include <texteditor/basetexteditor.h>
#include <utils/uncommentselection.h>

namespace TextEditor {
class FontSettings;
class TextEditorActionHandler;
}

namespace Qt4ProjectManager {

class Qt4Manager;
class Qt4Project;

namespace Internal {

class ProFileEditorFactory;
class ProFileHighlighter;

class ProFileEditorWidget;

class ProFileEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    ProFileEditor(ProFileEditorWidget *);
    Core::Context context() const;

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    QString id() const;
    bool isTemporary() const { return false; }
private:
    const Core::Context m_context;
};

class ProFileEditorWidget : public TextEditor::BaseTextEditorWidget
{
    Q_OBJECT

public:
    ProFileEditorWidget(QWidget *parent, ProFileEditorFactory *factory,
                  TextEditor::TextEditorActionHandler *ah);
    ~ProFileEditorWidget();

    bool save(const QString &fileName = QString());


    ProFileEditorFactory *factory() { return m_factory; }
    TextEditor::TextEditorActionHandler *actionHandler() const { return m_ah; }

    void unCommentSelection();
protected:
    virtual Link findLinkAt(const QTextCursor &, bool resolveTarget = true);
    TextEditor::BaseTextEditor *createEditor();
    void contextMenuEvent(QContextMenuEvent *);

public slots:
    virtual void setFontSettings(const TextEditor::FontSettings &);
    void addLibrary();
    void jumpToFile();

private:
    ProFileEditorFactory *m_factory;
    TextEditor::TextEditorActionHandler *m_ah;
    Utils::CommentDefinition m_commentDefinition;
};

class ProFileDocument : public TextEditor::BaseTextDocument
{
    Q_OBJECT

public:
    ProFileDocument();
    QString defaultPath() const;
    QString suggestedFileName() const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROFILEEDITOR_H
