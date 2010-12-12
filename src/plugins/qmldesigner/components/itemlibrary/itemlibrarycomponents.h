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

#ifndef ITEMLIBRARYTREEVIEW_H
#define ITEMLIBRARYTREEVIEW_H

#include <QtGui/QTreeView>
#include <QtGui/QStandardItemModel>
#include <QDrag>
#include <QDebug>
#include <QTimeLine>
#include <QToolButton>
#include <QStyledItemDelegate>

QT_BEGIN_NAMESPACE
class QDirModel;
class QLabel;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {

class ResourceItemDelegate;

// ItemLibraryTreeView with Drag implementation
class ItemLibraryTreeView : public QTreeView {
    Q_OBJECT
public:
    explicit ItemLibraryTreeView(QWidget *parent = 0);

    virtual void startDrag(Qt::DropActions supportedActions);
    virtual void setModel(QAbstractItemModel *model);

signals:
    void itemActivated(const QString &itemName);

private slots:
    void activateItem( const QModelIndex &index);

private:
    ResourceItemDelegate *m_delegate;
};

class ResourceItemDelegate : public QStyledItemDelegate
{
public:
    explicit ResourceItemDelegate(QObject *parent=0) :
        QStyledItemDelegate(parent),m_model(0) {}

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;

    void setModel(QDirModel *model);

private:
    QDirModel *m_model;
};

} // namespace Internal

} // namespace QmlDesigner

#endif // ITEMLIBRARYTREEVIEW_H

