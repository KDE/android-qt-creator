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

#include "clangparser.h"
#include "ldparser.h"
#include "taskwindow.h"
#include "projectexplorerconstants.h"

using namespace ProjectExplorer;

namespace {
    // opt. drive letter + filename: (2 brackets)
    const char * const FILE_PATTERN = "(<command line>|([A-Za-z]:)?[^:]+\\.[^:]+)";
}

ClangParser::ClangParser() :
    m_commandRegExp(QLatin1String("^clang(\\+\\+)?: +(fatal +)?(warning|error|note): (.*)$")),
    m_inLineRegExp(QLatin1String("^In (.*) included from (.*):(\\d+):$")),
    m_messageRegExp(QLatin1String("^") + QLatin1String(FILE_PATTERN) + QLatin1String("(:(\\d+):\\d+|\\((\\d+)\\) *): +(fatal +)?(error|warning|note): (.*)$")),
    m_summaryRegExp(QLatin1String("^\\d+ (warnings?|errors?)( and \\d (warnings?|errors?))? generated.$")),
    m_expectSnippet(false)
{
    setObjectName(QLatin1String("ClangParser"));

    appendOutputParser(new LdParser);
}

ClangParser::~ClangParser()
{
    emitTask();
}

void ClangParser::stdError(const QString &line)
{
    const QString lne = line.left(line.count() - 1);
    if (m_summaryRegExp.indexIn(lne) > -1) {
        emitTask();
        m_expectSnippet = false;
        return;
    }

    if (m_commandRegExp.indexIn(lne) > -1) {
        m_expectSnippet = true;
        newTask(Task::Error,
                m_commandRegExp.cap(4),
                QString(), /* filename */
                -1, /* line */
                Constants::TASK_CATEGORY_COMPILE);
        if (m_commandRegExp.cap(3) == QLatin1String("warning"))
            m_currentTask.type = Task::Warning;
        else if (m_commandRegExp.cap(3) == QLatin1String("note"))
            m_currentTask.type = Task::Unknown;
        return;
    }

    if (m_inLineRegExp.indexIn(lne) > -1) {
        m_expectSnippet = true;
        newTask(Task::Unknown,
                lne.trimmed(),
                m_inLineRegExp.cap(2), /* filename */
                m_inLineRegExp.cap(3).toInt(), /* line */
                Constants::TASK_CATEGORY_COMPILE);
        return;
    }

    if (m_messageRegExp.indexIn(lne) > -1) {
        m_expectSnippet = true;
        bool ok = false;
        int lineNo = m_messageRegExp.cap(4).toInt(&ok);
        if (!ok)
            lineNo = m_messageRegExp.cap(5).toInt(&ok);
        newTask(Task::Error,
                m_messageRegExp.cap(8),
                m_messageRegExp.cap(1), /* filename */
                lineNo,
                Constants::TASK_CATEGORY_COMPILE);
        if (m_messageRegExp.cap(7) == "warning")
            m_currentTask.type = Task::Warning;
        else if (m_messageRegExp.cap(7) == "note")
            m_currentTask.type = Task::Unknown;
        return;
    }

    if (m_expectSnippet && !m_currentTask.isNull()) {
        QTextLayout::FormatRange fr;
        fr.start = m_currentTask.description.count() + 1;
        fr.length = lne.count() + 1;
        fr.format.setFontFamily("Monospaced");
        fr.format.setFontStyleHint(QFont::TypeWriter);
        m_currentTask.description.append(QLatin1Char('\n'));
        m_currentTask.description.append(lne);
        m_currentTask.formats.append(fr);
        return;
    }

    IOutputParser::stdError(line);
}

void ClangParser::newTask(Task::TaskType type_, const QString &description_,
                          const QString &file_, int line_, const QString &category_)
{
    emitTask();
    m_currentTask = Task(type_, description_, file_, line_, category_);
}

