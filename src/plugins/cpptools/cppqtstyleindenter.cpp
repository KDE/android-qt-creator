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

#include "cppqtstyleindenter.h"

#include "cppcodeformatter.h"
#include "cpptoolssettings.h"
#include "cppcodestylepreferences.h"
#include "cpptoolsconstants.h"
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

#include <QtCore/QChar>
#include <QtGui/QTextDocument>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>

using namespace CppTools;

CppQtStyleIndenter::CppQtStyleIndenter()
    : m_cppCodeStylePreferences(0)
{
    // Just for safety. setCodeStylePreferences should be called when the editor the
    // indenter belongs to gets initialized.
    m_cppCodeStylePreferences = CppToolsSettings::instance()->cppCodeStyle();
}

CppQtStyleIndenter::~CppQtStyleIndenter()
{}

bool CppQtStyleIndenter::isElectricCharacter(const QChar &ch) const
{
    if (ch == QLatin1Char('{') ||
        ch == QLatin1Char('}') ||
        ch == QLatin1Char(':') ||
        ch == QLatin1Char('#')) {
        return true;
    }
    return false;
}

static bool colonIsElectric(const QString &text)
{
    // switch cases and access declarations should be reindented
    if (text.contains(QLatin1String("case"))
            || text.contains(QLatin1String("default"))
            || text.contains(QLatin1String("public"))
            || text.contains(QLatin1String("private"))
            || text.contains(QLatin1String("protected"))
            || text.contains(QLatin1String("signals"))
            || text.contains(QLatin1String("Q_SIGNALS"))) {
        return true;
    }

    // lines that start with : might have a constructor initializer list
    const QString trimmedtext = text.trimmed();
    if (!trimmedtext.isEmpty() && trimmedtext.at(0) == QLatin1Char(':'))
        return true;

    return false;
}

void CppQtStyleIndenter::indentBlock(QTextDocument *doc,
                                     const QTextBlock &block,
                                     const QChar &typedChar,
                                     const TextEditor::TabSettings &tabSettings)
{
    Q_UNUSED(doc)

    CppTools::QtStyleCodeFormatter codeFormatter(tabSettings, codeStyleSettings());

    codeFormatter.updateStateUntil(block);
    int indent;
    int padding;
    codeFormatter.indentFor(block, &indent, &padding);

    if (isElectricCharacter(typedChar)) {
        // : should not be electric for labels
        if (typedChar == QLatin1Char(':') && !colonIsElectric(block.text()))
            return;

        // only reindent the current line when typing electric characters if the
        // indent is the same it would be if the line were empty
        int newlineIndent;
        int newlinePadding;
        codeFormatter.indentForNewLineAfter(block.previous(), &newlineIndent, &newlinePadding);
        if (tabSettings.indentationColumn(block.text()) != newlineIndent + newlinePadding)
            return;
    }

    tabSettings.indentLine(block, indent + padding, padding);
}

void CppQtStyleIndenter::indent(QTextDocument *doc,
                                const QTextCursor &cursor,
                                const QChar &typedChar,
                                const TextEditor::TabSettings &tabSettings)
{
    if (cursor.hasSelection()) {
        QTextBlock block = doc->findBlock(cursor.selectionStart());
        const QTextBlock end = doc->findBlock(cursor.selectionEnd()).next();

        CppTools::QtStyleCodeFormatter codeFormatter(tabSettings, codeStyleSettings());
        codeFormatter.updateStateUntil(block);

        QTextCursor tc = cursor;
        tc.beginEditBlock();
        do {
            int indent;
            int padding;
            codeFormatter.indentFor(block, &indent, &padding);
            tabSettings.indentLine(block, indent + padding, padding);
            codeFormatter.updateLineStateChange(block);
            block = block.next();
        } while (block.isValid() && block != end);
        tc.endEditBlock();
    } else {
        indentBlock(doc, cursor.block(), typedChar, tabSettings);
    }
}

void CppQtStyleIndenter::setCodeStylePreferences(TextEditor::ICodeStylePreferences *preferences)
{
    CppTools::CppCodeStylePreferences *cppCodeStylePreferences
            = qobject_cast<CppTools::CppCodeStylePreferences *>(preferences);
    if (cppCodeStylePreferences)
        m_cppCodeStylePreferences = cppCodeStylePreferences;
}

void CppQtStyleIndenter::invalidateCache(QTextDocument *doc)
{
    CppTools::QtStyleCodeFormatter formatter;
    formatter.invalidateCache(doc);
}

CppCodeStyleSettings CppQtStyleIndenter::codeStyleSettings() const
{
    if (m_cppCodeStylePreferences)
        return m_cppCodeStylePreferences->currentCodeStyleSettings();
    return CppCodeStyleSettings();
}
