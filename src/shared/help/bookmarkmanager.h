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

#ifndef BOOKMARKMANAGER_H
#define BOOKMARKMANAGER_H

#include "ui_bookmarkdialog.h"

#include <QtCore/QUrl>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>

#include <QtGui/QIcon>
#include <QtGui/QDialog>
#include <QtGui/QWidget>
#include <QtGui/QTreeView>
#include <QtGui/QStandardItemModel>

QT_BEGIN_NAMESPACE

class QEvent;
class QLineEdit;
class QTreeView;
class QToolButton;
class QStandardItem;
class QAbstractItemModel;
class QSortFilterProxyModel;

QT_END_NAMESPACE

class BookmarkManager;

class BookmarkDialog : public QDialog
{
    Q_OBJECT

public:
    BookmarkDialog(BookmarkManager *manager, const QString &title,
        const QString &url, QWidget *parent = 0);
    ~BookmarkDialog();

private slots:
    void addAccepted();
    void addNewFolder();
    void toolButtonClicked();
    void itemChanged(QStandardItem *item);
    void textChanged(const QString& string);
    void selectBookmarkFolder(const QString &folderName);
    void customContextMenuRequested(const QPoint &point);
    void currentChanged(const QModelIndex& current);

private:
    bool eventFilter(QObject *object, QEvent *e);

private:
    QString m_url;
    QString m_title;

    QString oldText;
    QStandardItem *renameItem;

    Ui::BookmarkDialog ui;
    BookmarkManager *bookmarkManager;
    QSortFilterProxyModel *proxyModel;
};

class TreeView : public QTreeView
{
    Q_OBJECT

public:
    TreeView(QWidget* parent = 0) : QTreeView(parent) {}
    void subclassKeyPressEvent(QKeyEvent* event)
    {
        QTreeView::keyPressEvent(event);
    }
};

class BookmarkWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BookmarkWidget(BookmarkManager *manager, QWidget *parent = 0,
                            bool showButtons = true);
    ~BookmarkWidget();

signals:
    void addBookmark();
    void linkActivated(const QUrl &url);

private slots:
    void removeClicked();
    void filterChanged();
    void expand(const QModelIndex& index);
    void activated(const QModelIndex &index);
    void customContextMenuRequested(const QPoint &point);

private:
    void setup(bool showButtons);
    void expandItems();
    bool eventFilter(QObject *object, QEvent *event);

private:
    QRegExp regExp;
    TreeView *treeView;
    QLineEdit *searchField;
    QToolButton *addButton;
    QToolButton *removeButton;
    BookmarkManager *bookmarkManager;
    QSortFilterProxyModel* filterBookmarkModel;
};

class BookmarkModel : public QStandardItemModel
{
    Q_OBJECT

public:
    BookmarkModel(int rows, int columns, QObject *parent = 0);
    ~BookmarkModel();

    Qt::DropActions supportedDropActions() const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

class BookmarkManager : public QObject
{
    Q_OBJECT

public:
    BookmarkManager();
    ~BookmarkManager();

    BookmarkModel *treeBookmarkModel() const;
    BookmarkModel *listBookmarkModel() const;

    void saveBookmarks();
    QStringList bookmarkFolders() const;
    QModelIndex addNewFolder(const QModelIndex &index);
    void removeBookmarkItem(QTreeView *treeView, const QModelIndex& index);
    void showBookmarkDialog(QWidget *parent, const QString &name, const QString &url);
    void addNewBookmark(const QModelIndex &index, const QString &name, const QString &url);
    void setupBookmarkModels();

private slots:
    void itemChanged(QStandardItem *item);

private:
    QString uniqueFolderName() const;
    void removeBookmarkFolderItems(QStandardItem *item);
    void readBookmarksRecursive(const QStandardItem *item, QDataStream &stream,
        const qint32 depth) const;

private:
    const QIcon m_folderIcon;
    const QIcon m_bookmarkIcon;

    QString oldText;

    BookmarkModel *treeModel;
    BookmarkModel *listModel;
    QStandardItem *renameItem;
};

#endif
