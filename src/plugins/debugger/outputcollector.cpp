/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "outputcollector.h"

#ifdef Q_OS_WIN

#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>
#include <QtCore/QCoreApplication>

#include <stdlib.h>

#else

#include <QtCore/QFile>
#include <QtCore/QSocketNotifier>
#include <QtCore/QTemporaryFile>
#include <QtCore/QVarLengthArray>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef Q_OS_SOLARIS
# include <sys/filio.h> // FIONREAD
#endif
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#endif

namespace Debugger {
namespace Internal {

OutputCollector::OutputCollector(QObject *parent)
        : QObject(parent)
{
#ifdef Q_OS_WIN
    m_server = 0;
    m_socket = 0;
#endif
}

OutputCollector::~OutputCollector()
{
    shutdown();
}

bool OutputCollector::listen()
{
#ifdef Q_OS_WIN
    if (m_server)
        return m_server->isListening();
    m_server = new QLocalServer(this);
    connect(m_server, SIGNAL(newConnection()), SLOT(newConnectionAvailable()));
    return m_server->listen(QString::fromLatin1("creator-%1-%2")
                            .arg(QCoreApplication::applicationPid())
                            .arg(rand()));
#else
    if (!m_serverPath.isEmpty())
        return true;
    QByteArray codedServerPath;
    forever {
        {
            QTemporaryFile tf;
            if (!tf.open()) {
                m_errorString = tr("Cannot create temporary file: %1").arg(tf.errorString());
                m_serverPath.clear();
                return false;
            }
            m_serverPath = tf.fileName();
        }
        // By now the temp file was deleted again
        codedServerPath = QFile::encodeName(m_serverPath);
        if (!::mkfifo(codedServerPath.constData(), 0600))
            break;
        if (errno != EEXIST) {
            m_errorString = tr("Cannot create FiFo %1: %2").arg(m_serverPath, strerror(errno));
            m_serverPath.clear();
            return false;
        }
    }
    if ((m_serverFd = ::open(codedServerPath.constData(), O_RDONLY|O_NONBLOCK)) < 0) {
        m_errorString = tr("Cannot open FiFo %1: %2").arg(m_serverPath, strerror(errno));
        m_serverPath.clear();
        return false;
    }
    m_serverNotifier = new QSocketNotifier(m_serverFd, QSocketNotifier::Read, this);
    connect(m_serverNotifier, SIGNAL(activated(int)), SLOT(bytesAvailable()));
    return true;
#endif
}

void OutputCollector::shutdown()
{
#ifdef Q_OS_WIN
    delete m_server; // Deletes socket as well (QObject parent)
    m_server = 0;
    m_socket = 0;
#else
    if (!m_serverPath.isEmpty()) {
        ::close(m_serverFd);
        ::unlink(QFile::encodeName(m_serverPath).constData());
        delete m_serverNotifier;
        m_serverPath.clear();
    }
#endif
}

QString OutputCollector::errorString() const
{
#ifdef Q_OS_WIN
    return m_socket ? m_socket->errorString() : m_server->errorString();
#else
    return m_errorString;
#endif
}

QString OutputCollector::serverName() const
{
#ifdef Q_OS_WIN
    return m_server->fullServerName();
#else
    return m_serverPath;
#endif
}

#ifdef Q_OS_WIN
void OutputCollector::newConnectionAvailable()
{
    if (m_socket)
        return;
    m_socket = m_server->nextPendingConnection();
    connect(m_socket, SIGNAL(readyRead()), SLOT(bytesAvailable()));
}
#endif

void OutputCollector::bytesAvailable()
{
#ifdef Q_OS_WIN
    emit byteDelivery(m_socket->readAll());
#else
    size_t nbytes = 0;
    if (::ioctl(m_serverFd, FIONREAD, (char *) &nbytes) < 0)
        return;
    QVarLengthArray<char, 8192> buff(nbytes);
    if (::read(m_serverFd, buff.data(), nbytes) != (int)nbytes)
        return;
    if (nbytes) // Skip EOF notifications
        emit byteDelivery(QByteArray::fromRawData(buff.data(), nbytes));
#endif
}

} // namespace Internal
} // namespace Debugger
