/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "BackwardsScanner.h"
#include <Token.h>
#include <QtGui/QTextCursor>
#include <QTextDocument>

using namespace CPlusPlus;

BackwardsScanner::BackwardsScanner(const QTextCursor &cursor,
                                   int maxBlockCount,
                                   const QString &suffix,
                                   bool skipComments)
    : _offset(0)
    , _blocksTokenized(0)
    , _block(cursor.block())
    , _maxBlockCount(maxBlockCount)
{
    _tokenize.setQtMocRunEnabled(true);
    _tokenize.setSkipComments(skipComments);
    _tokenize.setObjCEnabled(true);
    _text = _block.text().left(cursor.position() - cursor.block().position());

    if (! suffix.isEmpty())
        _text += suffix;

    _tokens.append(_tokenize(_text, previousBlockState(_block)));

    _startToken = _tokens.size();
}

Token BackwardsScanner::LA(int index) const
{ return const_cast<BackwardsScanner *>(this)->fetchToken(_startToken - index); }

Token BackwardsScanner::operator[](int index) const
{ return const_cast<BackwardsScanner *>(this)->fetchToken(index); }

const Token &BackwardsScanner::fetchToken(int tokenIndex)
{
    while (_offset + tokenIndex < 0) {
        _block = _block.previous();
        if (_blocksTokenized == _maxBlockCount || !_block.isValid()) {
            ++_offset;
            _tokens.prepend(Token()); // sentinel
            break;
        } else {
            ++_blocksTokenized;

            QString blockText = _block.text();
            _text.prepend(QLatin1Char('\n'));
            _text.prepend(blockText);

            QList<Token> adaptedTokens;
            for (int i = 0; i < _tokens.size(); ++i) {
                Token t = _tokens.at(i);
                t.offset += + blockText.length() + 1;
                adaptedTokens.append(t);
            }

            _tokens = _tokenize(blockText, previousBlockState(_block));
            _offset += _tokens.size();
            _tokens += adaptedTokens;
        }
    }

    return _tokens.at(_offset + tokenIndex);
}

int BackwardsScanner::startToken() const
{ return _startToken; }

int BackwardsScanner::startPosition() const
{ return _block.position(); }

QString BackwardsScanner::text() const
{ return _text; }

QString BackwardsScanner::mid(int index) const
{
    const Token &firstToken = _tokens.at(index + _offset);
    return _text.mid(firstToken.begin());
}

QString BackwardsScanner::text(int index) const
{
    const Token &firstToken = _tokens.at(index + _offset);
    return _text.mid(firstToken.begin(), firstToken.length());
}

QStringRef BackwardsScanner::textRef(int index) const
{
    const Token &firstToken = _tokens.at(index + _offset);
    return _text.midRef(firstToken.begin(), firstToken.length());
}

int BackwardsScanner::size() const
{
    return _tokens.size();
}

int BackwardsScanner::startOfMatchingBrace(int index) const
{
    const BackwardsScanner &tk = *this;

    if (tk[index - 1].is(T_RPAREN)) {
        int i = index - 1;
        int count = 0;
        do {
            if (tk[i].is(T_LPAREN)) {
                if (! ++count)
                    return i;
            } else if (tk[i].is(T_RPAREN))
                --count;
            --i;
        } while (count != 0 && tk[i].isNot(T_EOF_SYMBOL));
    } else if (tk[index - 1].is(T_RBRACKET)) {
        int i = index - 1;
        int count = 0;
        do {
            if (tk[i].is(T_LBRACKET)) {
                if (! ++count)
                    return i;
            } else if (tk[i].is(T_RBRACKET))
                --count;
            --i;
        } while (count != 0 && tk[i].isNot(T_EOF_SYMBOL));
    } else if (tk[index - 1].is(T_RBRACE)) {
        int i = index - 1;
        int count = 0;
        do {
            if (tk[i].is(T_LBRACE)) {
                if (! ++count)
                    return i;
            } else if (tk[i].is(T_RBRACE))
                --count;
            --i;
        } while (count != 0 && tk[i].isNot(T_EOF_SYMBOL));
    } else if (tk[index - 1].is(T_GREATER)) {
        int i = index - 1;
        int count = 0;
        do {
            if (tk[i].is(T_LESS)) {
                if (! ++count)
                    return i;
            } else if (tk[i].is(T_GREATER))
                --count;
            --i;
        } while (count != 0 && tk[i].isNot(T_EOF_SYMBOL));
    } else {
        Q_ASSERT(0);
    }

    return index;
}

int BackwardsScanner::startOfLine(int index) const
{
    const BackwardsScanner tk(*this);

    forever {
        const Token &tok = tk[index - 1];

        if (tok.is(T_EOF_SYMBOL))
            break;
        else if (tok.newline())
            return index - 1;

        --index;
    }

    return index;
}

int BackwardsScanner::startOfBlock(int index) const
{
    const BackwardsScanner tk(*this);

    const int start = index;

    forever {
        Token token = tk[index - 1];

        if (token.is(T_EOF_SYMBOL)) {
            break;

        } else if (token.is(T_GREATER)) {
            const int matchingBrace = startOfMatchingBrace(index);

            if (matchingBrace != index && tk[matchingBrace - 1].is(T_TEMPLATE))
                index = matchingBrace;

        } else if (token.is(T_RPAREN) || token.is(T_RBRACKET) || token.is(T_RBRACE)) {
            const int matchingBrace = startOfMatchingBrace(index);

            if (matchingBrace != index)
                index = matchingBrace;

        } else if (token.is(T_LPAREN) || token.is(T_LBRACKET)) {
            break; // unmatched brace

        } else if (token.is(T_LBRACE)) {
            return index - 1;

        }

        --index;
    }

    return start;
}

QString BackwardsScanner::indentationString(int index) const
{
    const Token tokenAfterNewline = operator[](startOfLine(index + 1));
    const int newlinePos = qMax(0, _text.lastIndexOf(QLatin1Char('\n'),
                                                     tokenAfterNewline.begin()));
    return _text.mid(newlinePos, tokenAfterNewline.begin() - newlinePos);
}


int BackwardsScanner::previousBlockState(const QTextBlock &block)
{
    const QTextBlock prevBlock = block.previous();

    if (prevBlock.isValid()) {
        int state = prevBlock.userState();

        if (state != -1)
            return state;
    }

    return 0;
}
