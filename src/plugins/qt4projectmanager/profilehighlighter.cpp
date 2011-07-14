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

#include "profilehighlighter.h"
#include "profilekeywords.h"

#include <QtCore/QRegExp>
#include <QtGui/QColor>
#include <QtGui/QTextDocument>
#include <QtGui/QTextEdit>

using namespace Qt4ProjectManager::Internal;


ProFileHighlighter::ProFileHighlighter(QTextDocument *document) :
    TextEditor::SyntaxHighlighter(document)
{
}

void ProFileHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty())
        return;

    QString buf;
    bool inCommentMode = false;

    QTextCharFormat emptyFormat;
    int i = 0;
    for (;;) {
        const QChar c = text.at(i);
        if (inCommentMode) {
            setFormat(i, 1, m_formats[ProfileCommentFormat]);
        } else {
            if (c.isLetter() || c == '_' || c == '.' || c.isDigit()) {
                buf += c;
                setFormat(i - buf.length()+1, buf.length(), emptyFormat);
                if (!buf.isEmpty() && ProFileKeywords::isFunction(buf))
                    setFormat(i - buf.length()+1, buf.length(), m_formats[ProfileFunctionFormat]);
                else if (!buf.isEmpty() && ProFileKeywords::isVariable(buf))
                    setFormat(i - buf.length()+1, buf.length(), m_formats[ProfileVariableFormat]);
            } else if (c == '(') {
                if (!buf.isEmpty() && ProFileKeywords::isFunction(buf))
                    setFormat(i - buf.length(), buf.length(), m_formats[ProfileFunctionFormat]);
                buf.clear();
            } else if (c == '#') {
                inCommentMode = true;
                setFormat(i, 1, m_formats[ProfileCommentFormat]);
                buf.clear();
            } else {
                if (!buf.isEmpty() && ProFileKeywords::isVariable(buf))
                    setFormat(i - buf.length(), buf.length(), m_formats[ProfileVariableFormat]);
                buf.clear();
            }
        }
        i++;
        if (i >= text.length())
            break;
    }

    applyFormatToSpaces(text, m_formats[ProfileVisualWhitespaceFormat]);
}
