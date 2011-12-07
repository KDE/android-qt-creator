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

#include "openpageswidget.h"

#include "centralwidget.h"
#include "openpagesmodel.h"

#include <coreplugin/coreconstants.h>

#include <QtGui/QApplication>
#include <QtGui/QPainter>

#include <QtGui/QHeaderView>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QMenu>

#ifdef Q_WS_MAC
#include <qmacstyle_mac.h>
#endif

using namespace Help::Internal;

// -- OpenPagesDelegate

OpenPagesDelegate::OpenPagesDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void OpenPagesDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
           const QModelIndex &index) const
{
    if (option.state & QStyle::State_MouseOver) {
        if ((QApplication::mouseButtons() & Qt::LeftButton) == 0)
            pressedIndex = QModelIndex();
        QBrush brush = option.palette.alternateBase();
        if (index == pressedIndex)
            brush = option.palette.dark();
        painter->fillRect(option.rect, brush);
    }


    QStyledItemDelegate::paint(painter, option, index);

    if (index.column() == 1 && index.model()->rowCount() > 1
        && option.state & QStyle::State_MouseOver) {
        const QIcon icon(QLatin1String((option.state & QStyle::State_Selected) ?
                                       Core::Constants::ICON_CLOSE : Core::Constants::ICON_CLOSE_DARK));

        const QRect iconRect(option.rect.right() - option.rect.height(),
            option.rect.top(), option.rect.height(), option.rect.height());

        icon.paint(painter, iconRect, Qt::AlignRight | Qt::AlignVCenter);
    }

}

// -- OpenPagesWidget

OpenPagesWidget::OpenPagesWidget(OpenPagesModel *model, QWidget *parent)
    : QTreeView(parent)
    , m_allowContextMenu(true)
{
    setModel(model);
    setIndentation(0);
    setItemDelegate((m_delegate = new OpenPagesDelegate(this)));

    setTextElideMode(Qt::ElideMiddle);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    viewport()->setAttribute(Qt::WA_Hover);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    header()->hide();
    header()->setStretchLastSection(false);
    header()->setResizeMode(0, QHeaderView::Stretch);
    header()->setResizeMode(1, QHeaderView::Fixed);
    header()->resizeSection(1, 18);

    installEventFilter(this);
    setUniformRowHeights(true);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, SIGNAL(clicked(QModelIndex)), this,
        SLOT(handleClicked(QModelIndex)));
    connect(this, SIGNAL(pressed(QModelIndex)), this,
        SLOT(handlePressed(QModelIndex)));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this,
        SLOT(contextMenuRequested(QPoint)));
}

OpenPagesWidget::~OpenPagesWidget()
{
}

void OpenPagesWidget::selectCurrentPage()
{
    QItemSelectionModel * const selModel = selectionModel();
    selModel->clearSelection();
    selModel->select(model()->index(CentralWidget::instance()->currentIndex(), 0),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    scrollTo(currentIndex());
}

void OpenPagesWidget::allowContextMenu(bool ok)
{
    m_allowContextMenu = ok;
}

// -- private slots

void OpenPagesWidget::contextMenuRequested(QPoint pos)
{
    QModelIndex index = indexAt(pos);
    if (!index.isValid() || !m_allowContextMenu)
        return;

    if (index.column() == 1)
        index = index.sibling(index.row(), 0);
    QMenu contextMenu;
    QAction *closeEditor = contextMenu.addAction(tr("Close %1").arg(index.data()
        .toString()));
    QAction *closeOtherEditors = contextMenu.addAction(tr("Close All Except %1")
        .arg(index.data().toString()));

    if (model()->rowCount() == 1) {
        closeEditor->setEnabled(false);
        closeOtherEditors->setEnabled(false);
    }

    QAction *action = contextMenu.exec(mapToGlobal(pos));
    if (action == closeEditor)
        emit closePage(index);
    else if (action == closeOtherEditors)
        emit closePagesExcept(index);
}

void OpenPagesWidget::handlePressed(const QModelIndex &index)
{
    if (index.column() == 0)
        emit setCurrentPage(index);

    if (index.column() == 1)
        m_delegate->pressedIndex = index;
}

void OpenPagesWidget::handleClicked(const QModelIndex &index)
{
    // implemented here to handle the funky close button and to  work around a
    // bug in item views where the delegate wouldn't get the QStyle::State_MouseOver
    if (index.column() == 1) {
        if (model()->rowCount() > 1)
            emit closePage(index);

        QWidget *vp = viewport();
        const QPoint &cursorPos = QCursor::pos();
        QMouseEvent e(QEvent::MouseMove, vp->mapFromGlobal(cursorPos), cursorPos,
            Qt::NoButton, 0, 0);
        QCoreApplication::sendEvent(vp, &e);
    }
}

// -- private

bool OpenPagesWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == this && event->type() == QEvent::KeyPress) {
        if (currentIndex().isValid()) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            const int key = ke->key();
            if ((key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space)
                && ke->modifiers() == 0) {
                emit setCurrentPage(currentIndex());
            } else if ((key == Qt::Key_Delete || key == Qt::Key_Backspace)
                && ke->modifiers() == 0 && model()->rowCount() > 1) {
                emit closePage(currentIndex());
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
