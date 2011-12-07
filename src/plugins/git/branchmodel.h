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

#ifndef BRANCHMODEL_H
#define BRANCHMODEL_H

#include <QtCore/QAbstractListModel>
#include <QtCore/QList>
#include <QtCore/QVariant>

namespace Git {
namespace Internal {

class GitClient;

class BranchNode;

// --------------------------------------------------------------------------
// BranchModel:
// --------------------------------------------------------------------------

class BranchModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit BranchModel(GitClient *client, QObject *parent = 0);
    ~BranchModel();

    // QAbstractItemModel
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void clear();
    bool refresh(const QString &workingDirectory, QString *errorMessage);

    void renameBranch(const QString &oldName, const QString &newName);

    QString workingDirectory() const;
    GitClient *client() const;

    QModelIndex currentBranch() const;
    QString branchName(const QModelIndex &idx) const;
    QStringList localBranchNames() const;
    QString sha(const QModelIndex &idx) const;
    bool isLocal(const QModelIndex &idx) const;
    bool isLeaf(const QModelIndex &idx) const;

    void removeBranch(const QModelIndex &idx);
    void checkoutBranch(const QModelIndex &idx);
    bool branchIsMerged(const QModelIndex &idx);
    QModelIndex addBranch(const QString &branchName, bool track, const QString &trackedBranch);

private:
    void parseOutputLine(const QString &line);

    QString toolTip(const QString &sha) const;

    GitClient *m_client;
    QString m_workingDirectory;
    BranchNode *m_rootNode;
};

} // namespace Internal
} // namespace Git

#endif // BRANCHMODEL_H
