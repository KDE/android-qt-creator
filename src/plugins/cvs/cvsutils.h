/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CVSUTILS_H
#define CVSUTILS_H

#include "cvssubmiteditor.h"

#include <QtCore/QString>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace CVS {
namespace Internal {

// Utilities to parse output of a CVS log.

// A revision of a file.
struct CVS_Revision
{
    CVS_Revision(const QString &rev);

    QString revision;
    QString date; // ISO-Format (YYYY-MM-DD)
    QString commitId;
};

// A log entry consisting of the file and its revisions.
struct CVS_LogEntry
{
    CVS_LogEntry(const QString &file);

    QString file;
    QList<CVS_Revision> revisions;
};

QDebug operator<<(QDebug d, const CVS_LogEntry &);

// Parse. Pass on a directory to obtain full paths when
// running from the repository directory.
QList<CVS_LogEntry> parseLogEntries(const QString &output,
                                    const QString &directory = QString(),
                                    const QString filterCommitId = QString());

// Tortoise CVS outputs unknown files with question marks in
// the diff output on stdout ('? foo'); remove
QString fixDiffOutput(QString d);

// Parse the status output of CVS (stdout/stderr merged
// to catch directories).
typedef QList<CVSSubmitEditor::StateFilePair> StateList;
StateList parseStatusOutput(const QString &directory, const QString &output);

// Revision number utilities: Decrement version number "1.2" -> "1.1"
QString previousRevision(const QString &rev);
// Revision number utilities: Is it "[1.2...].1"?
bool isFirstRevision(const QString &r);

} // namespace Internal
} // namespace CVS

#endif // CVSUTILS_H
