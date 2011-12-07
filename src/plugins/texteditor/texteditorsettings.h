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

#ifndef TEXTEDITORSETTINGS_H
#define TEXTEDITORSETTINGS_H

#include "texteditor_global.h"

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
template <class Key, class T>
class QMap;
QT_END_NAMESPACE

namespace TextEditor {

class BaseTextEditorWidget;
class FontSettings;
class TabSettings;
class TypingSettings;
class StorageSettings;
class BehaviorSettings;
class DisplaySettings;
class CompletionSettings;
class HighlighterSettings;
class ExtraEncodingSettings;
class ICodeStylePreferences;
class ICodeStylePreferencesFactory;
class CodeStylePool;

namespace Internal {
class TextEditorSettingsPrivate;
}

/**
 * This class provides a central place for basic text editor settings. These
 * settings include font settings, tab settings, storage settings, behavior
 * settings, display settings and completion settings.
 */
class TEXTEDITOR_EXPORT TextEditorSettings : public QObject
{
    Q_OBJECT

public:
    explicit TextEditorSettings(QObject *parent);
    ~TextEditorSettings();

    static TextEditorSettings *instance();

    void initializeEditor(BaseTextEditorWidget *editor);

    const FontSettings &fontSettings() const;
    const TypingSettings &typingSettings() const;
    const StorageSettings &storageSettings() const;
    const BehaviorSettings &behaviorSettings() const;
    const DisplaySettings &displaySettings() const;
    const CompletionSettings &completionSettings() const;
    const HighlighterSettings &highlighterSettings() const;
    const ExtraEncodingSettings &extraEncodingSettings() const;

    void setCompletionSettings(const TextEditor::CompletionSettings &);

    ICodeStylePreferencesFactory *codeStyleFactory(const QString &languageId) const;
    QMap<QString, ICodeStylePreferencesFactory *> codeStyleFactories() const;
    void registerCodeStyleFactory(ICodeStylePreferencesFactory *codeStyleFactory);

    CodeStylePool *codeStylePool() const;
    CodeStylePool *codeStylePool(const QString &languageId) const;
    void registerCodeStylePool(const QString &languageId, CodeStylePool *pool);

    ICodeStylePreferences *codeStyle() const;
    ICodeStylePreferences *codeStyle(const QString &languageId) const;
    QMap<QString, ICodeStylePreferences *> codeStyles() const;
    void registerCodeStyle(const QString &languageId, ICodeStylePreferences *prefs);

    void registerMimeTypeForLanguageId(const QString &mimeType, const QString &languageId);
    QString languageId(const QString &mimeType) const;

signals:
    void fontSettingsChanged(const TextEditor::FontSettings &);
    void typingSettingsChanged(const TextEditor::TypingSettings &);
    void storageSettingsChanged(const TextEditor::StorageSettings &);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettings &);
    void displaySettingsChanged(const TextEditor::DisplaySettings &);
    void completionSettingsChanged(const TextEditor::CompletionSettings &);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettings &);

private:
    Internal::TextEditorSettingsPrivate *m_d;
    Q_PRIVATE_SLOT(m_d, void fontZoomRequested(int pointSize))
    Q_PRIVATE_SLOT(m_d, void zoomResetRequested())

    static TextEditorSettings *m_instance;
};

} // namespace TextEditor

#endif // TEXTEDITORSETTINGS_H
