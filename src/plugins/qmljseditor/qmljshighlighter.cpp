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

#include "qmljshighlighter.h"

#include <QtCore/QSet>
#include <QtCore/QtAlgorithms>
#include <QtCore/QDebug>

#include <utils/qtcassert.h>

using namespace QmlJSEditor;
using namespace QmlJS;

Highlighter::Highlighter(QTextDocument *parent)
    : TextEditor::SyntaxHighlighter(parent),
      m_qmlEnabled(true),
      m_inMultilineComment(false)
{
    m_currentBlockParentheses.reserve(20);
    m_braceDepth = 0;
    m_foldingIndent = 0;
}

Highlighter::~Highlighter()
{
}

bool Highlighter::isQmlEnabled() const
{
    return m_qmlEnabled;
}

void Highlighter::setQmlEnabled(bool qmlEnabled)
{
    m_qmlEnabled = qmlEnabled;
}

static bool checkStartOfBinding(const Token &token)
{
    switch (token.kind) {
    case Token::Semicolon:
    case Token::LeftBrace:
    case Token::RightBrace:
    case Token::LeftBracket:
    case Token::RightBracket:
        return true;

    default:
        return false;
    } // end of switch
}

void Highlighter::setFormats(const QVector<QTextCharFormat> &formats)
{
    QTC_ASSERT(formats.size() == NumFormats, return);
    qCopy(formats.begin(), formats.end(), m_formats);
}

void Highlighter::highlightBlock(const QString &text)
{
    const QList<Token> tokens = m_scanner(text, onBlockStart());

    int index = 0;
    while (index < tokens.size()) {
        const Token &token = tokens.at(index);

        switch (token.kind) {
            case Token::Keyword:
                setFormat(token.offset, token.length, m_formats[KeywordFormat]);
                break;

            case Token::String:
                setFormat(token.offset, token.length, m_formats[StringFormat]);
                break;

            case Token::Comment:
                if (m_inMultilineComment && text.midRef(token.end() - 2, 2) == QLatin1String("*/")) {
                    onClosingParenthesis('-', token.end() - 1, index == tokens.size()-1);
                    m_inMultilineComment = false;
                } else if (!m_inMultilineComment
                           && m_scanner.state() == Scanner::MultiLineComment
                           && index == tokens.size() - 1) {
                    onOpeningParenthesis('+', token.offset, index == 0);
                    m_inMultilineComment = true;
                }
                setFormat(token.offset, token.length, m_formats[CommentFormat]);
                break;

            case Token::LeftParenthesis:
                onOpeningParenthesis('(', token.offset, index == 0);
                break;

            case Token::RightParenthesis:
                onClosingParenthesis(')', token.offset, index == tokens.size()-1);
                break;

            case Token::LeftBrace:
                onOpeningParenthesis('{', token.offset, index == 0);
                break;

            case Token::RightBrace:
                onClosingParenthesis('}', token.offset, index == tokens.size()-1);
                break;

            case Token::LeftBracket:
                onOpeningParenthesis('[', token.offset, index == 0);
                break;

            case Token::RightBracket:
                onClosingParenthesis(']', token.offset, index == tokens.size()-1);
                break;

            case Token::Identifier: {
                if (!m_qmlEnabled)
                    break;

                const QStringRef spell = text.midRef(token.offset, token.length);

                if (maybeQmlKeyword(spell)) {
                    // check the previous token
                    if (index == 0 || tokens.at(index - 1).isNot(Token::Dot)) {
                        if (index + 1 == tokens.size() || tokens.at(index + 1).isNot(Token::Colon)) {
                            setFormat(token.offset, token.length, m_formats[KeywordFormat]);
                            break;
                        }
                    }
                } else if (index > 0 && maybeQmlBuiltinType(spell)) {
                    const Token &previousToken = tokens.at(index - 1);
                    if (previousToken.is(Token::Identifier) && text.at(previousToken.offset) == QLatin1Char('p')
                        && text.midRef(previousToken.offset, previousToken.length) == QLatin1String("property")) {
                        setFormat(token.offset, token.length, m_formats[KeywordFormat]);
                        break;
                    }
                }

                if (!spell.isEmpty() && spell.at(0).isUpper())
                    setFormat(token.offset, token.length, m_formats[TypeFormat]);

                if (index + 1 < tokens.size()) {
                    bool maybeBinding = (index == 0 || checkStartOfBinding(tokens.at(index - 1)));
                    bool maybeOnBinding = false;
                    if (index > 0) {
                        const Token &previousToken = tokens.at(index - 1);
                        if (text.midRef(previousToken.offset, previousToken.length) == QLatin1String("on")) {
                            maybeOnBinding = true;
                            maybeBinding = false;
                        }
                    }

                    if (maybeBinding || maybeOnBinding) {
                        Token::Kind expectedTerminator = Token::Colon;
                        if (maybeOnBinding)
                            expectedTerminator = Token::LeftBrace;

                        const int start = index;

                        // put index on last identifier not followed by .identifier
                        while (index + 2 < tokens.size() &&
                               tokens.at(index + 1).is(Token::Dot) &&
                               tokens.at(index + 2).is(Token::Identifier)) {
                            index += 2;
                        }

                        if (index + 1 < tokens.size() && tokens.at(index + 1).is(expectedTerminator)) {
                            // it's a binding.
                            for (int i = start; i <= index; ++i) {
                                const Token &tok = tokens.at(i);
                                if (tok.kind == Token::Dot)
                                    continue;
                                const QStringRef tokSpell = text.midRef(tok.offset, tok.length);
                                if (!tokSpell.isEmpty() && tokSpell.at(0).isUpper()) {
                                    setFormat(tok.offset, tok.length, m_formats[TypeFormat]);
                                } else {
                                    setFormat(tok.offset, tok.length, m_formats[FieldFormat]);
                                }
                            }
                            break;
                        } else {
                            index = start;
                        }
                    }
                }
            }   break;

            case Token::Delimiter:
                break;

            default:
                break;
        } // end swtich

        ++index;
    }

    int previousTokenEnd = 0;
    for (int index = 0; index < tokens.size(); ++index) {
        const Token &token = tokens.at(index);
        setFormat(previousTokenEnd, token.begin() - previousTokenEnd, m_formats[VisualWhitespace]);

        switch (token.kind) {
        case Token::Comment:
        case Token::String: {
            int i = token.begin(), e = token.end();
            while (i < e) {
                const QChar ch = text.at(i);
                if (ch.isSpace()) {
                    const int start = i;
                    do {
                        ++i;
                    } while (i < e && text.at(i).isSpace());
                    setFormat(start, i - start, m_formats[VisualWhitespace]);
                } else {
                    ++i;
                }
            }
        } break;

        default:
            break;
        } // end of switch

        previousTokenEnd = token.end();
    }

    setFormat(previousTokenEnd, text.length() - previousTokenEnd, m_formats[VisualWhitespace]);

    setCurrentBlockState(m_scanner.state());
    onBlockEnd(m_scanner.state());
}

