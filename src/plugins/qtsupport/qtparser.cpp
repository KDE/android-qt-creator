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

#include "qtparser.h"

#include <projectexplorer/task.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>

using namespace QtSupport;
using ProjectExplorer::Task;

// opt. drive letter + filename: (2 brackets)
#define FILE_PATTERN "^(([A-Za-z]:)?[^:]+\\.[^:]+)"

QtParser::QtParser() :
    m_mocRegExp(QLatin1String(FILE_PATTERN"[:\\(](\\d+)\\)?:\\s(Warning|Error):\\s(.+)$"))
{
    setObjectName(QLatin1String("QtParser"));
    m_mocRegExp.setMinimal(true);
}

void QtParser::stdError(const QString &line)
{
    QString lne(line.trimmed());
    if (m_mocRegExp.indexIn(lne) > -1) {
        bool ok;
        int lineno = m_mocRegExp.cap(3).toInt(&ok);
        if (!ok)
            lineno = -1;
        Task task(Task::Error,
                  m_mocRegExp.cap(5).trimmed(),
                  m_mocRegExp.cap(1) /* filename */,
                  lineno,
                  ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
        if (m_mocRegExp.cap(4) == QLatin1String("Warning"))
            task.type = Task::Warning;
        emit addTask(task);
        return;
    }
    IOutputParser::stdError(line);
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "qtsupportplugin.h"
#   include <projectexplorer/projectexplorerconstants.h>
#   include <projectexplorer/metatypedeclarations.h>
#   include <projectexplorer/outputparser_test.h>

using namespace ProjectExplorer;
using namespace QtSupport::Internal;

void QtSupportPlugin::testQtOutputParser_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<ProjectExplorer::Task> >("tasks");
    QTest::addColumn<QString>("outputLines");


    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext\n") << QString()
            << QList<ProjectExplorer::Task>()
            << QString();
    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext\n")
            << QList<ProjectExplorer::Task>()
            << QString();
    QTest::newRow("pass-through gcc infos")
            << QString::fromLatin1("/temp/test/untitled8/main.cpp: In function `int main(int, char**)':\n"
                                   "../../scriptbug/main.cpp: At global scope:\n"
                                   "../../scriptbug/main.cpp: In instantiation of void bar(i) [with i = double]:\n"
                                   "../../scriptbug/main.cpp:8: instantiated from void foo(i) [with i = double]\n"
                                   "../../scriptbug/main.cpp:22: instantiated from here")
            << OutputParserTester::STDERR
            << QString()
            << QString::fromLatin1("/temp/test/untitled8/main.cpp: In function `int main(int, char**)':\n"
                                   "../../scriptbug/main.cpp: At global scope:\n"
                                   "../../scriptbug/main.cpp: In instantiation of void bar(i) [with i = double]:\n"
                                   "../../scriptbug/main.cpp:8: instantiated from void foo(i) [with i = double]\n"
                                   "../../scriptbug/main.cpp:22: instantiated from here\n")
            << QList<ProjectExplorer::Task>()
            << QString();
    QTest::newRow("moc warning")
            << QString::fromLatin1("..\\untitled\\errorfile.h:0: Warning: No relevant classes found. No output generated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Warning,
                                                       QLatin1String("No relevant classes found. No output generated."),
                                                       QLatin1String("..\\untitled\\errorfile.h"), 0,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("moc warning 2")
            << QString::fromLatin1("c:\\code\\test.h(96): Warning: Property declaration ) has no READ accessor function. The property will be invalid.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Warning,
                                                       QLatin1String("Property declaration ) has no READ accessor function. The property will be invalid."),
                                                       QLatin1String("c:\\code\\test.h"), 96,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE))
            << QString();
}

void QtSupportPlugin::testQtOutputParser()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new QtParser);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(QList<Task>, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);

    testbench.testParsing(input, inputChannel, tasks, childStdOutLines, childStdErrLines, outputLines);
}
#endif
