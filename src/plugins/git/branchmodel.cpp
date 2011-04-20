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

#include "branchmodel.h"
#include "gitclient.h"

#include <QtGui/QFont>
#include <QtCore/QDebug>
#include <QtCore/QRegExp>
#include <QtCore/QTimer>

enum { debug = 0 };

namespace Git {
    namespace Internal {

// Parse a branch line: " *name sha description".
bool RemoteBranchModel::Branch::parse(const QString &lineIn, bool *isCurrent)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << lineIn;

    *isCurrent = lineIn.startsWith(QLatin1String("* "));
    if (lineIn.size() < 3)
        return false;

    const QString branchInfo = lineIn.mid(2);
    QStringList tokens;
    if (*isCurrent && branchInfo.startsWith(QLatin1String("(no branch)")))
        return false;
    else
        tokens = branchInfo.split(QLatin1Char(' '), QString::SkipEmptyParts);
    if (tokens.size() < 2)
        return false;
    name = tokens.at(0);
    currentSHA= tokens.at(1);
    toolTip.clear();
    return true;
}

// ------ RemoteBranchModel
RemoteBranchModel::RemoteBranchModel(GitClient *client, QObject *parent) :
    QAbstractListModel(parent),
    m_flags(Qt::ItemIsSelectable|Qt::ItemIsEnabled),
    m_client(client)
{
}

bool RemoteBranchModel::refresh(const QString &workingDirectory, QString *errorMessage)
{
    int currentBranch;
    return refreshBranches(workingDirectory, true, &currentBranch, errorMessage);
}

QString RemoteBranchModel::branchName(int row) const
{
    return m_branches.at(row).name;
}

QString RemoteBranchModel::workingDirectory() const
{
    return m_workingDirectory;
}

int RemoteBranchModel::branchCount() const
{
    return m_branches.size();
}

int RemoteBranchModel::rowCount(const QModelIndex & /* parent */) const
{
    return branchCount();
}

QVariant RemoteBranchModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    switch (role) {
        case Qt::DisplayRole:
        return branchName(row);
        case Qt::ToolTipRole:
        if (m_branches.at(row).toolTip.isEmpty())
            m_branches.at(row).toolTip = toolTip(m_branches.at(row).currentSHA);
        return m_branches.at(row).toolTip;
        break;
        default:
        break;
    }
    return QVariant();
}

Qt::ItemFlags RemoteBranchModel::flags(const QModelIndex & /* index */) const
{
    return m_flags;
}

QString RemoteBranchModel::toolTip(const QString &sha) const
{
    // Show the sha description excluding diff as toolTip
    QString output;
    QString errorMessage;
    if (!m_client->synchronousShow(m_workingDirectory, sha, &output, &errorMessage))
        return errorMessage;
    // Remove 'diff' output
    const int diffPos = output.indexOf(QLatin1String("\ndiff --"));
    if (diffPos != -1)
        output.remove(diffPos, output.size() - diffPos);
    return output;
}

bool RemoteBranchModel::runGitBranchCommand(const QString &workingDirectory, const QStringList &additionalArgs, QString *output, QString *errorMessage)
{
    return m_client->synchronousBranchCmd(workingDirectory, additionalArgs, output, errorMessage);
}

void RemoteBranchModel::clear()
{
    if (!m_branches.isEmpty()) {
        m_branches.clear();
        reset();
    }
}

bool RemoteBranchModel::refreshBranches(const QString &workingDirectory, bool remoteBranches,
                                        int *currentBranch, QString *errorMessage)
{
    // Run branch command with verbose.
    QStringList branchArgs;
    branchArgs << QLatin1String(GitClient::noColorOption) << QLatin1String("-v");
    QString output;
    *currentBranch = -1;
    if (remoteBranches)
        branchArgs.push_back(QLatin1String("-r"));
    if (!runGitBranchCommand(workingDirectory, branchArgs, &output, errorMessage))
        return false;
    if (debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << output;
    // Parse output
    m_workingDirectory = workingDirectory;
    m_branches.clear();
    const QStringList branches = output.split(QLatin1Char('\n'));
    const int branchCount = branches.size();
    bool isCurrent;
    for (int b = 0; b < branchCount; b++) {
        Branch newBranch;
        if (newBranch.parse(branches.at(b), &isCurrent)) {
            m_branches.push_back(newBranch);
            if (isCurrent)
                *currentBranch = b;
        }
    }
    reset();
    return true;
}

int RemoteBranchModel::findBranchByName(const QString &name) const
{
    const int count = branchCount();
    for (int i = 0; i < count; i++)
        if (branchName(i) == name)
            return i;
    return -1;
}

// --- LocalBranchModel
LocalBranchModel::LocalBranchModel(GitClient *client, QObject *parent) :
    RemoteBranchModel(client, parent),
    m_typeHere(tr("<New branch>")),
    m_typeHereToolTip(tr("Type to create a new branch")),
    m_currentBranch(-1)
{
}

int LocalBranchModel::currentBranch() const
{
    return m_currentBranch;
}

bool LocalBranchModel::isNewBranchRow(int row) const
{
    return row >= branchCount();
}

Qt::ItemFlags LocalBranchModel::flags(const QModelIndex & index) const
{
    if (isNewBranchRow(index))
        return Qt::ItemIsEditable|Qt::ItemIsSelectable|Qt::ItemIsEnabled| Qt::ItemIsUserCheckable;
    return RemoteBranchModel::flags(index) | Qt::ItemIsUserCheckable;
}

int LocalBranchModel::rowCount(const QModelIndex & /* parent */) const
{
    return branchCount() + 1;
}

QVariant LocalBranchModel::data(const QModelIndex &index, int role) const
{
    if (isNewBranchRow(index)) {
        switch (role) {
        case Qt::DisplayRole:
            return m_typeHere;
        case Qt::ToolTipRole:
            return m_typeHereToolTip;
        }
        return QVariant();
    }

    if (role == Qt::FontRole && index.row() == m_currentBranch) {
        QFont font = RemoteBranchModel::data(index, role).value<QFont>();
        font.setBold(true);
        font.setUnderline(true);
        return font;
    }
    return RemoteBranchModel::data(index, role);
}

void LocalBranchModel::clear()
{
    m_currentBranch = -1;
    m_newBranch.clear();
    RemoteBranchModel::clear();
}

bool LocalBranchModel::refresh(const QString &workingDirectory, QString *errorMessage)
{
    return refreshBranches(workingDirectory, false, &m_currentBranch, errorMessage);
}

bool LocalBranchModel::checkNewBranchName(const QString &name) const
{
    // Syntax
    const QRegExp pattern(QLatin1String("[a-zA-Z0-9-_]+"));
    if (!pattern.exactMatch(name))
        return false;
    // existing
    if (findBranchByName(name) != -1)
        return false;
    return true;
}

bool LocalBranchModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // Verify
    if (role != Qt::EditRole || index.row() < branchCount())
        return false;
    const QString branchName = value.toString();
    // Delay the signal as we don't want ourselves to be reset while
    // in setData().
    if (checkNewBranchName(branchName)) {
        m_newBranch = branchName;
        QTimer::singleShot(0, this, SLOT(slotNewBranchDelayedRefresh()));
    }
    return true;
}

void LocalBranchModel::slotNewBranchDelayedRefresh()
{
    emit newBranchEntered(m_newBranch);
}

}
}

