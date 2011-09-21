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

#include "refactoringchanges.h"
#include "basetexteditor.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QtCore/QFile>
#include <QtCore/QSet>
#include <QtGui/QMainWindow>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtCore/QDebug>

using namespace TextEditor;

RefactoringChanges::RefactoringChanges()
    : m_data(new RefactoringChangesData)
{}

RefactoringChanges::RefactoringChanges(RefactoringChangesData *data)
    : m_data(data)
{}

RefactoringChanges::~RefactoringChanges()
{}

BaseTextEditorWidget *RefactoringChanges::editorForFile(const QString &fileName)
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();

    const QList<Core::IEditor *> editors = editorManager->editorsForFileName(fileName);
    foreach (Core::IEditor *editor, editors) {
        BaseTextEditorWidget *textEditor = qobject_cast<BaseTextEditorWidget *>(editor->widget());
        if (textEditor != 0)
            return textEditor;
    }
    return 0;
}

QList<QTextCursor> RefactoringChanges::rangesToSelections(QTextDocument *document, const QList<Range> &ranges)
{
    QList<QTextCursor> selections;

    foreach (const Range &range, ranges) {
        QTextCursor selection(document);
        // ### workaround for moving the textcursor when inserting text at the beginning of the range.
        selection.setPosition(qMax(0, range.start - 1));
        selection.setPosition(qMin(range.end, document->characterCount() - 1), QTextCursor::KeepAnchor);

        selections.append(selection);
    }

    return selections;
}

bool RefactoringChanges::createFile(const QString &fileName, const QString &contents, bool reindent, bool openEditor) const
{
    if (QFile::exists(fileName))
        return false;

    BaseTextEditorWidget *editor = editorForFile(fileName);
    if (!editor && openEditor) {
        editor = this->openEditor(fileName, false, -1, -1);
    }

    QTextDocument *document;
    if (editor)
        document = editor->document();
    else
        document = new QTextDocument;

    {
        QTextCursor cursor(document);
        cursor.beginEditBlock();

        cursor.insertText(contents);

        if (reindent) {
            cursor.select(QTextCursor::Document);
            m_data->indentSelection(cursor, fileName, editor);
        }

        cursor.endEditBlock();
    }

    if (!editor) {
        Utils::TextFileFormat format;
        format.codec = Core::EditorManager::instance()->defaultTextCodec();
        QString error;
        bool saveOk = format.writeFile(fileName, document->toPlainText(), &error);
        delete document;
        if (!saveOk)
            return false;
    }

    m_data->fileChanged(fileName);

    return true;
}

bool RefactoringChanges::removeFile(const QString &fileName) const
{
    if (!QFile::exists(fileName))
        return false;

    // ### implement!
    qWarning() << "RefactoringChanges::removeFile is not implemented";
    return true;
}

BaseTextEditorWidget *RefactoringChanges::openEditor(const QString &fileName, bool activate, int line, int column)
{
    Core::EditorManager::OpenEditorFlags flags = Core::EditorManager::IgnoreNavigationHistory;
    if (!activate)
        flags |= Core::EditorManager::NoActivate;
    if (line != -1) {
        // openEditorAt uses a 1-based line and a 0-based column!
        column -= 1;
    }
    Core::IEditor *editor = BaseTextEditorWidget::openEditorAt(
                fileName, line, column, QString(), flags);
    return qobject_cast<BaseTextEditorWidget *>(editor->widget());
}

RefactoringFilePtr RefactoringChanges::file(BaseTextEditorWidget *editor)
{
    return RefactoringFilePtr(new RefactoringFile(editor));
}

RefactoringFilePtr RefactoringChanges::file(const QString &fileName) const
{
    return RefactoringFilePtr(new RefactoringFile(fileName, m_data));
}

RefactoringFile::RefactoringFile(QTextDocument *document, const QString &fileName)
    : m_fileName(fileName)
    , m_document(document)
    , m_editor(0)
    , m_openEditor(false)
    , m_activateEditor(false)
    , m_editorCursorPosition(-1)
{ }

RefactoringFile::RefactoringFile(BaseTextEditorWidget *editor)
    : m_fileName(editor->file()->fileName())
    , m_document(0)
    , m_editor(editor)
    , m_openEditor(false)
    , m_activateEditor(false)
    , m_editorCursorPosition(-1)
{ }

RefactoringFile::RefactoringFile(const QString &fileName, const QSharedPointer<RefactoringChangesData> &data)
    : m_fileName(fileName)
    , m_data(data)
    , m_document(0)
    , m_editor(0)
    , m_openEditor(false)
    , m_activateEditor(false)
    , m_editorCursorPosition(-1)
{
    m_editor = RefactoringChanges::editorForFile(fileName);
}

RefactoringFile::~RefactoringFile()
{
    delete m_document;
}

bool RefactoringFile::isValid() const
{
    if (m_fileName.isEmpty())
        return false;
    return document();
}

const QTextDocument *RefactoringFile::document() const
{
    return mutableDocument();
}

