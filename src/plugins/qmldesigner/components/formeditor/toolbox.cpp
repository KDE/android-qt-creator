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

#include "toolbox.h"
#include "utils/styledbar.h"
#include "utils/crumblepath.h"

#include <QToolBar>
#include <QHBoxLayout>
#include <QPainter>
#include <QtDebug>
#include <QFile>
#include <QFrame>
#include <QVariant>

namespace QmlDesigner {

ToolBox::ToolBox(QWidget *parentWidget)
    : Utils::StyledBar(parentWidget),
  m_leftToolBar(new QToolBar("LeftSidebar", this)),
  m_rightToolBar(new QToolBar("RightSidebar", this))
{
    setSingleRow(false);
    QFrame *frame = new QFrame(this);
    m_crumblePath = new Utils::CrumblePath(frame);
    frame->setStyleSheet("background-color: #4e4e4e;");
    frame->setFrameShape(QFrame::NoFrame);
    QHBoxLayout *layout = new QHBoxLayout(frame);
    layout->setMargin(0);
    layout->setSpacing(0);
    frame->setLayout(layout);
    layout->addWidget(m_crumblePath);
    frame->setProperty("panelwidget", true);
    frame->setProperty("panelwidget_singlerow", false);
    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->setMargin(0);
    verticalLayout->setSpacing(0);

    QHBoxLayout *horizontalLayout = new QHBoxLayout(this);
    verticalLayout->addLayout(horizontalLayout);
    verticalLayout->addWidget(frame);
    horizontalLayout->setMargin(0);
    horizontalLayout->setSpacing(0);

    m_leftToolBar->setFloatable(true);
    m_leftToolBar->setMovable(true);
    m_leftToolBar->setOrientation(Qt::Horizontal);
    m_leftToolBar->setIconSize(QSize(24, 24));

    QToolBar *stretchToolbar = new QToolBar(this);

    setSingleRow(false);

    m_leftToolBar->setProperty("panelwidget", true);
    m_leftToolBar->setProperty("panelwidget_singlerow", false);

    m_rightToolBar->setProperty("panelwidget", true);
    m_rightToolBar->setProperty("panelwidget_singlerow", false);

    stretchToolbar->setProperty("panelwidget", true);
    stretchToolbar->setProperty("panelwidget_singlerow", false);

    stretchToolbar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    m_rightToolBar->setOrientation(Qt::Horizontal);
    m_rightToolBar->setIconSize(QSize(24, 24));
    horizontalLayout->addWidget(m_leftToolBar);
    horizontalLayout->addWidget(stretchToolbar);
    horizontalLayout->addWidget(m_rightToolBar);

    setLayout(verticalLayout);
}

void ToolBox::setLeftSideActions(const QList<QAction*> &actions)
{
    m_leftToolBar->clear();
    m_leftToolBar->addActions(actions);
    resize(sizeHint());
}

void ToolBox::setRightSideActions(const QList<QAction*> &actions)
{
    m_rightToolBar->clear();
    m_rightToolBar->addActions(actions);
    resize(sizeHint());
}

void ToolBox::addLeftSideAction(QAction *action)
{
    m_leftToolBar->addAction(action);
}

void ToolBox::addRightSideAction(QAction *action)
{
    m_rightToolBar->addAction(action);
}


QList<QAction*> ToolBox::actions() const
{
    return QList<QAction*>() << m_leftToolBar->actions() << m_rightToolBar->actions();
}

Utils::CrumblePath *ToolBox::crumblePath() const
{
    return m_crumblePath;
}

} // namespace QmlDesigner
