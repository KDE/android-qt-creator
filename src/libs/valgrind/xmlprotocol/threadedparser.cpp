/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "threadedparser.h"
#include "parser.h"
#include "error.h"
#include "frame.h"
#include "status.h"
#include "suppression.h"
#include <utils/qtcassert.h>

#include <QtCore/QMetaType>
#include <QtCore/QThread>
#include <QtCore/QSharedPointer>

using namespace Valgrind;
using namespace Valgrind::XmlProtocol;

namespace {

class Thread : public QThread {
public:
    Thread()
        : QThread()
        , parser(0)
        , device(0)
    {
    }

    void run() {
        QTC_ASSERT(QThread::currentThread() == this, return);
        parser->parse(device);
        delete parser;
        parser = 0;
        delete device;
        device = 0;
    }

    XmlProtocol::Parser *parser;
    QIODevice *device;
};
}
class ThreadedParser::Private
{
public:
    Private()
    {}

    QWeakPointer<Thread> parserThread;
    QString errorString;
};


ThreadedParser::ThreadedParser(QObject *parent)
    : QObject(parent),
      d(new Private)
{
}

ThreadedParser::~ThreadedParser()
{
    delete d;
}

QString ThreadedParser::errorString() const
{
    return d->errorString;
}

bool ThreadedParser::isRunning() const
{
    return d->parserThread ? d->parserThread.data()->isRunning() : 0;
}

void ThreadedParser::parse(QIODevice *device)
{
    QTC_ASSERT(!d->parserThread, return);

    Parser *parser = new Parser;
    qRegisterMetaType<Valgrind::XmlProtocol::Status>();
    qRegisterMetaType<Valgrind::XmlProtocol::Error>();
    connect(parser, SIGNAL(status(Valgrind::XmlProtocol::Status)),
            SIGNAL(status(Valgrind::XmlProtocol::Status)),
            Qt::QueuedConnection);
    connect(parser, SIGNAL(error(Valgrind::XmlProtocol::Error)),
            SIGNAL(error(Valgrind::XmlProtocol::Error)),
            Qt::QueuedConnection);
    connect(parser, SIGNAL(internalError(QString)),
            SLOT(slotInternalError(QString)),
            Qt::QueuedConnection);
    connect(parser, SIGNAL(errorCount(qint64, qint64)),
            SIGNAL(errorCount(qint64, qint64)),
            Qt::QueuedConnection);
    connect(parser, SIGNAL(suppressionCount(QString, qint64)),
            SIGNAL(suppressionCount(QString, qint64)),
            Qt::QueuedConnection);
    connect(parser, SIGNAL(finished()), SIGNAL(finished()),
            Qt::QueuedConnection);


    Thread *thread = new Thread;
    d->parserThread = thread;
    connect(thread, SIGNAL(finished()),
            thread, SLOT(deleteLater()));
    device->setParent(0);
    device->moveToThread(thread);
    parser->moveToThread(thread);
    thread->device = device;
    thread->parser = parser;
    thread->start();
}

void ThreadedParser::slotInternalError(const QString &errorString)
{
    d->errorString = errorString;
    emit internalError(errorString);
}
bool ThreadedParser::waitForFinished()
{
    return d->parserThread ? d->parserThread.data()->wait() : true;
}
