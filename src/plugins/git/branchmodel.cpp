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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "branchmodel.h"
#include "gitclient.h"

#include <vcsbase/vcsbaseoutputwindow.h>

#include <QtGui/QFont>
#include <QtCore/QRegExp>
#include <QtCore/QTimer>

namespace Git {
namespace Internal {

// --------------------------------------------------------------------------
// BranchNode:
// --------------------------------------------------------------------------

class BranchNode
{
public:
    BranchNode() :
        parent(0), current(false)
    { }

    BranchNode(const QString &n, const QString &s = QString(), bool c = false) :
        parent(0), current(c), name(n), sha(s)
    { }

    ~BranchNode()
    {
        qDeleteAll(children);
    }

    BranchNode *rootNode()
    {
        return parent ? parent->rootNode() : this;
    }

    int count()
    {
        return children.count();
    }

    bool isLeaf()
    {
        return children.isEmpty();
    }

    bool childOf(BranchNode *node)
    {
        if (this == node)
            return true;
        return parent ? parent->childOf(node) : false;
    }

    bool isLocal()
    {
        BranchNode *rn = rootNode();
        if (rn->isLeaf())
            return false;
        return childOf(rn->children.at(0));
    }

    BranchNode *childOfName(const QString &name)
    {
        for (int i = 0; i < children.count(); ++i) {
            if (children.at(i)->name == name)
                return children.at(i);
        }
        return 0;
    }

    QStringList fullName()
    {
        Q_ASSERT(isLeaf());

        QStringList fn;
        QList<BranchNode *> nodes;
        BranchNode *current = this;
        while (current->parent) {
            nodes.prepend(current);
            current = current->parent;
        }

        if (current->children.at(0) == nodes.at(0))
            nodes.removeFirst(); // remove local branch designation

        foreach (BranchNode *n, nodes)
            fn.append(n->name);

        return fn;
    }

    void insert(const QStringList path, BranchNode *n)
    {
        BranchNode *current = this;
        for (int i = 0; i < path.count(); ++i) {
            BranchNode *c = current->childOfName(path.at(i));
            if (c)
                current = c;
            else
                current = current->append(new BranchNode(path.at(i)));
        }
        current->append(n);
    }

    BranchNode *append(BranchNode *n)
    {
        n->parent = this;
        children.append(n);
        return n;
    }

    QStringList childrenNames()
    {
        if (children.count() > 0) {
            QStringList names;
            foreach (BranchNode *n, children) {
                names.append(n->childrenNames());
            }
            return names;
        }
        return QStringList(fullName().join(QString('/')));
    }

    BranchNode *parent;
    QList<BranchNode *> children;

    bool current;
    QString name;
    QString sha;
    mutable QString toolTip;
};

// --------------------------------------------------------------------------
// BranchModel:
// --------------------------------------------------------------------------

BranchModel::BranchModel(GitClient *client, QObject *parent) :
    QAbstractItemModel(parent),
    m_client(client),
    m_rootNode(new BranchNode)
{
    Q_ASSERT(m_client);
    m_rootNode->append(new BranchNode(tr("Local Branches")));
}

BranchModel::~BranchModel()
{
    delete m_rootNode;
}

QModelIndex BranchModel::index(int row, int column, const QModelIndex &parent) const
{
    BranchNode *node = m_rootNode;
    if (parent.isValid())
        node = static_cast<BranchNode *>(parent.internalPointer());
    if (row >= node->count())
        return QModelIndex();
    return createIndex(row, column, static_cast<void *>(node->children.at(row)));
}

QModelIndex BranchModel::parent(const QModelIndex &index) const
{
    BranchNode *node = static_cast<BranchNode *>(index.internalPointer());
    if (node->parent == m_rootNode)
        return QModelIndex();
    int row = node->parent->children.indexOf(node);
    return createIndex(row, 0, static_cast<void *>(node->parent));
}

int BranchModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_rootNode->count();
    if (parent.column() != 0)
        return 0;
    return static_cast<BranchNode *>(parent.internalPointer())->count();
}

int BranchModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant BranchModel::data(const QModelIndex &index, int role) const
{
    BranchNode *node = static_cast<BranchNode *>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return node->name;
    case Qt::ToolTipRole:
        if (!node->isLeaf())
            return QVariant();
        if (node->toolTip.isEmpty())
            node->toolTip = toolTip(node->sha);
        return node->toolTip;
    case Qt::FontRole:
    {
        QFont font;
        if (!node->isLeaf()) {
            font.setBold(true);
        } else if (node->current) {
            font.setBold(true);
            font.setUnderline(true);
        }
        return font;
    }
    default:
        return QVariant();
    }
}