void ClangParser::emitTask()
{
    if (!m_currentTask.isNull())
        emit addTask(m_currentTask);
    m_currentTask = Task();
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "projectexplorer.h"
#   include "metatypedeclarations.h"
#   include "outputparser_test.h"

void ProjectExplorerPlugin::testClangOutputParser_data()
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

    QTest::newRow("clang++ warning")
            << QString::fromLatin1("clang++: warning: argument unused during compilation: '-mthreads'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("argument unused during compilation: '-mthreads'"),
                        QString(), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("clang++ error")
            << QString::fromLatin1("clang++: error: no input files [err_drv_no_input_files]")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("no input files [err_drv_no_input_files]"),
                        QString(), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("complex warning")
            << QString::fromLatin1("In file included from ..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qnamespace.h:45:\n"
                                   "..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qglobal.h(1425) :  warning: unknown attribute 'dllimport' ignored [-Wunknown-attributes]\n"
                                   "class Q_CORE_EXPORT QSysInfo {\n"
                                   "      ^")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Unknown,
                        QLatin1String("In file included from ..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qnamespace.h:45:"),
                        QLatin1String("..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qnamespace.h"), 45,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Warning,
                        QLatin1String("unknown attribute 'dllimport' ignored [-Wunknown-attributes]\n"
                                      "class Q_CORE_EXPORT QSysInfo {\n"
                                      "      ^"),
                        QLatin1String("..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qglobal.h"), 1425,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();
        QTest::newRow("note")
                << QString::fromLatin1("..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qglobal.h:1289:27: note: instantiated from:\n"
                                       "#    define Q_CORE_EXPORT Q_DECL_IMPORT\n"
                                       "                          ^")
                << OutputParserTester::STDERR
                << QString() << QString()
                << (QList<ProjectExplorer::Task>()
                    << Task(Task::Unknown,
                            QLatin1String("instantiated from:\n"
                                          "#    define Q_CORE_EXPORT Q_DECL_IMPORT\n"
                                          "                          ^"),
                            QLatin1String("..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qglobal.h"), 1289,
                            Constants::TASK_CATEGORY_COMPILE))
                << QString();
        QTest::newRow("fatal error")
                << QString::fromLatin1("/usr/include/c++/4.6/utility:68:10: fatal error: 'bits/c++config.h' file not found\n"
                                       "#include <bits/c++config.h>\n"
                                       "         ^")
                << OutputParserTester::STDERR
                << QString() << QString()
                << (QList<ProjectExplorer::Task>()
                    << Task(Task::Error,
                            QLatin1String("'bits/c++config.h' file not found\n"
                                          "#include <bits/c++config.h>\n"
                                          "         ^"),
                            QLatin1String("/usr/include/c++/4.6/utility"), 68,
                            Constants::TASK_CATEGORY_COMPILE))
                << QString();

        QTest::newRow("line confusion")
                << QString::fromLatin1("/home/code/src/creator/src/plugins/coreplugin/manhattanstyle.cpp:567:51: warning: ?: has lower precedence than +; + will be evaluated first [-Wparentheses]\n"
                                       "            int x = option->rect.x() + horizontal ? 2 : 6;\n"
                                       "                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ^")
                << OutputParserTester::STDERR
                << QString() << QString()
                << (QList<ProjectExplorer::Task>()
                    << Task(Task::Warning,
                            QLatin1String("?: has lower precedence than +; + will be evaluated first [-Wparentheses]\n"
                                          "            int x = option->rect.x() + horizontal ? 2 : 6;\n"
                                          "                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ^"),
                            QLatin1String("/home/code/src/creator/src/plugins/coreplugin/manhattanstyle.cpp"), 567,
                            Constants::TASK_CATEGORY_COMPILE))
                << QString();
}

void ProjectExplorerPlugin::testClangOutputParser()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new ClangParser);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(QList<Task>, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);

    testbench.testParsing(input, inputChannel,
                          tasks, childStdOutLines, childStdErrLines,
                          outputLines);
}
#endif
