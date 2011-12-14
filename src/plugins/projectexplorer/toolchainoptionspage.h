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

#ifndef TOOLCHAINOPTIONSPAGE_H
#define TOOLCHAINOPTIONSPAGE_H

#include "ui_toolchainoptionspage.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QtCore/QAbstractItemModel>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
class QWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {

class ToolChain;
class ToolChainConfigWidget;
class ToolChainFactory;
class ToolChainManager;

namespace Internal {

class ToolChainNode;
// --------------------------------------------------------------------------
// ToolChainModel
// --------------------------------------------------------------------------

class ToolChainModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ToolChainModel(QWidget *configWidgetParent, QObject *parent = 0);
    ~ToolChainModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(const QModelIndex &topIdx, ToolChain *) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    ToolChain *toolChain(const QModelIndex &);
    int manualToolChains() const;

    ToolChainConfigWidget *widget(const QModelIndex &);

    bool isDirty() const;
    bool isDirty(ToolChain *) const;

    void apply();

    void markForRemoval(ToolChain *);
    void markForAddition(ToolChain *);

signals:
    void toolChainStateChanged();

private slots:
    void addToolChain(ProjectExplorer::ToolChain *);
    void removeToolChain(ProjectExplorer::ToolChain *);
    void setDirty(ProjectExplorer::ToolChain *);

private:
    QModelIndex index(ToolChainNode *, int column = 0) const;
    ToolChainNode *createNode(ToolChainNode *parent, ToolChain *tc, bool changed);

    ToolChainNode * m_root;
    ToolChainNode * m_autoRoot;
    ToolChainNode * m_manualRoot;

    QList<ToolChainNode *> m_toAddList;
    QList<ToolChainNode *> m_toRemoveList;

    QWidget *m_configWidgetParent;
};

// --------------------------------------------------------------------------
// ToolChainOptionsPage
// --------------------------------------------------------------------------

class ToolChainOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    ToolChainOptionsPage();

    virtual QString id() const;
    virtual QString displayName() const;
    virtual QString category() const;
    virtual QString displayCategory() const;
    virtual QIcon categoryIcon() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish();
    virtual bool matches(const QString &) const;

private slots:
    void toolChainSelectionChanged(const QItemSelection &);
    void createToolChain(QObject *);
    void removeToolChain();
    void updateState();

private:
    Ui::ToolChainOptionsPage *m_ui;
    QWidget *m_configWidget;
    QString m_searchKeywords;

    ToolChainModel *m_model;
    QList<ToolChainFactory *> m_factories;
    QItemSelectionModel * m_selectionModel;
    ToolChainConfigWidget *m_currentTcWidget;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TOOLCHAINOPTIONSPAGE_H
