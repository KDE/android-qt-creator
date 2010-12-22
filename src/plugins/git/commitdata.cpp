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

#include "commitdata.h"
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QRegExp>

const char *const kBranchIndicatorC = "# On branch";

namespace Git {
namespace Internal {

void GitSubmitEditorPanelInfo::clear()
{
    repository.clear();
    description.clear();
    branch.clear();
}

QDebug operator<<(QDebug d, const GitSubmitEditorPanelInfo &data)
{
    d.nospace() << "Rep: " << data.repository << " Descr: " << data.description
        << " branch: " << data.branch;
    return d;
}

void GitSubmitEditorPanelData::clear()
{
    author.clear();
    email.clear();
}

QString GitSubmitEditorPanelData::authorString() const
{
    QString rc;
    rc += author;

    if (email.isEmpty())
        return rc;

    rc += QLatin1String(" <");
    rc += email;
    rc += QLatin1Char('>');
    return rc;
}

QDebug operator<<(QDebug d, const GitSubmitEditorPanelData &data)
{
    d.nospace() << " author:" << data.author << " email: " << data.email;
    return d;
}

void CommitData::clear()
{
    panelInfo.clear();
    panelData.clear();
    amendSHA1.clear();

    stagedFiles.clear();
    unstagedFiles.clear();
    untrackedFiles.clear();
}

// Split a state/file spec from git status output
// '#<tab>modified:<blanks>git .pro'
// into state and file ('modified', 'git .pro').
CommitData::StateFilePair splitStateFileSpecification(const QString &line)
{
    QPair<QString, QString> rc;
    const int statePos = 2;
    const int colonIndex = line.indexOf(QLatin1Char(':'), statePos);
    if (colonIndex == -1)
        return rc;
    rc.first = line.mid(statePos, colonIndex - statePos);
    int filePos = colonIndex + 1;
    const QChar blank = QLatin1Char(' ');
    while (line.at(filePos) == blank)
        filePos++;
    if (filePos < line.size())
        rc.second = line.mid(filePos, line.size() - filePos);
    return rc;
}

// Convenience to add a state/file spec to a list
static inline bool addStateFileSpecification(const QString &line, QList<CommitData::StateFilePair> *list)
{
    const CommitData::StateFilePair sf = splitStateFileSpecification(line);
    if (sf.first.isEmpty() || sf.second.isEmpty())
        return false;
    list->push_back(sf);
    return true;
}

/* Parse a git status file list:
 * \code
    # Changes to be committed:
    #<tab>modified:<blanks>git.pro
    # Changed but not updated:
    #<tab>modified:<blanks>git.pro
    # Untracked files:
    #<tab>git.pro
    \endcode
*/

bool CommitData::filesEmpty() const
{
    return stagedFiles.empty() && unstagedFiles.empty() && untrackedFiles.empty();
}

bool CommitData::parseFilesFromStatus(const QString &output)
{
    enum State { None, CommitFiles, NotUpdatedFiles, UntrackedFiles };

    const QStringList lines = output.split(QLatin1Char('\n'));
    const QString branchIndicator = QLatin1String(kBranchIndicatorC);
    const QString commitIndicator = QLatin1String("# Changes to be committed:");
    const QString notUpdatedIndicator = QLatin1String("# Changed but not updated:");
    const QString untrackedIndicator = QLatin1String("# Untracked files:");

    State s = None;
    // Match added/changed-not-updated files: "#<tab>modified: foo.cpp"
    QRegExp filesPattern(QLatin1String("#\\t[^:]+:\\s+.+"));
    QTC_ASSERT(filesPattern.isValid(), return false);

    const QStringList::const_iterator cend = lines.constEnd();
    for (QStringList::const_iterator it =  lines.constBegin(); it != cend; ++it) {
        QString line = *it;
        if (line.startsWith(branchIndicator)) {
            panelInfo.branch = line.mid(branchIndicator.size() + 1);
            continue;
        }
        if (line.startsWith(commitIndicator)) {
            s = CommitFiles;
            continue;
        }
        if (line.startsWith(notUpdatedIndicator)) {
            s = NotUpdatedFiles;
            continue;
        }
        if (line.startsWith(untrackedIndicator)) {
            // Now match untracked: "#<tab>foo.cpp"
            s = UntrackedFiles;
            filesPattern = QRegExp(QLatin1String("#\\t.+"));
            QTC_ASSERT(filesPattern.isValid(), return false);
            continue;
        }
        if (filesPattern.exactMatch(line)) {
            switch (s) {
            case CommitFiles:
                addStateFileSpecification(line, &stagedFiles);
                break;
            case NotUpdatedFiles:
                // skip submodules:
                if (line.endsWith(QLatin1String(" (modified content)"))
                        || line.endsWith(" (new commits)"))
                    line = line.left(line.lastIndexOf(QLatin1Char('(')) - 1);
                addStateFileSpecification(line, &unstagedFiles);
                break;
            case UntrackedFiles:
                untrackedFiles.push_back(line.mid(2).trimmed());
                break;
            case None:
                break;
            }
        }
    }
    return true;
}

// Convert a spec pair list to a list of file names, optionally
// filter for a state
static QStringList specToFileNames(const QList<CommitData::StateFilePair> &files,
                                   const QString &stateFilter)
{
    typedef QList<CommitData::StateFilePair>::const_iterator ConstIterator;
    if (files.empty())
        return QStringList();
    const bool emptyFilter = stateFilter.isEmpty();
    QStringList rc;
    const ConstIterator cend = files.constEnd();
    for (ConstIterator it = files.constBegin(); it != cend; ++it)
        if (emptyFilter || stateFilter == it->first)
            rc.push_back(it->second);
    return rc;
}

QStringList CommitData::stagedFileNames(const QString &stateFilter) const
{
    return specToFileNames(stagedFiles, stateFilter);
}

QStringList CommitData::unstagedFileNames(const QString &stateFilter) const
{
    return specToFileNames(unstagedFiles, stateFilter);
}

QDebug operator<<(QDebug d, const CommitData &data)
{
    d <<  data.panelInfo << data.panelData;
    d.nospace() << "Commit: " << data.stagedFiles << " Not updated: "
        << data.unstagedFiles << " Untracked: " << data.untrackedFiles;
    return d;
}

} // namespace Internal
} // namespace Git
