/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <filesearch.h>

#include <QtTest/QtTest>

bool operator==(const Utils::FileSearchResult &r1, const Utils::FileSearchResult &r2)
{
    return r1.fileName == r2.fileName
            && r1.lineNumber == r2.lineNumber
            && r1.matchingLine == r2.matchingLine
            && r1.matchStart == r2.matchStart
            && r1.matchLength == r2.matchLength
            && r1.regexpCapturedTexts == r2.regexpCapturedTexts;
}

class tst_FileSearch : public QObject
{
    Q_OBJECT

private slots:
    void multipleResults();
    void caseSensitive();
    void caseInSensitive();
};

namespace {
    const char * const FILENAME = ":/tst_filesearch/testfile.txt";

    void test_helper(const Utils::FileSearchResultList &expectedResults,
                     const QString &term,
                     QTextDocument::FindFlags flags)
    {
        Utils::FileIterator *it = new Utils::FileIterator(QStringList() << QLatin1String(FILENAME), QList<QTextCodec *>() << QTextCodec::codecForLocale());
        QFutureWatcher<Utils::FileSearchResultList> watcher;
        QSignalSpy ready(&watcher, SIGNAL(resultsReadyAt(int,int)));
        watcher.setFuture(Utils::findInFiles(term, it, flags));
        watcher.future().waitForFinished();
        QTest::qWait(100); // process events
        QCOMPARE(ready.count(), 1);
        Utils::FileSearchResultList results = watcher.resultAt(0);
        QCOMPARE(results.count(), expectedResults.count());
        for (int i = 0; i < expectedResults.size(); ++i) {
            QCOMPARE(results.at(i), expectedResults.at(i));
        }
    }
}

void tst_FileSearch::multipleResults()
{
    Utils::FileSearchResultList expectedResults;
    expectedResults << Utils::FileSearchResult(QLatin1String(FILENAME), 2, QLatin1String("search to find multiple find results"), 10, 4, QStringList());
    expectedResults << Utils::FileSearchResult(QLatin1String(FILENAME), 2, QLatin1String("search to find multiple find results"), 24, 4, QStringList());
    expectedResults << Utils::FileSearchResult(QLatin1String(FILENAME), 4, QLatin1String("here you find another result"), 9, 4, QStringList());
    test_helper(expectedResults, QLatin1String("find"), QTextDocument::FindFlags(0));
}

void tst_FileSearch::caseSensitive()
{
    Utils::FileSearchResultList expectedResults;
    expectedResults << Utils::FileSearchResult(QLatin1String(FILENAME), 3, QLatin1String("search CaseSensitively for casesensitive"), 7, 13, QStringList());
    test_helper(expectedResults, QLatin1String("CaseSensitive"), QTextDocument::FindCaseSensitively);
}

void tst_FileSearch::caseInSensitive()
{
    Utils::FileSearchResultList expectedResults;
    expectedResults << Utils::FileSearchResult(QLatin1String(FILENAME), 3, QLatin1String("search CaseSensitively for casesensitive"), 7, 13, QStringList());
    expectedResults << Utils::FileSearchResult(QLatin1String(FILENAME), 3, QLatin1String("search CaseSensitively for casesensitive"), 27, 13, QStringList());
    test_helper(expectedResults, QLatin1String("CaseSensitive"), QTextDocument::FindFlags(0));
}

QTEST_MAIN(tst_FileSearch)

#include "tst_filesearch.moc"
