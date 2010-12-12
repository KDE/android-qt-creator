/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef COMPLETIONSETTINGSPAGE_H
#define COMPLETIONSETTINGSPAGE_H

#include <texteditor/completionsettings.h>
#include <texteditor/texteditoroptionspage.h>

QT_BEGIN_NAMESPACE
class Ui_CompletionSettingsPage;
QT_END_NAMESPACE

namespace CppTools {
namespace Internal {

// TODO: Move this class to the text editor plugin

class CompletionSettingsPage : public TextEditor::TextEditorOptionsPage
{
    Q_OBJECT

public:
    CompletionSettingsPage();
    ~CompletionSettingsPage();

    QString id() const;
    QString displayName() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    virtual bool matches(const QString &) const;

private:
    TextEditor::CaseSensitivity caseSensitivity() const;
    TextEditor::CompletionTrigger completionTrigger() const;

    Ui_CompletionSettingsPage *m_page;
    QString m_searchKeywords;
};

} // namespace Internal
} // namespace CppTools

#endif // COMPLETIONSETTINGSPAGE_H