bool Highlighter::maybeQmlKeyword(const QStringRef &text) const
{
    if (text.isEmpty())
        return false;

    const QChar ch = text.at(0);
    if (ch == QLatin1Char('p') && text == QLatin1String("property")) {
        return true;
    } else if (ch == QLatin1Char('a') && text == QLatin1String("alias")) {
        return true;
    } else if (ch == QLatin1Char('s') && text == QLatin1String("signal")) {
        return true;
    } else if (ch == QLatin1Char('p') && text == QLatin1String("property")) {
        return true;
    } else if (ch == QLatin1Char('r') && text == QLatin1String("readonly")) {
        return true;
    } else if (ch == QLatin1Char('i') && text == QLatin1String("import")) {
        return true;
    } else if (ch == QLatin1Char('o') && text == QLatin1String("on")) {
        return true;
    } else {
        return false;
    }
}

bool Highlighter::maybeQmlBuiltinType(const QStringRef &text) const
{
    if (text.isEmpty())
        return false;

    const QChar ch = text.at(0);

    if (ch == QLatin1Char('i') && text == QLatin1String("int")) {
        return true;
    } else if (ch == QLatin1Char('b') && text == QLatin1String("bool")) {
        return true;
    } else if (ch == QLatin1Char('d') && text == QLatin1String("double")) {
        return true;
    } else if (ch == QLatin1Char('r') && text == QLatin1String("real")) {
        return true;
    } else if (ch == QLatin1Char('s') && text == QLatin1String("string")) {
        return true;
    } else if (ch == QLatin1Char('u') && text == QLatin1String("url")) {
        return true;
    } else if (ch == QLatin1Char('c') && text == QLatin1String("color")) {
        return true;
    } else if (ch == QLatin1Char('d') && text == QLatin1String("date")) {
        return true;
    } else if (ch == QLatin1Char('v') && text == QLatin1String("var")) {
        return true;
    } else if (ch == QLatin1Char('v') && text == QLatin1String("variant")) {
        return true;
    } else {
        return false;
    }
}

int Highlighter::onBlockStart()
{
    m_currentBlockParentheses.clear();
    m_braceDepth = 0;
    m_foldingIndent = 0;
    m_inMultilineComment = false;
    if (TextEditor::TextBlockUserData *userData = TextEditor::BaseTextDocumentLayout::testUserData(currentBlock())) {
        userData->setFoldingIndent(0);
        userData->setFoldingStartIncluded(false);
        userData->setFoldingEndIncluded(false);
    }

    int state = 0;
    int previousState = previousBlockState();
    if (previousState != -1) {
        state = previousState & 0xff;
        m_braceDepth = (previousState >> 8);
        m_inMultilineComment = (state == Scanner::MultiLineComment);
    }
    m_foldingIndent = m_braceDepth;

    return state;
}

void Highlighter::onBlockEnd(int state)
{
    typedef TextEditor::TextBlockUserData TextEditorBlockData;

    setCurrentBlockState((m_braceDepth << 8) | state);
    TextEditor::BaseTextDocumentLayout::setParentheses(currentBlock(), m_currentBlockParentheses);
    TextEditor::BaseTextDocumentLayout::setFoldingIndent(currentBlock(), m_foldingIndent);
}

void Highlighter::onOpeningParenthesis(QChar parenthesis, int pos, bool atStart)
{
    if (parenthesis == QLatin1Char('{') || parenthesis == QLatin1Char('[') || parenthesis == QLatin1Char('+')) {
        ++m_braceDepth;
        // if a folding block opens at the beginning of a line, treat the entire line
        // as if it were inside the folding block
        if (atStart)
            TextEditor::BaseTextDocumentLayout::userData(currentBlock())->setFoldingStartIncluded(true);
    }
    m_currentBlockParentheses.push_back(Parenthesis(Parenthesis::Opened, parenthesis, pos));
}

void Highlighter::onClosingParenthesis(QChar parenthesis, int pos, bool atEnd)
{
    if (parenthesis == QLatin1Char('}') || parenthesis == QLatin1Char(']') || parenthesis == QLatin1Char('-')) {
        --m_braceDepth;
        if (atEnd)
            TextEditor::BaseTextDocumentLayout::userData(currentBlock())->setFoldingEndIncluded(true);
        else
            m_foldingIndent = qMin(m_braceDepth, m_foldingIndent); // folding indent is the minimum brace depth of a block
    }
    m_currentBlockParentheses.push_back(Parenthesis(Parenthesis::Closed, parenthesis, pos));
}

