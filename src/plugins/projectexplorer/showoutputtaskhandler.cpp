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

#include "showoutputtaskhandler.h"

#include "projectexplorerconstants.h"
#include "task.h"

#include "compileoutputwindow.h"

#include <QtGui/QAction>

using namespace ProjectExplorer::Internal;

ShowOutputTaskHandler::ShowOutputTaskHandler(CompileOutputWindow *window) :
    ITaskHandler(QLatin1String(Constants::SHOW_TASK_OUTPUT)),
    m_window(window)
{
    Q_ASSERT(m_window);
}

bool ShowOutputTaskHandler::canHandle(const ProjectExplorer::Task &task)
{
    return m_window->knowsPositionOf(task);
}

void ShowOutputTaskHandler::handle(const ProjectExplorer::Task &task)
{
    Q_ASSERT(canHandle(task));
    m_window->popup(); // popup first as this does move the visible area!
    m_window->showPositionOf(task);
}

QAction *ShowOutputTaskHandler::createAction(QObject *parent)
{
    QAction *outputAction = new QAction(tr("Show &Output"), parent);
    outputAction->setToolTip(tr("Show output generating this issue."));
    return outputAction;
}
