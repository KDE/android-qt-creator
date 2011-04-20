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

#include "ldparser.h"
#include "projectexplorerconstants.h"
#include "taskwindow.h"

#include <QtCore/QDir>

using namespace ProjectExplorer;

namespace {
    // opt. drive letter + filename: (2 brackets)
    const char * const FILE_PATTERN = "(([A-Za-z]:)?[^:]+\\.[^:]+):";
    // line no. or elf segment + offset: (1 bracket)
    const char * const POSITION_PATTERN = "(\\d+|\\(\\.[a-zA-Z0-9]*.0x[a-fA-F0-9]+\\)):";
    const char * const COMMAND_PATTERN = "^(.*[\\\\/])?([a-z0-9]+-[a-z0-9]+-[a-z0-9]+-)?(ld|gold)(-[0-9\\.]+)?(\\.exe)?: ";
}

LdParser::LdParser()
{
    setObjectName(QLatin1String("LdParser"));
    m_regExpLinker.setPattern(QString('^') +
                              QString::fromLatin1(FILE_PATTERN) + '(' +
                              QString::fromLatin1(FILE_PATTERN) + ")?(" +
                              QLatin1String(POSITION_PATTERN) + ")?\\s(.+)$");
    m_regExpLinker.setMinimal(true);

    m_regExpGccNames.setPattern(COMMAND_PATTERN);
    m_regExpGccNames.setMinimal(true);

    m_regExpInFunction.setPattern("^In (static |member )*function ");
    m_regExpInFunction.setMinimal(true);
}

void LdParser::stdError(const QString &line)
{
    QString lne = line.trimmed();
    if (lne.startsWith(QLatin1String("TeamBuilder ")) ||
        lne.startsWith(QLatin1String("distcc["))) {
        IOutputParser::stdError(line);
        return;
    }

    if (lne.startsWith(QLatin1String("collect2:"))) {
        emit addTask(Task(Task::Error,
                          lne /* description */,
                          QString() /* filename */,
                          -1 /* linenumber */,
                          Constants::TASK_CATEGORY_COMPILE));
        return;
    } else if (m_regExpGccNames.indexIn(lne) > -1) {
        QString description = lne.mid(m_regExpGccNames.matchedLength());
        Task task(Task::Error,
                  description,
                  QString(), /* filename */
                  -1, /* line */
                  Constants::TASK_CATEGORY_COMPILE);
        if (description.startsWith(QLatin1String("warning: "))) {
            task.type = Task::Warning;
            task.description = description.mid(9);
        } else if (description.startsWith(QLatin1String("fatal: ")))  {
            task.description = description.mid(7);
        }
        emit addTask(task);
        return;
    } else if (m_regExpLinker.indexIn(lne) > -1) {
        bool ok;
        int lineno = m_regExpLinker.cap(7).toInt(&ok);
        if (!ok)
            lineno = -1;
        QString filename = m_regExpLinker.cap(1);
        if (!m_regExpLinker.cap(4).isEmpty()
            && !m_regExpLinker.cap(4).startsWith(QLatin1String("(.text+0x")))
            filename = m_regExpLinker.cap(4);
        QString description = m_regExpLinker.cap(8).trimmed();
        Task task(Task::Error, description, QDir::fromNativeSeparators(filename), lineno,
                  Constants::TASK_CATEGORY_COMPILE);
        if (m_regExpInFunction.indexIn(description) > -1 ||
            description.startsWith(QLatin1String("At global scope")) ||
            description.startsWith(QLatin1String("At top level")) ||
            description.startsWith(QLatin1String("instantiated from ")) ||
            description.startsWith(QLatin1String("In ")))
            task.type = Task::Unknown;

        emit addTask(task);
        return;
    }
    IOutputParser::stdError(line);
}