QTextDocument *RefactoringFile::mutableDocument() const
{
    if (m_editor)
        return m_editor->document();
    else if (!m_document) {
        QString fileContents;
        if (!m_fileName.isEmpty()) {
            QString error;
            QTextCodec *defaultCodec = Core::EditorManager::instance()->defaultTextCodec();
            Utils::TextFileFormat::ReadResult result = Utils::TextFileFormat::readFile(
                        m_fileName, defaultCodec,
                        &fileContents, &m_textFileFormat,
                        &error);
            if (result != Utils::TextFileFormat::ReadSuccess) {
                qWarning() << "Could not read " << m_fileName << ". Error: " << error;
                m_textFileFormat.codec = 0;
            }
        }
        // always make a QTextDocument to avoid excessive null checks
        m_document = new QTextDocument(fileContents);
    }
    return m_document;
}

const QTextCursor RefactoringFile::cursor() const
{
    if (m_editor)
        return m_editor->textCursor();
    else if (!m_fileName.isEmpty()) {
        if (QTextDocument *doc = mutableDocument())
            return QTextCursor(doc);
    }

    return QTextCursor();
}

QString RefactoringFile::fileName() const
{
    return m_fileName;
}

int RefactoringFile::position(unsigned line, unsigned column) const
{
    Q_ASSERT(line != 0);
    Q_ASSERT(column != 0);
    if (const QTextDocument *doc = document())
        return doc->findBlockByNumber(line - 1).position() + column - 1;
    return -1;
}

void RefactoringFile::lineAndColumn(int offset, unsigned *line, unsigned *column) const
{
    Q_ASSERT(line);
    Q_ASSERT(column);
    Q_ASSERT(offset >= 0);
    QTextCursor c(cursor());
    c.setPosition(offset);
    *line = c.blockNumber() + 1;
    *column = c.positionInBlock() + 1;
}

QChar RefactoringFile::charAt(int pos) const
{
    if (const QTextDocument *doc = document())
        return doc->characterAt(pos);
    return QChar();
}

QString RefactoringFile::textOf(int start, int end) const
{
    QTextCursor c = cursor();
    c.setPosition(start);
    c.setPosition(end, QTextCursor::KeepAnchor);
    return c.selectedText();
}

QString RefactoringFile::textOf(const Range &range) const
{
    return textOf(range.start, range.end);
}

void RefactoringFile::setChangeSet(const Utils::ChangeSet &changeSet)
{
    if (m_fileName.isEmpty())
        return;

    m_changes = changeSet;
}

void RefactoringFile::appendIndentRange(const Range &range)
{
    if (m_fileName.isEmpty())
        return;

    m_indentRanges.append(range);
}

void RefactoringFile::setOpenEditor(bool activate, int pos)
{
    m_openEditor = true;
    m_activateEditor = activate;
    m_editorCursorPosition = pos;
}

void RefactoringFile::apply()
{
    // open / activate / goto position
    if (m_openEditor && !m_fileName.isEmpty()) {
        unsigned line = -1, column = -1;
        if (m_editorCursorPosition != -1)
            lineAndColumn(m_editorCursorPosition, &line, &column);
        m_editor = RefactoringChanges::openEditor(m_fileName, m_activateEditor, line, column);
        m_openEditor = false;
        m_activateEditor = false;
        m_editorCursorPosition = -1;
    }

    // apply changes, if any
    if (m_data && !(m_indentRanges.isEmpty() && m_changes.isEmpty())) {
        QTextDocument *doc = mutableDocument();
        if (!doc)
            return;

        {
            QTextCursor c(doc);
            c.beginEditBlock();

            // build indent selections now, applying the changeset will change locations
            const QList<QTextCursor> &indentSelections =
                    RefactoringChanges::rangesToSelections(
                            doc, m_indentRanges);
            m_indentRanges.clear();

            // apply changes and reindent
            m_changes.apply(&c);
            m_changes.clear();
            foreach (const QTextCursor &selection, indentSelections) {
                m_data->indentSelection(selection, m_fileName, m_editor);
            }

            c.endEditBlock();
        }

        // if this document doesn't have an editor, write the result to a file
        if (!m_editor && m_textFileFormat.codec) {
            QTC_ASSERT(!m_fileName.isEmpty(), return);
            QString error;
            if (!m_textFileFormat.writeFile(m_fileName, doc->toPlainText(), &error))
                qWarning() << "Could not apply changes to" << m_fileName << ". Error: " << error;
        }

        fileChanged();
    }
}

void RefactoringFile::fileChanged()
{
    if (!m_fileName.isEmpty())
        m_data->fileChanged(m_fileName);
}

RefactoringChangesData::~RefactoringChangesData()
{}

void RefactoringChangesData::indentSelection(const QTextCursor &, const QString &, const BaseTextEditorWidget *) const
{
    qWarning() << Q_FUNC_INFO << "not implemented";
}

void RefactoringChangesData::fileChanged(const QString &)
{
}
