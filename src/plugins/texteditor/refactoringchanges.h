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

#ifndef REFACTORINGCHANGES_H
#define REFACTORINGCHANGES_H

#include <utils/changeset.h>
#include <utils/textfileformat.h>
#include <texteditor/texteditor_global.h>

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor {
class BaseTextEditorWidget;
class RefactoringChanges;
class RefactoringFile;
class RefactoringChangesData;
typedef QSharedPointer<RefactoringFile> RefactoringFilePtr;

// ### listen to the m_editor::destroyed signal?
class TEXTEDITOR_EXPORT RefactoringFile
{
    Q_DISABLE_COPY(RefactoringFile)
public:
    typedef Utils::ChangeSet::Range Range;

public:
    virtual ~RefactoringFile();

    bool isValid() const;

    const QTextDocument *document() const;
    // mustn't use the cursor to change the document
    const QTextCursor cursor() const;
    QString fileName() const;

    // converts 1-based line and column into 0-based source offset
    int position(unsigned line, unsigned column) const;
    // converts 0-based source offset into 1-based line and column
    void lineAndColumn(int offset, unsigned *line, unsigned *column) const;

    QChar charAt(int pos) const;
    QString textOf(int start, int end) const;
    QString textOf(const Range &range) const;

    void setChangeSet(const Utils::ChangeSet &changeSet);
    void appendIndentRange(const Range &range);
    void setOpenEditor(bool activate = false, int pos = -1);
    void apply();

protected:
    // users may only get const access to RefactoringFiles created through
    // this constructor, because it can't be used to apply changes
    RefactoringFile(QTextDocument *document, const QString &fileName);

    RefactoringFile(BaseTextEditorWidget *editor);
    RefactoringFile(const QString &fileName, const QSharedPointer<RefactoringChangesData> &data);

    QTextDocument *mutableDocument() const;
    // derived classes may want to clear language specific extra data
    virtual void fileChanged();

protected:
    QString m_fileName;
    QSharedPointer<RefactoringChangesData> m_data;
    mutable Utils::TextFileFormat m_textFileFormat;
    mutable QTextDocument *m_document;
    BaseTextEditorWidget *m_editor;
    Utils::ChangeSet m_changes;
    QList<Range> m_indentRanges;
    bool m_openEditor;
    bool m_activateEditor;
    int m_editorCursorPosition;

    friend class RefactoringChanges; // access to constructor
};

 /*!
    This class batches changes to multiple file, which are applied as a single big
    change.
 */
class TEXTEDITOR_EXPORT RefactoringChanges
{
public:
    typedef Utils::ChangeSet::Range Range;

public:
    RefactoringChanges();
    virtual ~RefactoringChanges();

    static RefactoringFilePtr file(BaseTextEditorWidget *editor);
    RefactoringFilePtr file(const QString &fileName) const;
    bool createFile(const QString &fileName, const QString &contents, bool reindent = true, bool openEditor = true) const;
    bool removeFile(const QString &fileName) const;

    static BaseTextEditorWidget *editorForFile(const QString &fileName);

protected:
    explicit RefactoringChanges(RefactoringChangesData *data);

    static BaseTextEditorWidget *openEditor(const QString &fileName, bool activate, int line, int column);

    static QList<QTextCursor> rangesToSelections(QTextDocument *document, const QList<Range> &ranges);

protected:
    QSharedPointer<RefactoringChangesData> m_data;

    friend class RefactoringFile;
};

class TEXTEDITOR_EXPORT RefactoringChangesData
{
    Q_DISABLE_COPY(RefactoringChangesData)

public:
    RefactoringChangesData() {}
    virtual ~RefactoringChangesData();

    virtual void indentSelection(const QTextCursor &selection,
                                 const QString &fileName,
                                 const BaseTextEditorWidget *textEditor) const;
    virtual void fileChanged(const QString &fileName);
};

} // namespace TextEditor

#endif // REFACTORINGCHANGES_H
