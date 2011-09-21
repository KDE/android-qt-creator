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

#include "msvcparser.h"
#include "projectexplorerconstants.h"

static const char FILE_POS_PATTERN[] = "(cl|LINK|.+) : ";
static const char ERROR_PATTERN[] = "[A-Z]+\\d\\d\\d\\d ?:";

static QPair<QString, int> parseFileName(const QString &input)
{
    QString fileName = input;
    if (fileName.startsWith(QLatin1String("LINK"))
            || fileName.startsWith(QLatin1String("cl")))
        return qMakePair(QString(), -1);

    // Extract linenumber (if it is there):
    int linenumber = -1;
    if (fileName.endsWith(QLatin1Char(')'))) {
        int pos = fileName.lastIndexOf(QLatin1Char('('));
        if (pos >= 0) {
            bool ok = false;
            int n = fileName.mid(pos + 1, fileName.count() - pos - 2).toInt(&ok);
            if (ok) {
                fileName = fileName.left(pos);
                linenumber = n;
            }
        }
    }
    return qMakePair(fileName, linenumber);
}

using namespace ProjectExplorer;

MsvcParser::MsvcParser()
{
    setObjectName(QLatin1String("MsvcParser"));
    m_compileRegExp.setPattern(QString::fromLatin1("^") + QLatin1String(FILE_POS_PATTERN)
                               + QLatin1String("(Command line |fatal )?(warning|error) (")
                               + QLatin1String(ERROR_PATTERN) + QLatin1String(".*)$"));
    m_compileRegExp.setMinimal(true);
    m_additionalInfoRegExp.setPattern(QString::fromLatin1("^        (.*)\\((\\d+)\\) : (.*)$"));
    m_additionalInfoRegExp.setMinimal(true);
}

MsvcParser::~MsvcParser()
{
    sendQueuedTask();
}

void MsvcParser::stdOutput(const QString &line)
{
    int infoPos = m_additionalInfoRegExp.indexIn(line);
    if (line.startsWith(QLatin1String("        ")) && infoPos < 0) {
        if (m_lastTask.isNull())
            return;

        m_lastTask.description.append(QChar('\n'));
        m_lastTask.description.append(line.mid(8));
        // trim trailing spaces:
        int i = 0;
        for (i = m_lastTask.description.length() - 1; i >= 0; --i) {
            if (!m_lastTask.description.at(i).isSpace())
                break;
        }
        m_lastTask.description.truncate(i + 1);

        if (m_lastTask.formats.isEmpty()) {
            QTextLayout::FormatRange fr;
            fr.start = m_lastTask.description.indexOf('\n') + 1;
            fr.length = m_lastTask.description.length() - fr.start;
            fr.format.setFontItalic(true);
            m_lastTask.formats.append(fr);
        } else {
            m_lastTask.formats[0].length = m_lastTask.description.length() - m_lastTask.formats[0].start;
        }
        return;
    }

    if (processCompileLine(line))
        return;
    if (infoPos > -1) {
        m_lastTask = Task(Task::Unknown,
                          m_additionalInfoRegExp.cap(3).trimmed(), /* description */
                          m_additionalInfoRegExp.cap(1), /* fileName */
                          m_additionalInfoRegExp.cap(2).toInt(), /* linenumber */
                          Constants::TASK_CATEGORY_COMPILE);
        return;
    }
    IOutputParser::stdOutput(line);
}

void MsvcParser::stdError(const QString &line)
{
    if (processCompileLine(line))
        return;
    IOutputParser::stdError(line);
}

bool MsvcParser::processCompileLine(const QString &line)
{
    sendQueuedTask();

    if (m_compileRegExp.indexIn(line) > -1) {
        QPair<QString, int> position = parseFileName( m_compileRegExp.cap(1));
        m_lastTask = Task(Task::Unknown,
                          m_compileRegExp.cap(4).trimmed() /* description */,
                          position.first, position.second,
                          Constants::TASK_CATEGORY_COMPILE);
        if (m_compileRegExp.cap(3) == QLatin1String("warning"))
            m_lastTask.type = Task::Warning;
        else if (m_compileRegExp.cap(3) == QLatin1String("error"))
            m_lastTask.type = Task::Error;

        return true;
    }
    return false;
}