bool BranchModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;
    BranchNode *node = static_cast<BranchNode *>(index.internalPointer());

    const QString newName = value.toString();
    if (newName.isEmpty())
        return false;

    if (node->name == newName)
        return true;

    QStringList oldFullName = node->fullName();
    node->name = newName;
    QStringList newFullName = node->fullName();

    QString output;
    QString errorMessage;
    if (!m_client->synchronousBranchCmd(m_workingDirectory,
                                        QStringList() << QLatin1String("-m")
                                                      << oldFullName.last()
                                                      << newFullName.last(),
                                        &output, &errorMessage)) {
        node->name = oldFullName.last();
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
        return false;
    }

    emit dataChanged(index, index);
    return true;
}

Qt::ItemFlags BranchModel::flags(const QModelIndex &index) const
{
    BranchNode *node = static_cast<BranchNode *>(index.internalPointer());
    if (node->isLeaf() && node->isLocal())
        return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
    else
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void BranchModel::clear()
{
    while (m_rootNode->count() > 1) {
        BranchNode *n = m_rootNode->children.takeLast();
        delete n;
    }
    BranchNode *locals = m_rootNode->children.at(0);
    while (locals->count()) {
        BranchNode *n = locals->children.takeLast();
        delete n;
    }
}

bool BranchModel::refresh(const QString &workingDirectory, QString *errorMessage)
{
    if (workingDirectory.isEmpty())
        return false;

    QStringList branchArgs;
    branchArgs << QLatin1String(GitClient::noColorOption)
               << QLatin1String("-v") << QLatin1String("-a");
    QString output;
    if (!m_client->synchronousBranchCmd(workingDirectory, branchArgs, &output, errorMessage)) {
        VCSBase::VCSBaseOutputWindow::instance()->appendError(*errorMessage);
        return false;
    }

    beginResetModel();

    clear();

    m_workingDirectory = workingDirectory;
    const QStringList lines = output.split(QLatin1Char('\n'));
    foreach (const QString &l, lines)
        parseOutputLine(l);

    endResetModel();
    return true;
}

void BranchModel::renameBranch(const QString &oldName, const QString &newName)
{
    QString errorMessage;
    QString output;
    if (!m_client->synchronousBranchCmd(m_workingDirectory,
                                        QStringList() << QLatin1String("-m") << oldName << newName,
                                        &output, &errorMessage))
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
    else
        refresh(m_workingDirectory, &errorMessage);
}

QString BranchModel::workingDirectory() const
{
    return m_workingDirectory;
}

GitClient *BranchModel::client() const
{
    return m_client;
}

QModelIndex BranchModel::currentBranch() const
{
    if (!m_rootNode || !m_rootNode->count())
        return QModelIndex();
    BranchNode *localBranches = m_rootNode->children.at(0);
    QModelIndex localIdx = index(0, 0, QModelIndex());
    for (int i = 0; i < localBranches->count(); ++i) {
        if (localBranches->children.at(i)->current)
            return index(i, 0, localIdx);
    }
    return QModelIndex();
}

QString BranchModel::branchName(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QString();
    BranchNode *node = static_cast<BranchNode *>(idx.internalPointer());
    if (!node->isLeaf())
        return QString();
    QStringList path = node->fullName();
    return path.join(QString('/'));
}

QStringList BranchModel::localBranchNames() const
{
    if (!m_rootNode || m_rootNode->children.isEmpty())
        return QStringList();

    return m_rootNode->children.at(0)->childrenNames();
}

QString BranchModel::sha(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QString();
    BranchNode *node = static_cast<BranchNode *>(idx.internalPointer());
    return node->sha;
}

bool BranchModel::isLocal(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return false;
    BranchNode *node = static_cast<BranchNode *>(idx.internalPointer());
    return node->isLocal();
}

bool BranchModel::isLeaf(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return false;
    BranchNode *node = static_cast<BranchNode *>(idx.internalPointer());
    return node->isLeaf();
}

void BranchModel::removeBranch(const QModelIndex &idx)
{
    QString branch = branchName(idx);
    if (branch.isEmpty())
        return;

    QString errorMessage;
    QString output;
    QStringList args;

    args << QLatin1String("-D") << branch;
    if (!m_client->synchronousBranchCmd(m_workingDirectory, args, &output, &errorMessage)) {
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
        return;
    }

    QModelIndex parentIdx = parent(idx);
    beginRemoveRows(parentIdx, idx.row(), idx.row());
    static_cast<BranchNode *>(parentIdx.internalPointer())->children.removeAt(parentIdx.row());
    delete static_cast<BranchNode *>(idx.internalPointer());
    endRemoveRows();
}

void BranchModel::checkoutBranch(const QModelIndex &idx)
{
    QString branch = branchName(idx);
    if (branch.isEmpty())
        return;

    QString errorMessage;
    switch (m_client->ensureStash(m_workingDirectory, &errorMessage)) {
    case GitClient::StashUnchanged:
    case GitClient::Stashed:
    case GitClient::NotStashed:
        break;
    case GitClient::StashCanceled:
        return;
    case GitClient::StashFailed:
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
        return;
    }
    if (m_client->synchronousCheckoutBranch(m_workingDirectory, branch, &errorMessage)) {
        if (errorMessage.isEmpty()) {
            static_cast<BranchNode *>(currentBranch().internalPointer())->current = false;
            emit dataChanged(currentBranch(), currentBranch());
            static_cast<BranchNode *>(idx.internalPointer())->current = true;
            emit dataChanged(idx, idx);
        } else {
            refresh(m_workingDirectory, &errorMessage); // not sure all went well... better refresh!
        }
    }
    if (!errorMessage.isEmpty())
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
}

bool BranchModel::branchIsMerged(const QModelIndex &idx)
{
    QString branch = branchName(idx);
    if (branch.isEmpty())
        return false;

    QString errorMessage;
    QString output;
    QStringList args;

    args << QLatin1String("--contains") << sha(idx);
    if (!m_client->synchronousBranchCmd(m_workingDirectory, args, &output, &errorMessage)) {
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
        return false;
    }

    QStringList lines = output.split(QLatin1Char('/'), QString::SkipEmptyParts);
    foreach (const QString &l, lines) {
        if (l.startsWith(QLatin1String("  ")) && l.count() >= 3)
            return true;
    }
    return false;
}

QModelIndex BranchModel::addBranch(const QString &branchName, bool track, const QString &startPoint)
{
    if (!m_rootNode || !m_rootNode->count())
        return QModelIndex();

    QString output;
    QString errorMessage;

    QStringList args;
    args << (track ? QLatin1String("--track") : QLatin1String("--no-track"));
    args << branchName << startPoint;

    if (!m_client->synchronousBranchCmd(m_workingDirectory, args, &output, &errorMessage)) {
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
        return QModelIndex();
    }

    BranchNode *local = m_rootNode->children.at(0);
    int pos = 0;
    for (pos = 0; pos < local->count(); ++pos) {
        if (local->children.at(pos)->name > branchName)
            break;
    }
    BranchNode *newNode = new BranchNode(branchName);

    // find the sha of the new branch:
    output = toolTip(branchName); // abuse toolTip to get the data;-)
    QStringList lines = output.split(QLatin1Char('\n'));
    foreach (const QString &l, lines) {
        if (l.startsWith("commit ")) {
            newNode->sha = l.mid(7, 8);
            break;
        }
    }

    beginInsertRows(index(0, 0), pos, pos);
    newNode->parent = local;
    local->children.insert(pos, newNode);
    endInsertRows();

    return index(pos, 0, index(0, 0));
}

void BranchModel::parseOutputLine(const QString &line)
{
    if (line.size() < 3)
        return;

    bool current = line.startsWith(QLatin1String("* "));

    const QString branchInfo = line.mid(2);
    if (current && branchInfo.startsWith(QLatin1String("(no branch)")))
        return;

    QStringList tokens = branchInfo.split(QLatin1Char(' '), QString::SkipEmptyParts);
    if (tokens.size() < 2)
        return;

    QString sha = tokens.at(1);

    // insert node into tree:
    QStringList nameParts = tokens.at(0).split(QLatin1Char('/'));
    if (nameParts.count() < 1)
        return;

    QString name = nameParts.last();
    nameParts.removeLast();

    if (nameParts.isEmpty() || nameParts.at(0) != QLatin1String("remotes")) {
        // local branch:
        while (nameParts.count() > 2)
            nameParts[1] = nameParts.at(1) + QLatin1Char('/') + nameParts.at(2);
        m_rootNode->children.at(0)->insert(nameParts, new BranchNode(name, sha, current));
    } else {
        // remote branch:
        nameParts.removeFirst(); // remove "remotes"
        while (nameParts.count() > 2)
            nameParts[1] = nameParts.at(1) + QLatin1Char('/') + nameParts.at(2);
        m_rootNode->insert(nameParts, new BranchNode(name, sha, current));
    }
}

QString BranchModel::toolTip(const QString &sha) const
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

} // namespace Internal
} // namespace Git

