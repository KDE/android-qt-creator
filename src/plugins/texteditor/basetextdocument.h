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

#ifndef BASETEXTDOCUMENT_H
#define BASETEXTDOCUMENT_H

#include "texteditor_global.h"

#include <coreplugin/ifile.h>

QT_BEGIN_NAMESPACE
class QTextCursor;
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor {

class ITextMarkable;
class StorageSettings;
class TabSettings;
class ExtraEncodingSettings;
class SyntaxHighlighter;
class BaseTextDocumentPrivate;

class TEXTEDITOR_EXPORT BaseTextDocument : public Core::IFile
{
    Q_OBJECT

public:
    BaseTextDocument();
    virtual ~BaseTextDocument();

    void setStorageSettings(const StorageSettings &storageSettings);
    void setTabSettings(const TabSettings &tabSettings);
    void setExtraEncodingSettings(const ExtraEncodingSettings &extraEncodingSettings);

    const StorageSettings &storageSettings() const;
    const TabSettings &tabSettings() const;
    const ExtraEncodingSettings &extraEncodingSettings() const;

    ITextMarkable *documentMarker() const;

    // IFile implementation.
    virtual bool save(QString *errorString, const QString &fileName, bool autoSave);
    virtual QString fileName() const;
    virtual bool shouldAutoSave() const;
    virtual bool isReadOnly() const;
    virtual bool isModified() const;
    virtual bool isSaveAsAllowed() const;
    virtual void checkPermissions();
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);
    virtual QString mimeType() const;
    void setMimeType(const QString &mt);
    virtual void rename(const QString &newName);

    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;

    void setDefaultPath(const QString &defaultPath);
    void setSuggestedFileName(const QString &suggestedFileName);

    virtual bool open(QString *errorString, const QString &fileName, const QString &realFileName);
    virtual bool reload(QString *errorString);

    QTextDocument *document() const;
    void setSyntaxHighlighter(SyntaxHighlighter *highlighter);
    SyntaxHighlighter *syntaxHighlighter() const;

    bool hasDecodingError() const;
    QTextCodec *codec() const;
    void setCodec(QTextCodec *c);
    QByteArray decodingErrorSample() const;

    bool reload(QString *errorString, QTextCodec *codec);
    void cleanWhitespace(const QTextCursor &cursor);

    bool hasHighlightWarning() const;
    void setHighlightWarning(bool has);

signals:
    void titleChanged(QString title);

private:
    void cleanWhitespace(QTextCursor &cursor, bool cleanIndentation, bool inEntireDocument);
    void ensureFinalNewLine(QTextCursor &cursor);
    void documentClosing();

    BaseTextDocumentPrivate *d;
};

} // namespace TextEditor

#endif // BASETEXTDOCUMENT_H
