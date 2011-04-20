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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef OUTPUT_COLLECTOR_H
#define OUTPUT_COLLECTOR_H

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QLocalSocket;
class QSocketNotifier;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// OutputCollector
//
///////////////////////////////////////////////////////////////////////

class OutputCollector : public QObject
{
    Q_OBJECT

public:
    OutputCollector(QObject *parent = 0);
    ~OutputCollector();
    bool listen();
    void shutdown();
    QString serverName() const;
    QString errorString() const;

signals:
    void byteDelivery(const QByteArray &data);

private slots:
    void bytesAvailable();
#ifdef Q_OS_WIN
    void newConnectionAvailable();
#endif

private:
#ifdef Q_OS_WIN
    QLocalServer *m_server;
    QLocalSocket *m_socket;
#else
    QString m_serverPath;
    int m_serverFd;
    QSocketNotifier *m_serverNotifier;
    QString m_errorString;
#endif
};

} // namespace Internal
} // namespace Debugger

#endif // OUTPUT_COLLECTOR_H
