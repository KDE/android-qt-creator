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

#ifndef QDECLARATIVECANVASTIMER_P_H
#define QDECLARATIVECANVASTIMER_P_H

#include <QtScript/qscriptvalue.h>
#include <QtCore/qtimer.h>
#include <QtCore/qlist.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class CanvasTimer : public QTimer
{
    Q_OBJECT

public:
    CanvasTimer(QObject *parent, const QScriptValue &data);

public Q_SLOTS:
    void handleTimeout();
    bool equals(const QScriptValue &value){return m_value.equals(value);}

public:
    static void createTimer(QObject *parent, const QScriptValue &val, long timeout, bool singleshot);
    static void removeTimer(CanvasTimer *timer);
    static void removeTimer(const QScriptValue &);

private:
    QScriptValue m_value;

};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDECLARATIVECANVASTIMER_P_H
