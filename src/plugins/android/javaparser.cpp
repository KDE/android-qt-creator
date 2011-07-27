/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "javaparser.h"

#include <projectexplorer/projectexplorerconstants.h>

using namespace Android::Internal;
using namespace ProjectExplorer;

JavaParser::JavaParser() :
    ProjectExplorer::IOutputParser()
  ,m_javaRegExp("^(.*\\[javac\\]\\s)(.*\\.java):(\\d+):(.*)$")
{
}

void JavaParser::stdOutput(const QString &line)
{
    stdError(line);
}

void JavaParser::stdError(const QString &line)
{
    if (m_javaRegExp.indexIn(line) > -1)
    {
        bool ok;
        int lineno = m_javaRegExp.cap(3).toInt(&ok);
        if (!ok)
            lineno = -1;
        Task task(Task::Error,
                  m_javaRegExp.cap(4).trimmed(),
                  m_javaRegExp.cap(2) /* filename */,
                  lineno,
                  ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
        emit addTask(task);
        return;
    }
    IOutputParser::stdError(line);
}