void MsvcParser::sendQueuedTask()
{
    if (m_lastTask.isNull())
        return;

    addTask(m_lastTask);
    m_lastTask = Task();
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "projectexplorer.h"

#   include "projectexplorer/outputparser_test.h"

using namespace ProjectExplorer::Internal;

void ProjectExplorerPlugin::testMsvcOutputParsers_data()
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

    QTest::newRow("labeled error")
            << QString::fromLatin1("qmlstandalone\\main.cpp(54) : error C4716: 'findUnresolvedModule' : must return a value") << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Error,
                                                       QLatin1String("C4716: 'findUnresolvedModule' : must return a value"),
                                                       QLatin1String("qmlstandalone\\main.cpp"), 54,
                                                       QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();

    QTest::newRow("labeled warning")
            << QString::fromLatin1("x:\\src\\plugins\\projectexplorer\\msvcparser.cpp(69) : warning C4100: 'something' : unreferenced formal parameter") << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Warning,
                                                       QLatin1String("C4100: 'something' : unreferenced formal parameter"),
                                                       QLatin1String("x:\\src\\plugins\\projectexplorer\\msvcparser.cpp"), 69,
                                                       QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();

    QTest::newRow("additional information")
            << QString::fromLatin1("x:\\src\\plugins\\texteditor\\icompletioncollector.h(50) : warning C4099: 'TextEditor::CompletionItem' : type name first seen using 'struct' now seen using 'class'\n"
                                   "        x:\\src\\plugins\\texteditor\\completionsupport.h(39) : see declaration of 'TextEditor::CompletionItem'")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("C4099: 'TextEditor::CompletionItem' : type name first seen using 'struct' now seen using 'class'"),
                        QLatin1String("x:\\src\\plugins\\texteditor\\icompletioncollector.h"), 50,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE))
                << Task(Task::Unknown,
                        QLatin1String("see declaration of 'TextEditor::CompletionItem'"),
                        QLatin1String("x:\\src\\plugins\\texteditor\\completionsupport.h"), 39,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();

    QTest::newRow("fatal linker error")
            << QString::fromLatin1("LINK : fatal error LNK1146: no argument specified with option '/LIBPATH:'")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("LNK1146: no argument specified with option '/LIBPATH:'"),
                        QString(), -1,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();

    // This actually comes through stderr!
    QTest::newRow("command line warning")
            << QString::fromLatin1("cl : Command line warning D9002 : ignoring unknown option '-fopenmp'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("D9002 : ignoring unknown option '-fopenmp'"),
                        QString(), -1,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();
    QTest::newRow("complex error")
            << QString::fromLatin1("..\\untitled\\main.cpp(19) : error C2440: 'initializing' : cannot convert from 'int' to 'std::_Tree<_Traits>::iterator'\n"
                                   "        with\n"
                                   "        [\n"
                                   "            _Traits=std::_Tmap_traits<int,double,std::less<int>,std::allocator<std::pair<const int,double>>,false>\n"
                                   "        ]\n"
                                   "        No constructor could take the source type, or constructor overload resolution was ambiguous")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("C2440: 'initializing' : cannot convert from 'int' to 'std::_Tree<_Traits>::iterator'\n"
                                      "with\n"
                                      "[\n"
                                      "    _Traits=std::_Tmap_traits<int,double,std::less<int>,std::allocator<std::pair<const int,double>>,false>\n"
                                      "]\n"
                                      "No constructor could take the source type, or constructor overload resolution was ambiguous"),
                        QLatin1String("..\\untitled\\main.cpp"), 19,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();
    QTest::newRow("Linker error 1")
            << QString::fromLatin1("main.obj : error LNK2019: unresolved external symbol \"public: void __thiscall Data::doit(void)\" (?doit@Data@@QAEXXZ) referenced in function _main")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("LNK2019: unresolved external symbol \"public: void __thiscall Data::doit(void)\" (?doit@Data@@QAEXXZ) referenced in function _main"),
                        QLatin1String("main.obj"), -1,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();
    QTest::newRow("Linker error 2")
            << QString::fromLatin1("debug\\Experimentation.exe : fatal error LNK1120: 1 unresolved externals")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("LNK1120: 1 unresolved externals"),
                        QLatin1String("debug\\Experimentation.exe"), -1,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();
    QTest::newRow("Multiline error")
            << QString::fromLatin1("c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\INCLUDE\\xutility(2227) : warning C4996: 'std::_Copy_impl': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'\n"
                                   "        c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\INCLUDE\\xutility(2212) : see declaration of 'std::_Copy_impl'\n"
                                   "        symbolgroupvalue.cpp(2314) : see reference to function template instantiation '_OutIt std::copy<const unsigned char*,unsigned short*>(_InIt,_InIt,_OutIt)' being compiled\n"
                                   "        with\n"
                                   "        [\n"
                                   "            _OutIt=unsigned short *,\n"
                                   "            _InIt=const unsigned char *\n"
                                   "        ]")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("C4996: 'std::_Copy_impl': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'"),
                        QLatin1String("c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\INCLUDE\\xutility"), 2227,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE))
                << Task(Task::Unknown,
                        QLatin1String("see declaration of 'std::_Copy_impl'"),
                        QLatin1String("c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\INCLUDE\\xutility"), 2212,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE))
                << Task(Task::Unknown,
                        QLatin1String("see reference to function template instantiation '_OutIt std::copy<const unsigned char*,unsigned short*>(_InIt,_InIt,_OutIt)' being compiled\n"
                                      "with\n"
                                      "[\n"
                                      "    _OutIt=unsigned short *,\n"
                                      "    _InIt=const unsigned char *\n"
                                      "]"),
                        QLatin1String("symbolgroupvalue.cpp"), 2314,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();
}

void ProjectExplorerPlugin::testMsvcOutputParsers()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new MsvcParser);
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

