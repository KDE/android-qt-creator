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

#ifndef TEXTEDITOR_SEMANTICHIGHLIGHTER_H
#define TEXTEDITOR_SEMANTICHIGHLIGHTER_H

#include "texteditor_global.h"

#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QVector>
#include <QtCore/QFuture>
#include <QtGui/QTextCharFormat>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor {

class SyntaxHighlighter;

namespace SemanticHighlighter {

class TEXTEDITOR_EXPORT Result {
public:
    unsigned line; // 1-based
    unsigned column; // 1-based
    unsigned length;
    int kind;

    Result()
        : line(0), column(0), length(0), kind(-1) {}
    Result(unsigned line, unsigned column, unsigned length, int kind)
        : line(line), column(column), length(length), kind(kind) {}
};

// Applies the future results [from, to) and applies the extra formats
// indicated by Result::kind and kindToFormat to the correct location using
// SyntaxHighlighter::setExtraAdditionalFormats.
// It is incremental in the sense that it clears the extra additional formats
// from all lines that have no results between the (from-1).line result and
// the (to-1).line result.
// Requires that results of the Future are ordered by line.
void TEXTEDITOR_EXPORT incrementalApplyExtraAdditionalFormats(
        SyntaxHighlighter *highlighter,
        const QFuture<Result> &future,
        int from, int to,
        const QHash<int, QTextCharFormat> &kindToFormat);

// Cleans the extra additional formats after the last result of the Future
// until the end of the document.
// Requires that results of the Future are ordered by line.
void TEXTEDITOR_EXPORT clearExtraAdditionalFormatsUntilEnd(
        SyntaxHighlighter *highlighter,
        const QFuture<Result> &future);


} // namespace SemanticHighlighter
} // namespace TextEditor

#endif // TEXTEDITOR_SEMANTICHIGHLIGHTER_H
