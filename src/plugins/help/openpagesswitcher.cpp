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

#include "openpagesswitcher.h"

#include "centralwidget.h"
#include "openpagesmodel.h"
#include "openpageswidget.h"

#include <QtCore/QEvent>

#include <QtGui/QKeyEvent>
#include <QtGui/QVBoxLayout>

using namespace Help::Internal;

const int gWidth = 300;
const int gHeight = 200;

OpenPagesSwitcher::OpenPagesSwitcher(OpenPagesModel *model)
    : QFrame(0, Qt::Popup)
    , m_openPagesModel(model)
{
    resize(gWidth, gHeight);

    m_openPagesWidget = new OpenPagesWidget(m_openPagesModel, this);

    // We disable the frame on this list view and use a QFrame around it instead.
    // This improves the look with QGTKStyle.
#ifndef Q_WS_MAC
    setFrameStyle(m_openPagesWidget->frameStyle());
#endif
    m_openPagesWidget->setFrameStyle(QFrame::NoFrame);

    m_openPagesWidget->allowContextMenu(false);
    m_openPagesWidget->installEventFilter(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_openPagesWidget);

    connect(m_openPagesWidget, SIGNAL(closePage(QModelIndex)), this,
        SIGNAL(closePage(QModelIndex)));
    connect(m_openPagesWidget, SIGNAL(setCurrentPage(QModelIndex)), this,
        SIGNAL(setCurrentPage(QModelIndex)));
}

OpenPagesSwitcher::~OpenPagesSwitcher()
{
}

void OpenPagesSwitcher::gotoNextPage()
{
    selectPageUpDown(-1);
}

void OpenPagesSwitcher::gotoPreviousPage()
{
    selectPageUpDown(1);
}

void OpenPagesSwitcher::selectAndHide()
{
    setVisible(false);
    emit setCurrentPage(m_openPagesWidget->currentIndex());
}

void OpenPagesSwitcher::selectCurrentPage()
{
    m_openPagesWidget->selectCurrentPage();
}

void OpenPagesSwitcher::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if (visible)
        setFocus();
}

void OpenPagesSwitcher::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    m_openPagesWidget->setFocus();
}

bool OpenPagesSwitcher::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_openPagesWidget) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Escape) {
                setVisible(false);
                return true;
            }

            const int key = ke->key();
            if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
                emit setCurrentPage(m_openPagesWidget->currentIndex());
                return true;
            }
#ifdef Q_WS_MAC
            const Qt::KeyboardModifier modifier = Qt::AltModifier;
#else
            const Qt::KeyboardModifier modifier = Qt::ControlModifier;
#endif
            if (key == Qt::Key_Backtab
                && (ke->modifiers() == (modifier | Qt::ShiftModifier)))
                gotoNextPage();
            else if (key == Qt::Key_Tab && (ke->modifiers() == modifier))
                gotoPreviousPage();
        } else if (event->type() == QEvent::KeyRelease) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            if (ke->modifiers() == 0
               /*HACK this is to overcome some event inconsistencies between platforms*/
               || (ke->modifiers() == Qt::AltModifier
               && (ke->key() == Qt::Key_Alt || ke->key() == -1))) {
                selectAndHide();
            }
        }
    }
    return QWidget::eventFilter(object, event);
}

void OpenPagesSwitcher::selectPageUpDown(int summand)
{
    const int pageCount = m_openPagesModel->rowCount();
    if (pageCount < 2)
        return;

    const QModelIndexList &list = m_openPagesWidget->selectionModel()->selectedIndexes();
    if (list.isEmpty())
        return;

    QModelIndex index = list.first();
    if (!index.isValid())
        return;

    index = m_openPagesModel->index((index.row() + summand + pageCount) % pageCount, 0);
    if (index.isValid()) {
        m_openPagesWidget->setCurrentIndex(index);
        m_openPagesWidget->scrollTo(index, QAbstractItemView::PositionAtCenter);
    }
}
