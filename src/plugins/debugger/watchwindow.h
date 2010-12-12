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

#ifndef DEBUGGER_WATCHWINDOW_H
#define DEBUGGER_WATCHWINDOW_H

#include <QtGui/QTreeView>

namespace Debugger {
namespace Internal {

/////////////////////////////////////////////////////////////////////
//
// WatchWindow
//
/////////////////////////////////////////////////////////////////////

class WatchWindow : public QTreeView
{
    Q_OBJECT

public:
    enum Type { ReturnType, LocalsType, TooltipType, WatchersType };

    explicit WatchWindow(Type type, QWidget *parent = 0);
    Type type() const { return m_type; }

public slots:
    void resizeColumnsToContents();
    void setAlwaysResizeColumnsToContents(bool on = true);
    void setModel(QAbstractItemModel *model);
    void setAlternatingRowColorsHelper(bool on) { setAlternatingRowColors(on); }
    void watchExpression(const QString &exp);
    void removeWatchExpression(const QString &exp);

private:
    Q_SLOT void resetHelper();
    Q_SLOT void expandNode(const QModelIndex &idx);
    Q_SLOT void collapseNode(const QModelIndex &idx);
    Q_SLOT void setUpdatesEnabled(bool enable);

    void keyPressEvent(QKeyEvent *ev);
    void contextMenuEvent(QContextMenuEvent *ev);
    void dragEnterEvent(QDragEnterEvent *ev);
    void dropEvent(QDropEvent *ev);
    void dragMoveEvent(QDragMoveEvent *ev);
    void mouseDoubleClickEvent(QMouseEvent *ev);
    bool event(QEvent *ev);

    void editItem(const QModelIndex &idx);
    void resetHelper(const QModelIndex &idx);
    void setWatchpoint(quint64 address);

    void setModelData(int role, const QVariant &value = QVariant(),
        const QModelIndex &index = QModelIndex());

    bool m_alwaysResizeColumnsToContents;
    Type m_type;
    bool m_grabbing;
};


} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_WATCHWINDOW_H
