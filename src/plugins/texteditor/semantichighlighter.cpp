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

#include "semantichighlighter.h"

#include "syntaxhighlighter.h"

#include <utils/qtcassert.h>

#include <QtGui/QTextDocument>
#include <QtGui/QTextBlock>

using namespace TextEditor;
using namespace TextEditor::SemanticHighlighter;

void TextEditor::SemanticHighlighter::incrementalApplyExtraAdditionalFormats(
        SyntaxHighlighter *highlighter,
        const QFuture<Result> &future,
        int from, int to,
        const QHash<int, QTextCharFormat> &kindToFormat)
{
    if (to <= from)
        return;

    const int firstResultBlockNumber = future.resultAt(from).line - 1;

    // blocks between currentBlockNumber and the last block with results will
    // be cleaned of additional extra formats if they have no results
    int currentBlockNumber = 0;
    for (int i = from - 1; i >= 0; --i) {
        const Result &result = future.resultAt(i);
        const int blockNumber = result.line - 1;
        if (blockNumber < firstResultBlockNumber) {
            // stop! found where last format stopped
            currentBlockNumber = blockNumber + 1;
            // add previous results for the same line to avoid undoing their formats
            from = i + 1;
            break;
        }
    }

    QTextDocument *doc = highlighter->document();
    QTC_ASSERT(currentBlockNumber < doc->blockCount(), return);
    QTextBlock b = doc->findBlockByNumber(currentBlockNumber);

    Result result = future.resultAt(from);
    for (int i = from; i < to && b.isValid(); ) {
        const int blockNumber = result.line - 1;
        QTC_ASSERT(blockNumber < doc->blockCount(), return);

        // clear formats of blocks until blockNumber
        while (currentBlockNumber < blockNumber) {
            highlighter->setExtraAdditionalFormats(b, QList<QTextLayout::FormatRange>());
            b = b.next();
            ++currentBlockNumber;
        }

        // collect all the formats for the current line
        QList<QTextLayout::FormatRange> formats;
        forever {
            QTextLayout::FormatRange formatRange;

            formatRange.format = kindToFormat.value(result.kind);
            if (formatRange.format.isValid()) {
                formatRange.start = result.column - 1;
                formatRange.length = result.length;
                formats.append(formatRange);
            }

            ++i;
            if (i >= to)
                break;
            result = future.resultAt(i);
            const int nextBlockNumber = result.line - 1;
            if (nextBlockNumber != blockNumber)
                break;
        }
        highlighter->setExtraAdditionalFormats(b, formats);
        b = b.next();
        ++currentBlockNumber;
    }
}

void TextEditor::SemanticHighlighter::clearExtraAdditionalFormatsUntilEnd(
        SyntaxHighlighter *highlighter,
        const QFuture<Result> &future)
{
    // find block number of last result
    int lastBlockNumber = 0;
    for (int i = future.resultCount() - 1; i >= 0; --i) {
        const Result &result = future.resultAt(i);
        if (result.line) {
            lastBlockNumber = result.line - 1;
            break;
        }
    }

    QTextDocument *doc = highlighter->document();

    const int firstBlockToClear = lastBlockNumber + 1;
    if (firstBlockToClear == doc->blockCount())
        return;
    QTC_ASSERT(firstBlockToClear < doc->blockCount(), return);

    QTextBlock b = doc->findBlockByNumber(firstBlockToClear);

    while (b.isValid()) {
        highlighter->setExtraAdditionalFormats(b, QList<QTextLayout::FormatRange>());
        b = b.next();
    }
}
