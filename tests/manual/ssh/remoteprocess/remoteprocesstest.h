/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef SFTPTEST_H
#define SFTPTEST_H

#include <utils/ssh/sshremoteprocessrunner.h>

#include <QtCore/QObject>

QT_FORWARD_DECLARE_CLASS(QTextStream)
QT_FORWARD_DECLARE_CLASS(QTimer)

class RemoteProcessTest : public QObject
{
    Q_OBJECT
public:
    RemoteProcessTest(const Utils::SshConnectionParameters &params);
    ~RemoteProcessTest();
    void run();

private slots:
    void handleConnectionError();
    void handleProcessStarted();
    void handleProcessStdout(const QByteArray &output);
    void handleProcessStderr(const QByteArray &output);
    void handleProcessClosed(int exitStatus);
    void handleTimeout();
    void handleReadyRead();
    void handleReadyReadStdout();
    void handleReadyReadStderr();
    void handleConnected();

private:
    enum State {
        Inactive, TestingSuccess, TestingFailure, TestingCrash, TestingTerminal, TestingIoDevice,
        TestingProcessChannels
    };

    QString testString() const;

    const Utils::SshConnectionParameters m_sshParams;
    QTimer * const m_timeoutTimer;
    QTextStream *m_textStream;
    Utils::SshRemoteProcessRunner * const m_remoteRunner;
    Utils::SshRemoteProcess::Ptr m_catProcess;
    Utils::SshRemoteProcess::Ptr m_echoProcess;
    Utils::SshConnection::Ptr m_sshConnection;
    QByteArray m_remoteStdout;
    QByteArray m_remoteStderr;
    QByteArray m_remoteData;
    State m_state;
    bool m_started;
};


#endif // SFTPTEST_H
