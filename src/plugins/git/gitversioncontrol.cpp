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

#include "gitversioncontrol.h"
#include "gitclient.h"
#include "gitplugin.h"
#include "gitutils.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>

static const char stashMessageKeywordC[] = "IVersionControl@";
static const char stashRevisionIdC[] = "revision";

namespace Git {
namespace Internal {

static inline GitClient *gitClient()
{
    return GitPlugin::instance()->gitClient();
}

GitVersionControl::GitVersionControl(GitClient *client) :
    m_enabled(true),
    m_client(client)
{
}

QString GitVersionControl::displayName() const
{
    return QLatin1String("git");
}

// Add: Implement using "git add --intent-to-add" starting from 1.6.1
static inline bool addOperationSupported()
{
    return gitClient()->gitVersion(true) >= version(1, 6, 1);
}

bool GitVersionControl::supportsOperation(Operation operation) const
{
    bool rc = false;
    switch (operation) {
    case AddOperation:
        rc = addOperationSupported();
        break;
    case DeleteOperation:
        rc = true;
        break;
    case MoveOperation:
        rc = true;
        break;
    case OpenOperation:
        break;
    case CreateRepositoryOperation:
    case SnapshotOperations:
        rc = true;
        break;
    case AnnotateOperation:
        rc = true;
        break;
    case CheckoutOperation:
    case GetRepositoryRootOperation:
        rc = true;
        break;
    }
    return rc;
}

bool GitVersionControl::vcsOpen(const QString & /*fileName*/)
{
    return false;
}

bool GitVersionControl::vcsAdd(const QString & fileName)
{
    // Implement in terms of using "--intent-to-add"
    QTC_ASSERT(addOperationSupported(), return false);
    const QFileInfo fi(fileName);
    return gitClient()->synchronousAdd(fi.absolutePath(), true, QStringList(fi.fileName()));
}

bool GitVersionControl::vcsDelete(const QString & fileName)
{
    const QFileInfo fi(fileName);
    return gitClient()->synchronousDelete(fi.absolutePath(), true, QStringList(fi.fileName()));
}

bool GitVersionControl::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo fromInfo(from);
    const QFileInfo toInfo(to);
    return gitClient()->synchronousMove(fromInfo.absolutePath(), fromInfo.absoluteFilePath(), toInfo.absoluteFilePath());
}

bool GitVersionControl::vcsCreateRepository(const QString &directory)
{
    return gitClient()->synchronousInit(directory);
}

bool GitVersionControl::vcsCheckout(const QString &directory, const QByteArray &url)
{
    return gitClient()->cloneRepository(directory,url);
}

QString GitVersionControl::vcsGetRepositoryURL(const QString &directory)
{
    return gitClient()->vcsGetRepositoryURL(directory);
}

/* Snapshots are implement using stashes, relying on stash messages for
 * naming as the actual stash names (stash{n}) are rotated as one adds stashes.
 * Note that the snapshot interface does not care whether we have an unmodified
 * repository state, in which case git refuses to stash.
 * In that case, return a special identifier as "specialprefix:<branch>:<head revision>",
 * which will trigger a checkout in restore(). */

QString GitVersionControl::vcsCreateSnapshot(const QString &topLevel)
{
    bool repositoryUnchanged;
    // Create unique keyword
    static int n = 1;
    QString keyword = QLatin1String(stashMessageKeywordC) + QString::number(n++);
    const QString stashMessage =
            gitClient()->synchronousStash(topLevel, keyword,
                                          GitClient::StashImmediateRestore|GitClient::StashIgnoreUnchanged,
                                          &repositoryUnchanged);
    if (!stashMessage.isEmpty())
        return stashMessage;
    if (repositoryUnchanged) {
        // For unchanged repository state: return identifier + top revision
        QString topRevision;
        QString branch;
        if (!gitClient()->synchronousTopRevision(topLevel, &topRevision, &branch))
            return QString();
        const QChar colon = QLatin1Char(':');
        QString id = QLatin1String(stashRevisionIdC);
        id += colon;
        id += branch;
        id += colon;
        id += topRevision;
        return id;
    }
    return QString(); // Failure
}

QStringList GitVersionControl::vcsSnapshots(const QString &topLevel)
{
    QList<Stash> stashes;
    if (!gitClient()->synchronousStashList(topLevel, &stashes))
        return QStringList();
    // Return the git stash 'message' as identifier, ignoring empty ones
    QStringList rc;
    foreach(const Stash &s, stashes)
        if (!s.message.isEmpty())
            rc.push_back(s.message);
    return rc;
}

bool GitVersionControl::vcsRestoreSnapshot(const QString &topLevel, const QString &name)
{
    bool success = false;
    do {
        // Is this a revision or a stash
        if (name.startsWith(QLatin1String(stashRevisionIdC))) {
            // Restore "id:branch:revision"
            const QStringList tokens = name.split(QLatin1Char(':'));
            if (tokens.size() != 3)
                break;
            const QString branch = tokens.at(1);
            const QString revision = tokens.at(2);
            success = gitClient()->synchronousReset(topLevel)
                      && gitClient()->synchronousCheckoutBranch(topLevel, branch)
                      && gitClient()->synchronousCheckoutFiles(topLevel, QStringList(), revision);
        } else {
            // Restore stash if it can be resolved.
            QString stashName;
            success = gitClient()->stashNameFromMessage(topLevel, name, &stashName)
                      && gitClient()->synchronousReset(topLevel)
                      && gitClient()->synchronousStashRestore(topLevel, stashName);
        }
    }  while (false);
    return success;
}

bool GitVersionControl::vcsRemoveSnapshot(const QString &topLevel, const QString &name)
{
    // Is this a revision -> happy
    if (name.startsWith(QLatin1String(stashRevisionIdC)))
        return true;
    QString stashName;
    return gitClient()->stashNameFromMessage(topLevel, name, &stashName)
            && gitClient()->synchronousStashRemove(topLevel, stashName);
}

bool GitVersionControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    const QString topLevelFound = GitClient::findRepositoryForDirectory(directory);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool GitVersionControl::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    gitClient()->blame(fi.absolutePath(), QStringList(), fi.fileName(), QString(), line);
    return true;
}

void GitVersionControl::emitFilesChanged(const QStringList &l)
{
    emit filesChanged(l);
}

void GitVersionControl::emitRepositoryChanged(const QString &r)
{
    emit repositoryChanged(r);
}

} // Internal
} // Git
