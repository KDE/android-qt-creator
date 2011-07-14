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

#include <QtScript/qscriptengine.h>
#include <QtScript/qscriptvalue.h>
#include <QtCore/qtimer.h>

#include "qdeclarativecanvastimer_p.h"

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QList<CanvasTimer*> , activeTimers);

CanvasTimer::CanvasTimer(QObject *parent, const QScriptValue &data)
    : QTimer(parent), m_value(data)
{
}

void CanvasTimer::handleTimeout()
{
    Q_ASSERT(m_value.isFunction());
    m_value.call();
    if (isSingleShot()) {
        removeTimer(this);
    }
}

void CanvasTimer::createTimer(QObject *parent, const QScriptValue &val, long timeout, bool singleshot)
{

    CanvasTimer *timer = new CanvasTimer(parent, val);
    timer->setInterval(timeout);
    timer->setSingleShot(singleshot);
    connect(timer, SIGNAL(timeout()), timer, SLOT(handleTimeout()));
    activeTimers()->append(timer);
    timer->start();
}

void CanvasTimer::removeTimer(CanvasTimer *timer)
{
    activeTimers()->removeAll(timer);
    timer->deleteLater();
}

void CanvasTimer::removeTimer(const QScriptValue &val)
{
    if (!val.isFunction())
        return;

    for (int i = 0 ; i < activeTimers()->count() ; ++i) {
        CanvasTimer *timer = activeTimers()->at(i);
        if (timer->equals(val)) {
            removeTimer(timer);
            return;
        }
    }
}

QT_END_NAMESPACE

