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

#ifndef GNUMAKEPARSER_H
#define GNUMAKEPARSER_H

#include "ioutputparser.h"

#include <QtCore/QRegExp>
#include <QtCore/QStringList>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT GnuMakeParser : public ProjectExplorer::IOutputParser
{
    Q_OBJECT

public:
    explicit GnuMakeParser();

    virtual void stdOutput(const QString &line);
    virtual void stdError(const QString &line);

    virtual void setWorkingDirectory(const QString &workingDirectory);

    QStringList searchDirectories() const;

    bool hasFatalErrors() const;

public slots:
    virtual void taskAdded(const ProjectExplorer::Task &task);

private:
    void addDirectory(const QString &dir);
    void removeDirectory(const QString &dir);

    QRegExp m_makeDir;
    QRegExp m_makeLine;
    QRegExp m_makefileError;

    QStringList m_directories;

#if defined WITH_TESTS
    friend class ProjectExplorerPlugin;
#endif
    bool m_suppressIssues;

    int m_fatalErrorCount;
};

#if defined WITH_TESTS
class GnuMakeParserTester : public QObject
{
    Q_OBJECT

public:
    explicit GnuMakeParserTester(GnuMakeParser *parser, QObject *parent = 0);

    QStringList directories;
    GnuMakeParser *parser;

public slots:
    void parserIsAboutToBeDeleted();
};
#endif

} // namespace ProjectExplorer

#endif // GNUMAKEPARSER_H
