/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "memcheckrunner.h"

#include <xmlprotocol/error.h>
#include <xmlprotocol/status.h>
#include <xmlprotocol/threadedparser.h>

#include <utils/qtcassert.h>

#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtCore/QEventLoop>
#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QVBoxLayout>

namespace Valgrind {
namespace Memcheck {

class MemcheckRunner::Private
{
public:
    explicit Private()
        : parser(0),
          logSocket(0)
    {
    }

    QTcpServer xmlServer;
    XmlProtocol::ThreadedParser *parser;
    QTcpServer logServer;
    QTcpSocket *logSocket;
};

MemcheckRunner::MemcheckRunner(QObject *parent)
    : ValgrindRunner(parent),
      d(new Private)
{
}

MemcheckRunner::~MemcheckRunner()
{
    if (d->parser->isRunning()) {
        // make sure we don't delete the thread while it's still running
        waitForFinished();
    }
    delete d;
    d = 0;
}

QString MemcheckRunner::tool() const
{
    return QString("memcheck");
}

void MemcheckRunner::setParser(XmlProtocol::ThreadedParser *parser)
{
    QTC_ASSERT(!d->parser, qt_noop());
    d->parser = parser;
}

void MemcheckRunner::start()
{
    if (startMode() == Analyzer::StartLocal) {
        QTC_ASSERT(d->parser, return);

        bool check = d->xmlServer.listen(QHostAddress(QHostAddress::LocalHost));
        QTC_ASSERT(check, return);
        d->xmlServer.setMaxPendingConnections(1);
        const quint16 xmlPortNumber = d->xmlServer.serverPort();
        connect(&d->xmlServer, SIGNAL(newConnection()), SLOT(xmlSocketConnected()));

        check = d->logServer.listen(QHostAddress(QHostAddress::LocalHost));
        QTC_ASSERT(check, return);
        d->logServer.setMaxPendingConnections(1);
        const quint16 logPortNumber = d->logServer.serverPort();
        connect(&d->logServer, SIGNAL(newConnection()), SLOT(logSocketConnected()));

        QStringList memcheckArguments;
        memcheckArguments << QString("--xml=yes")
                          << QString("--xml-socket=127.0.0.1:%1").arg(QString::number(xmlPortNumber))
                          << QString("--child-silent-after-fork=yes")
                          << QString("--log-socket=127.0.0.1:%1").arg(QString::number(logPortNumber))
                          << valgrindArguments();
        setValgrindArguments(memcheckArguments);
    }

    if (startMode() == Analyzer::StartRemote) {
        QTC_ASSERT(d->parser, return);

        QList<QHostAddress> possibleHostAddresses;
        //NOTE: ::allAddresses does not seem to work for usb interfaces...
        foreach (const QNetworkInterface &iface, QNetworkInterface::allInterfaces()) {
            foreach (const QNetworkAddressEntry &entry, iface.addressEntries()) {
                const QHostAddress addr = entry.ip();
                if (addr.toString() != "127.0.0.1"
                    && addr.toString() != "0:0:0:0:0:0:0:1")
                {
                    possibleHostAddresses << addr;
                    break;
                }
            }
        }

        QHostAddress hostAddr;

        if (possibleHostAddresses.isEmpty()) {
            emit processErrorReceived(tr("No network interface found for remote analysis."),
                                      QProcess::FailedToStart);
            return;
        } else if (possibleHostAddresses.size() > 1) {
            QDialog dlg;
            dlg.setWindowTitle(tr("Select Network Interface"));
            QVBoxLayout *layout = new QVBoxLayout;
            QLabel *description = new QLabel;
            description->setWordWrap(true);
            description->setText(tr("More than one network interface was found on your machine. Please select which one you want to use for remote analysis."));
            layout->addWidget(description);
            QListWidget *list = new QListWidget;
            foreach (const QHostAddress &address, possibleHostAddresses)
                list->addItem(address.toString());

            list->setSelectionMode(QAbstractItemView::SingleSelection);
            list->setCurrentRow(0);
            layout->addWidget(list);

            QDialogButtonBox *buttons = new QDialogButtonBox;
            buttons->addButton(QDialogButtonBox::Ok);
            buttons->addButton(QDialogButtonBox::Cancel);
            connect(buttons, SIGNAL(accepted()),
                    &dlg, SLOT(accept()));
            connect(buttons, SIGNAL(rejected()),
                    &dlg, SLOT(reject()));
            layout->addWidget(buttons);

            dlg.setLayout(layout);
            if (dlg.exec() != QDialog::Accepted)
                return;

            QTC_ASSERT(list->currentRow() >= 0, return);
            QTC_ASSERT(list->currentRow() < possibleHostAddresses.size(), return);
            hostAddr = possibleHostAddresses.at(list->currentRow());
        } else {
            hostAddr = possibleHostAddresses.first();
        }

        QString ip = hostAddr.toString();
        QTC_ASSERT(!ip.isEmpty(), return);

        bool check = d->xmlServer.listen(hostAddr);
        QTC_ASSERT(check, return);
        d->xmlServer.setMaxPendingConnections(1);
        const quint16 xmlPortNumber = d->xmlServer.serverPort();
        connect(&d->xmlServer, SIGNAL(newConnection()), SLOT(xmlSocketConnected()));

        check = d->logServer.listen(hostAddr);
        QTC_ASSERT(check, return);
        d->logServer.setMaxPendingConnections(1);
        const quint16 logPortNumber = d->logServer.serverPort();
        connect(&d->logServer, SIGNAL(newConnection()), SLOT(logSocketConnected()));

        QStringList memcheckArguments;
        memcheckArguments << QString("--xml=yes")
                          << QString("--xml-socket=%1:%2").arg(ip, QString::number(xmlPortNumber))
                          << QString("--child-silent-after-fork=yes")
                          << QString("--log-socket=%1:%2").arg(ip, QString::number(logPortNumber));
        setValgrindArguments(memcheckArguments);
    }

    ValgrindRunner::start();
}

void MemcheckRunner::xmlSocketConnected()
{
    QTcpSocket *socket = d->xmlServer.nextPendingConnection();
    QTC_ASSERT(socket, return);
    d->xmlServer.close();
    d->parser->parse(socket);
}

void MemcheckRunner::logSocketConnected()
{
    d->logSocket = d->logServer.nextPendingConnection();
    QTC_ASSERT(d->logSocket, return);
    connect(d->logSocket, SIGNAL(readyRead()),
            this, SLOT(readLogSocket()));
    d->logServer.close();
}

void MemcheckRunner::readLogSocket()
{
    emit logMessageReceived(d->logSocket->readAll());
}

} // namespace Memcheck
} // namespace Valgrind
