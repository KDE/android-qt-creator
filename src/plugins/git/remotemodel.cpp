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

#include "remotemodel.h"
#include "gitclient.h"

namespace Git {
namespace Internal {

// Parse a branch line: " *name sha description".
bool RemoteModel::Remote::parse(const QString &line)
{
    if (!line.endsWith(" (fetch)"))
        return false;

    QStringList tokens = line.split(QRegExp("\\s"), QString::SkipEmptyParts);
    if (tokens.count() != 3)
        return false;

    name = tokens.at(0);
    url = tokens.at(1);
    return true;
}

// ------ RemoteModel
RemoteModel::RemoteModel(GitClient *client, QObject *parent) :
    QAbstractTableModel(parent),
    m_flags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable),
    m_client(client)
{ }

QString RemoteModel::remoteName(int row) const
{
    return m_remotes.at(row).name;
}

QString RemoteModel::remoteUrl(int row) const
{
    return m_remotes.at(row).url;
}

bool RemoteModel::removeRemote(int row)
{
    QString output;
    QString error;
    bool success = m_client->synchronousRemoteCmd(m_workingDirectory,
                                                  QStringList() << QLatin1String("rm") << remoteName(row),
                                                  &output, &error);
    if (success)
        success = refresh(m_workingDirectory, &error);
    return success;
}

bool RemoteModel::addRemote(const QString &name, const QString &url)
{
    QString output;
    QString error;
    if (name.isEmpty() || url.isEmpty())
        return false;

    bool success = m_client->synchronousRemoteCmd(m_workingDirectory,
                                                  QStringList() << QLatin1String("add") << name << url,
                                                  &output, &error);
    if (success)
        success = refresh(m_workingDirectory, &error);
    return success;
}

bool RemoteModel::renameRemote(const QString &oldName, const QString &newName)
{
    QString output;
    QString error;
    bool success = m_client->synchronousRemoteCmd(m_workingDirectory,
                                                  QStringList() << QLatin1String("rename") << oldName << newName,
                                                  &output, &error);
    if (success)
        success = refresh(m_workingDirectory, &error);
    return success;
}

bool RemoteModel::updateUrl(const QString &name, const QString &newUrl)
{
    QString output;
    QString error;
    bool success = m_client->synchronousRemoteCmd(m_workingDirectory,
                                                  QStringList() << QLatin1String("set-url") << name << newUrl,
                                                  &output, &error);
    if (success)
        success = refresh(m_workingDirectory, &error);
    return success;
}

QString RemoteModel::workingDirectory() const
{
    return m_workingDirectory;
}

int RemoteModel::remoteCount() const
{
    return m_remotes.size();
}

int RemoteModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return remoteCount();
}

int RemoteModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant RemoteModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        if (index.column() == 0)
            return remoteName(row);
        else
            return remoteUrl(row);
    default:
        break;
    }
    return QVariant();
}

bool RemoteModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    const QString name = remoteName(index.row());
    const QString url = remoteUrl(index.row());
    switch (index.column()) {
    case 0:
        if (name == value.toString())
            return true;
        return renameRemote(name, value.toString());
    case 1:
        if (url == value.toString())
            return true;
        return updateUrl(name, value.toString());
    default:
        return false;
    }
}

Qt::ItemFlags RemoteModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return m_flags;
}

void RemoteModel::clear()
{
    if (m_remotes.isEmpty())
        return;
    m_remotes.clear();
    reset();
}

bool RemoteModel::refresh(const QString &workingDirectory, QString *errorMessage)
{
    // Run branch command with verbose.
    QStringList remoteArgs;
    remoteArgs << QLatin1String("-v");
    QString output;
    if (!m_client->synchronousRemoteCmd(workingDirectory, remoteArgs, &output, errorMessage))
        return false;
    // Parse output
    m_workingDirectory = workingDirectory;
    m_remotes.clear();
    const QStringList lines = output.split(QLatin1Char('\n'));
    for (int r = 0; r < lines.count(); ++r) {
        Remote newRemote;
        if (newRemote.parse(lines.at(r)))
            m_remotes.push_back(newRemote);
    }
    reset();
    return true;
}

int RemoteModel::findRemoteByName(const QString &name) const
{
    const int count = remoteCount();
    for (int i = 0; i < count; i++)
        if (remoteName(i) == name)
            return i;
    return -1;
}

GitClient *RemoteModel::client() const
{
    return m_client;
}

} // namespace Internal
} // namespace Git

