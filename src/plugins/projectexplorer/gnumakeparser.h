/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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
    GnuMakeParserTester(GnuMakeParser *parser, QObject *parent = 0);

    QStringList directories;
    GnuMakeParser *parser;

public slots:
    void parserIsAboutToBeDeleted();
};
#endif

} // namespace ProjectExplorer

#endif // GNUMAKEPARSER_H
