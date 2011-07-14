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

#include <utils/ssh/sftpchannel.h>
#include <utils/ssh/sshconnection.h>
#include <utils/ssh/sshremoteprocess.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QTimer>

using namespace Utils;

class Test : public QObject {
    Q_OBJECT
public:
    Test()
    {
        m_timeoutTimer.setSingleShot(true);
        m_connection = SshConnection::create(SshConnectionParameters(SshConnectionParameters::DefaultProxy));
        if (m_connection->state() != SshConnection::Unconnected) {
            qDebug("Error: Newly created SSH connection has state %d.",
                m_connection->state());
        }

        if (m_connection->createRemoteProcess(""))
            qDebug("Error: Unconnected SSH connection creates remote process.");
        if (m_connection->createSftpChannel())
            qDebug("Error: Unconnected SSH connection creates SFTP channel.");

        SshConnectionParameters noHost=SshConnectionParameters(SshConnectionParameters::DefaultProxy);
        noHost.host = QLatin1String("hgdfxgfhgxfhxgfchxgcf");
        noHost.port = 12345;
        noHost.timeout = 10;

        SshConnectionParameters noUser=SshConnectionParameters(SshConnectionParameters::DefaultProxy);
        noUser.host = QLatin1String("localhost");
        noUser.port = 22;
        noUser.timeout = 30;
        noUser.authenticationType = SshConnectionParameters::AuthenticationByPassword;
        noUser.userName = QLatin1String("dumdidumpuffpuff");
        noUser.password = QLatin1String("whatever");

        SshConnectionParameters wrongPwd=SshConnectionParameters(SshConnectionParameters::DefaultProxy);
        wrongPwd.host = QLatin1String("localhost");
        wrongPwd.port = 22;
        wrongPwd.timeout = 30;
        wrongPwd.authenticationType = SshConnectionParameters::AuthenticationByPassword;
        wrongPwd.userName = QLatin1String("root");
        noUser.password = QLatin1String("thiscantpossiblybeapasswordcanit");

        SshConnectionParameters invalidKeyFile=SshConnectionParameters(SshConnectionParameters::DefaultProxy);
        invalidKeyFile.host = QLatin1String("localhost");
        invalidKeyFile.port = 22;
        invalidKeyFile.timeout = 30;
        invalidKeyFile.authenticationType = SshConnectionParameters::AuthenticationByKey;
        invalidKeyFile.userName = QLatin1String("root");
        invalidKeyFile.privateKeyFile
            = QLatin1String("somefilenamethatwedontexpecttocontainavalidkey");

        // TODO: Create a valid key file and check for authentication error.

        m_testSet << TestItem("Behavior with non-existing host",
            noHost, ErrorList() << SshSocketError);
        m_testSet << TestItem("Behavior with non-existing user", noUser,
            ErrorList() << SshSocketError << SshTimeoutError
                << SshAuthenticationError);
        m_testSet << TestItem("Behavior with wrong password", wrongPwd,
            ErrorList() << SshSocketError << SshTimeoutError
                << SshAuthenticationError);
        m_testSet << TestItem("Behavior with invalid key file", invalidKeyFile,
            ErrorList() << SshSocketError << SshTimeoutError
                << SshKeyFileError);

        runNextTest();
    }

    ~Test();

private slots:
    void handleConnected()
    {
        qDebug("Error: Received unexpected connected() signal.");
        qApp->quit();
    }

    void handleDisconnected()
    {
        qDebug("Error: Received unexpected disconnected() signal.");
        qApp->quit();
    }

    void handleDataAvailable(const QString &msg)
    {
        qDebug("Error: Received unexpected dataAvailable() signal. "
            "Message was: '%s'.", qPrintable(msg));
        qApp->quit();
    }

    void handleError(Utils::SshError error)
    {
        if (m_testSet.isEmpty()) {
            qDebug("Error: Received error %d, but no test was running.", error);
            qApp->quit();
        }

        const TestItem testItem = m_testSet.takeFirst();
        if (testItem.allowedErrors.contains(error)) {
            qDebug("Received error %d, as expected.", error);
            if (m_testSet.isEmpty()) {
                qDebug("All tests finished successfully.");
                qApp->quit();
            } else {
                runNextTest();
            }
        } else {
            qDebug("Received unexpected error %d.", error);
            qApp->quit();
        }
    }

    void handleTimeout()
    {
        if (m_testSet.isEmpty()) {
            qDebug("Error: timeout, but no test was running.");
            qApp->quit();
        }
        const TestItem testItem = m_testSet.takeFirst();
        qDebug("Error: The following test timed out: %s", testItem.description);
    }

private:
    void runNextTest()
    {
        if (m_connection)
            disconnect(m_connection.data(), 0, this, 0);
        m_connection = SshConnection::create(m_testSet.first().params);
        connect(m_connection.data(), SIGNAL(connected()), this,
            SLOT(handleConnected()));
        connect(m_connection.data(), SIGNAL(disconnected()), this,
            SLOT(handleDisconnected()));
        connect(m_connection.data(), SIGNAL(dataAvailable(QString)), this,
            SLOT(handleDataAvailable(QString)));
        connect(m_connection.data(), SIGNAL(error(Utils::SshError)), this,
            SLOT(handleError(Utils::SshError)));
        const TestItem &nextItem = m_testSet.first();
        m_timeoutTimer.stop();
        m_timeoutTimer.setInterval(qMax(10000, nextItem.params.timeout * 1000));
        qDebug("Testing: %s", nextItem.description);
        m_connection->connectToHost();
    }

    SshConnection::Ptr m_connection;
    typedef QList<SshError> ErrorList;
    struct TestItem {
        TestItem(const char *d, const SshConnectionParameters &p,
            const ErrorList &e) : description(d), params(p), allowedErrors(e) {}

        const char *description;
        SshConnectionParameters params;
        ErrorList allowedErrors;
    };
    QList<TestItem> m_testSet;
    QTimer m_timeoutTimer;
};

Test::~Test() {}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Test t;

    return a.exec();
}


#include "main.moc"
