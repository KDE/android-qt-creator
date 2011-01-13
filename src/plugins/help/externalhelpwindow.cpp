/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "externalhelpwindow.h"

#include "centralwidget.h"
#include "helpconstants.h"
#include "openpagesmanager.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <QtGui/QAction>
#include <QtGui/QHBoxLayout>
#include <QtGui/QKeyEvent>
#include <QtGui/QStatusBar>
#include <QtGui/QToolButton>

using namespace Help::Internal;

ExternalHelpWindow::ExternalHelpWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(Help::Constants::ID_MODE_HELP);

    const QVariant geometry = settings->value(QLatin1String("geometry"));
    if (geometry.isValid())
        restoreGeometry(geometry.toByteArray());
    else
        resize(640, 480);

    settings->endGroup();

    QAction *action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I));
    connect(action, SIGNAL(triggered()), this, SIGNAL(activateIndex()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C));
    connect(action, SIGNAL(triggered()), this, SIGNAL(activateContents()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Slash));
    connect(action, SIGNAL(triggered()), this, SIGNAL(activateSearch()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
    connect(action, SIGNAL(triggered()), this, SIGNAL(activateBookmarks()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O));
    connect(action, SIGNAL(triggered()), this, SIGNAL(activateOpenPages()));
    addAction(action);

    CentralWidget *centralWidget = CentralWidget::instance();

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus));
    connect(action, SIGNAL(triggered()), centralWidget, SLOT(zoomIn()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus));
    connect(action, SIGNAL(triggered()), centralWidget, SLOT(zoomOut()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
    connect(action, SIGNAL(triggered()), this, SIGNAL(addBookmark()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    connect(action, SIGNAL(triggered()), centralWidget, SLOT(copy()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));
    connect(action, SIGNAL(triggered()), centralWidget, SLOT(print()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence::Back);
    action->setEnabled(centralWidget->isBackwardAvailable());
    connect(action, SIGNAL(triggered()), centralWidget, SLOT(backward()));
    connect(centralWidget, SIGNAL(backwardAvailable(bool)), action,
        SLOT(setEnabled(bool)));

    action = new QAction(this);
    action->setShortcut(QKeySequence::Forward);
    action->setEnabled(centralWidget->isForwardAvailable());
    connect(action, SIGNAL(triggered()), centralWidget, SLOT(forward()));
    connect(centralWidget, SIGNAL(forwardAvailable(bool)), action,
        SLOT(setEnabled(bool)));

    QAction *reset = new QAction(this);
    connect(reset, SIGNAL(triggered()), centralWidget, SLOT(resetZoom()));
    addAction(reset);

    QAction *ctrlTab = new QAction(this);
    connect(ctrlTab, SIGNAL(triggered()), &OpenPagesManager::instance(),
        SLOT(gotoPreviousPage()));
    addAction(ctrlTab);

    QAction *ctrlShiftTab = new QAction(this);
    connect(ctrlShiftTab, SIGNAL(triggered()), &OpenPagesManager::instance(),
        SLOT(gotoNextPage()));
    addAction(ctrlShiftTab);

    action = new QAction(QIcon(Core::Constants::ICON_TOGGLE_SIDEBAR),
        tr("Show Sidebar"), this);
    connect(action, SIGNAL(triggered()), this, SIGNAL(showHideSidebar()));

#ifdef Q_WS_MAC
    reset->setShortcut(QKeySequence(Qt::ALT + Qt::Key_0));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
    ctrlTab->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Tab));
    ctrlShiftTab->setShortcut(QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Tab));
#else
    reset->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_0));
    ctrlTab->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Tab));
    ctrlShiftTab->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Tab));
#endif

    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);

    QStatusBar *statusbar = statusBar();
    statusbar->show();
    statusbar->setProperty("p_styled", true);
    statusbar->addPermanentWidget(button);

    QWidget *w = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(w);
    layout->addStretch(1);
    statusbar->insertWidget(1, w, 1);

    installEventFilter(this);
    setWindowTitle(tr("Qt Creator Offline Help"));
}

ExternalHelpWindow::~ExternalHelpWindow()
{
}

void ExternalHelpWindow::closeEvent(QCloseEvent *event)
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(Help::Constants::ID_MODE_HELP);
    settings->setValue(QLatin1String("geometry"), saveGeometry());
    settings->endGroup();

    QMainWindow::closeEvent(event);
}

bool ExternalHelpWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == this) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*> (event);
            switch (keyEvent->key()) {
                case Qt::Key_Escape:
                    Core::ICore::instance()->mainWindow()->activateWindow();
                default:
                    break;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
