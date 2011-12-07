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

#include "rightpane.h"

#include <coreplugin/modemanager.h>

#include <QtCore/QSettings>

#include <QtGui/QVBoxLayout>
#include <QtGui/QSplitter>
#include <QtGui/QResizeEvent>
#include <QtGui/QTextEdit>


using namespace Core;
using namespace Core::Internal;

RightPanePlaceHolder *RightPanePlaceHolder::m_current = 0;

RightPanePlaceHolder* RightPanePlaceHolder::current()
{
    return m_current;
}

RightPanePlaceHolder::RightPanePlaceHolder(Core::IMode *mode, QWidget *parent)
    :QWidget(parent), m_mode(mode)
{
    setLayout(new QVBoxLayout);
    layout()->setMargin(0);
    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode *)),
            this, SLOT(currentModeChanged(Core::IMode *)));
}

RightPanePlaceHolder::~RightPanePlaceHolder()
{
    if (m_current == this) {
        RightPaneWidget::instance()->setParent(0);
        RightPaneWidget::instance()->hide();
    }
}

void RightPanePlaceHolder::applyStoredSize(int width)
{
    if (width) {
        QSplitter *splitter = qobject_cast<QSplitter *>(parentWidget());
        if (splitter) {
            // A splitter we need to resize the splitter sizes
            QList<int> sizes = splitter->sizes();
            int index = splitter->indexOf(this);
            int diff = width - sizes.at(index);
            int adjust = sizes.count() > 1 ? (diff / (sizes.count() - 1)) : 0;
            for (int i = 0; i < sizes.count(); ++i) {
                if (i != index)
                    sizes[i] -= adjust;
            }
            sizes[index]= width;
            splitter->setSizes(sizes);
        } else {
            QSize s = size();
            s.setWidth(width);
            resize(s);
        }
    }
}

// This function does work even though the order in which
// the placeHolder get the signal is undefined.
// It does ensure that after all PlaceHolders got the signal
// m_current points to the current PlaceHolder, or zero if there
// is no PlaceHolder in this mode
// And that the parent of the RightPaneWidget gets the correct parent
void RightPanePlaceHolder::currentModeChanged(Core::IMode *mode)
{
    if (m_current == this) {
        m_current = 0;
        RightPaneWidget::instance()->setParent(0);
        RightPaneWidget::instance()->hide();
    }
    if (m_mode == mode) {
        m_current = this;

        int width = RightPaneWidget::instance()->storedWidth();

        layout()->addWidget(RightPaneWidget::instance());
        RightPaneWidget::instance()->show();

        applyStoredSize(width);
        setVisible(RightPaneWidget::instance()->isShown());
    }
}

/////
// RightPaneWidget
/////


RightPaneWidget *RightPaneWidget::m_instance = 0;

RightPaneWidget::RightPaneWidget()
    : m_shown(true), m_width(0)
{
    m_instance = this;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    setLayout(layout);
}

RightPaneWidget::~RightPaneWidget()
{
    clearWidget();
    m_instance = 0;
}

RightPaneWidget *RightPaneWidget::instance()
{
    return m_instance;
}

void RightPaneWidget::setWidget(QWidget *widget)
{
    clearWidget();
    m_widget = widget;
    if (m_widget) {
        m_widget->setParent(this);
        layout()->addWidget(m_widget);
        setFocusProxy(m_widget);
        m_widget->show();
    }
}

int RightPaneWidget::storedWidth()
{
    return m_width;
}

void RightPaneWidget::resizeEvent(QResizeEvent *re)
{
    if (m_width && re->size().width())
        m_width = re->size().width();
    QWidget::resizeEvent(re);
}

void RightPaneWidget::saveSettings(QSettings *settings)
{
    settings->setValue("RightPane/Visible", isShown());
    settings->setValue("RightPane/Width", m_width);
}

void RightPaneWidget::readSettings(QSettings *settings)
{
    if (settings->contains("RightPane/Visible")) {
        setShown(settings->value("RightPane/Visible").toBool());
    } else {
        setShown(false); //TODO set to false
    }

    if (settings->contains("RightPane/Width")) {
        m_width = settings->value("RightPane/Width").toInt();
        if (!m_width)
            m_width = 500;
    } else {
        m_width = 500; //pixel
    }
    // Apply
    if (RightPanePlaceHolder::m_current) {
        RightPanePlaceHolder::m_current->applyStoredSize(m_width);
    }
}

void RightPaneWidget::setShown(bool b)
{
    if (RightPanePlaceHolder::m_current)
        RightPanePlaceHolder::m_current->setVisible(b);
    m_shown = b;
}

bool RightPaneWidget::isShown()
{
    return m_shown;
}

void RightPaneWidget::clearWidget()
{
    if (m_widget) {
        m_widget->hide();
        m_widget->setParent(0);
        m_widget = 0;
    }
}
