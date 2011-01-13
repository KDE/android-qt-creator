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

#include "progressview.h"
#include "futureprogress.h"

#include <utils/qtcassert.h>

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>

using namespace Core;
using namespace Core::Internal;

ProgressView::ProgressView(QWidget *parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout;
    setLayout(m_layout);
    m_layout->setMargin(0);
    m_layout->setSpacing(0);
    setWindowTitle(tr("Processes"));
}

ProgressView::~ProgressView()
{
    qDeleteAll(m_taskList);
    m_taskList.clear();
}

FutureProgress *ProgressView::addTask(const QFuture<void> &future,
                                      const QString &title,
                                      const QString &type,
                                      ProgressManager::ProgressFlags flags)
{
    removeOldTasks(type);
    if (m_taskList.size() == 3)
        removeOneOldTask();
    FutureProgress *progress = new FutureProgress(this);
    progress->setTitle(title);
    progress->setFuture(future);

    m_layout->insertWidget(0, progress);
    m_taskList.append(progress);
    progress->setType(type);
    if (flags.testFlag(ProgressManager::KeepOnFinish)) {
        progress->setKeepOnFinish(FutureProgress::KeepOnFinishTillUserInteraction);
    } else {
        progress->setKeepOnFinish(FutureProgress::DontKeepOnFinish);
    }
    connect(progress, SIGNAL(removeMe()), this, SLOT(slotRemoveTask()));
    return progress;
}

void ProgressView::removeOldTasks(const QString &type, bool keepOne)
{
    bool firstFound = !keepOne; // start with false if we want to keep one
    QList<FutureProgress *>::iterator i = m_taskList.end();
    while (i != m_taskList.begin()) {
        --i;
        if ((*i)->type() == type) {
            if (firstFound && (*i)->future().isFinished()) {
                deleteTask(*i);
                i = m_taskList.erase(i);
            }
            firstFound = true;
        }
    }
}

void ProgressView::deleteTask(FutureProgress *progress)
{
    layout()->removeWidget(progress);
    progress->hide();
    progress->deleteLater();
}

void ProgressView::removeOneOldTask()
{
    if (m_taskList.isEmpty())
        return;
    // look for oldest ended process
    for (QList<FutureProgress *>::iterator i = m_taskList.begin(); i != m_taskList.end(); ++i) {
        if ((*i)->future().isFinished()) {
            deleteTask(*i);
            i = m_taskList.erase(i);
            return;
        }
    }
    // no ended process, look for a task type with multiple running tasks and remove the oldest one
    for (QList<FutureProgress *>::iterator i = m_taskList.begin(); i != m_taskList.end(); ++i) {
        QString type = (*i)->type();

        int taskCount = 0;
        foreach (FutureProgress *p, m_taskList)
            if (p->type() == type)
                ++taskCount;

        if (taskCount > 1) { // don't care for optimizations it's only a handful of entries
            deleteTask(*i);
            i = m_taskList.erase(i);
            return;
        }
    }

    // no ended process, no type with multiple processes, just remove the oldest task
    FutureProgress *task = m_taskList.takeFirst();
    deleteTask(task);
}

void ProgressView::removeTask(FutureProgress *task)
{
    m_taskList.removeAll(task);
    deleteTask(task);
}

void ProgressView::slotRemoveTask()
{
    FutureProgress *progress = qobject_cast<FutureProgress *>(sender());
    QTC_ASSERT(progress, return);
    QString type = progress->type();
    removeTask(progress);
    removeOldTasks(type, true);
}
