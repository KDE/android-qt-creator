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

#include "remoteprocesstest.h"

#include <utils/ssh/sshpseudoterminal.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>

#include <iostream>

using namespace Utils;

const QByteArray StderrOutput("ChannelTest");

RemoteProcessTest::RemoteProcessTest(const SshConnectionParameters &params)
    : m_sshParams(params),
      m_timeoutTimer(new QTimer(this)),
      m_remoteRunner(new SshRemoteProcessRunner(this)),
      m_state(Inactive)
{
    m_timeoutTimer->setInterval(5000);
    connect(m_timeoutTimer, SIGNAL(timeout()), SLOT(handleTimeout()));
}

RemoteProcessTest::~RemoteProcessTest() { }

void RemoteProcessTest::run()
{
    connect(m_remoteRunner, SIGNAL(connectionError()),
        SLOT(handleConnectionError()));
    connect(m_remoteRunner, SIGNAL(processStarted()),
        SLOT(handleProcessStarted()));
    connect(m_remoteRunner, SIGNAL(processOutputAvailable(QByteArray)),
        SLOT(handleProcessStdout(QByteArray)));
    connect(m_remoteRunner, SIGNAL(processErrorOutputAvailable(QByteArray)),
        SLOT(handleProcessStderr(QByteArray)));
    connect(m_remoteRunner, SIGNAL(processClosed(int)),
        SLOT(handleProcessClosed(int)));

    std::cout << "Testing successful remote process... " << std::flush;
    m_state = TestingSuccess;
    m_started = false;
    m_timeoutTimer->start();
    m_remoteRunner->run("ls -a /tmp", m_sshParams);
}

void RemoteProcessTest::handleConnectionError()
{
    const QString error = m_state == TestingIoDevice || m_state == TestingProcessChannels
        ? m_sshConnection->errorString() : m_remoteRunner->lastConnectionErrorString();

    std::cerr << "Error: Connection failure (" << qPrintable(error) << ")." << std::endl;
    qApp->quit();
}

void RemoteProcessTest::handleProcessStarted()
{
    if (m_started) {
        std::cerr << "Error: Received started() signal again." << std::endl;
        qApp->quit();
    } else {
        m_started = true;
        if (m_state == TestingCrash) {
            Utils::SshRemoteProcessRunner * const killer
                = new Utils::SshRemoteProcessRunner(this);
            killer->run("pkill -9 sleep", m_sshParams);
        } else if (m_state == TestingIoDevice) {
            connect(m_catProcess.data(), SIGNAL(readyRead()), SLOT(handleReadyRead()));
            m_textStream = new QTextStream(m_catProcess.data());
            *m_textStream << testString();
            m_textStream->flush();
        }
    }
}

void RemoteProcessTest::handleProcessStdout(const QByteArray &output)
{
    if (!m_started) {
        std::cerr << "Error: Remote output from non-started process."
            << std::endl;
        qApp->quit();
    } else if (m_state != TestingSuccess && m_state != TestingTerminal) {
        std::cerr << "Error: Got remote standard output in state " << m_state
            << "." << std::endl;
        qApp->quit();
    } else {
        m_remoteStdout += output;
    }
}

void RemoteProcessTest::handleProcessStderr(const QByteArray &output)
{
    if (!m_started) {
        std::cerr << "Error: Remote error output from non-started process."
            << std::endl;
        qApp->quit();
    } else if (m_state == TestingSuccess) {
        std::cerr << "Error: Unexpected remote standard error output."
            << std::endl;
        qApp->quit();
    } else {
        m_remoteStderr += output;
    }
}

void RemoteProcessTest::handleProcessClosed(int exitStatus)
{
    switch (exitStatus) {
    case SshRemoteProcess::ExitedNormally:
        if (!m_started) {
            std::cerr << "Error: Process exited without starting." << std::endl;
            qApp->quit();
            return;
        }
        switch (m_state) {
        case TestingSuccess: {
            const int exitCode = m_remoteRunner->processExitCode();
            if (exitCode != 0) {
                std::cerr << "Error: exit code is " << exitCode
                    << ", expected zero." << std::endl;
                qApp->quit();
                return;
            }
            if (m_remoteStdout.isEmpty()) {
                std::cerr << "Error: Command did not produce output."
                    << std::endl;
                qApp->quit();
                return;
            }

            std::cout << "Ok.\nTesting unsuccessful remote process... " << std::flush;
            m_state = TestingFailure;
            m_started = false;
            m_timeoutTimer->start();
            m_remoteRunner->run("top -n 1", m_sshParams); // Does not succeed without terminal.
            break;
        }
        case TestingFailure: {
            const int exitCode = m_remoteRunner->processExitCode();
            if (exitCode == 0) {
                std::cerr << "Error: exit code is zero, expected non-zero."
                    << std::endl;
                qApp->quit();
                return;
            }
            if (m_remoteStderr.isEmpty()) {
                std::cerr << "Error: Command did not produce error output." << std::flush;
                qApp->quit();
                return;
            }

            std::cout << "Ok.\nTesting crashing remote process... " << std::flush;
            m_state = TestingCrash;
            m_started = false;
            m_timeoutTimer->start();
            m_remoteRunner->run("sleep 100", m_sshParams);
            break;
        }
        case TestingCrash:
            std::cerr << "Error: Successful exit from process that was "
                "supposed to crash." << std::endl;
            qApp->quit();
            break;
        case TestingTerminal: {
            const int exitCode = m_remoteRunner->processExitCode();
            if (exitCode != 0) {
                std::cerr << "Error: exit code is " << exitCode
                    << ", expected zero." << std::endl;
                qApp->quit();
                return;
            }
            if (m_remoteStdout.isEmpty()) {
                std::cerr << "Error: Command did not produce output."
                    << std::endl;
                qApp->quit();
                return;
            }
            std::cout << "Ok.\nTesting I/O device functionality... " << std::flush;
            m_state = TestingIoDevice;
            m_sshConnection = Utils::SshConnection::create(m_sshParams);
            connect(m_sshConnection.data(), SIGNAL(connected()), SLOT(handleConnected()));
            connect(m_sshConnection.data(), SIGNAL(error(Utils::SshError)),
                SLOT(handleConnectionError()));
            m_sshConnection->connectToHost();
            m_timeoutTimer->start();
            break;
        }
        case TestingIoDevice:
            std::cerr << "Error: Successful exit from process that was supposed to crash."
                << std::endl;
            qApp->exit(EXIT_FAILURE);
            break;
        case TestingProcessChannels:
            if (m_remoteStderr.isEmpty()) {
                std::cerr << "Error: Did not receive readyReadStderr()." << std::endl;
                qApp->exit(EXIT_FAILURE);
                return;
            }
            if (m_remoteData != StderrOutput) {
                std::cerr << "Error: Expected output '" << StderrOutput.data() << "', received '"
                    << m_remoteData.data() << "'." << std::endl;
                qApp->exit(EXIT_FAILURE);
                return;
            }
            std::cout << "Ok.\nAll tests succeeded." << std::endl;
            qApp->quit();
            break;
        case Inactive:
            Q_ASSERT(false);
        }
        break;
    case SshRemoteProcess::FailedToStart:
        if (m_started) {
            std::cerr << "Error: Got 'failed to start' signal for process "
                "that has not started yet." << std::endl;
        } else {
            std::cerr << "Error: Process failed to start." << std::endl;
        }
        qApp->quit();
        break;
    case SshRemoteProcess::KilledBySignal:
        switch (m_state) {
        case TestingCrash:
            std::cout << "Ok.\nTesting remote process with terminal... " << std::flush;
            m_state = TestingTerminal;
            m_started = false;
            m_timeoutTimer->start();
            m_remoteRunner->runInTerminal("top -n 1", SshPseudoTerminal(), m_sshParams);
            break;
        case TestingIoDevice:
            std::cout << "Ok\nTesting process channels... " << std::flush;
            m_state = TestingProcessChannels;
            m_started = false;
            m_remoteStderr.clear();
            m_echoProcess = m_sshConnection->createRemoteProcess("printf " + StderrOutput + " >&2");
            m_echoProcess->setReadChannel(QProcess::StandardError);
            connect(m_echoProcess.data(), SIGNAL(started()), SLOT(handleProcessStarted()));
            connect(m_echoProcess.data(), SIGNAL(closed(int)), SLOT(handleProcessClosed(int)));
            connect(m_echoProcess.data(), SIGNAL(readyRead()), SLOT(handleReadyRead()));
            connect(m_echoProcess.data(), SIGNAL(readyReadStandardError()),
                SLOT(handleReadyReadStderr()));
            m_echoProcess->start();
            m_timeoutTimer->start();
            break;
        default:
            std::cerr << "Error: Unexpected crash." << std::endl;
            qApp->quit();
            return;
        }
    }
}

void RemoteProcessTest::handleTimeout()
{
    std::cerr << "Error: Timeout waiting for progress." << std::endl;
    qApp->quit();
}

void RemoteProcessTest::handleConnected()
{
    Q_ASSERT(m_state == TestingIoDevice);

    m_catProcess = m_sshConnection->createRemoteProcess(QString::fromLocal8Bit("cat").toUtf8());
    connect(m_catProcess.data(), SIGNAL(started()), SLOT(handleProcessStarted()));
    connect(m_catProcess.data(), SIGNAL(closed(int)), SLOT(handleProcessClosed(int)));
    m_started = false;
    m_timeoutTimer->start();
    m_catProcess->start();
}

QString RemoteProcessTest::testString() const
{
    return QLatin1String("x");
}

void RemoteProcessTest::handleReadyRead()
{
    switch (m_state) {
    case TestingIoDevice: {
        const QString &data = QString::fromUtf8(m_catProcess->readAll());
        if (data != testString()) {
            std::cerr << "Testing of QIODevice functionality failed: Expected '"
                << qPrintable(testString()) << "', got '" << qPrintable(data) << "'." << std::endl;
            qApp->exit(1);
        }
        Utils::SshRemoteProcessRunner * const killer = new Utils::SshRemoteProcessRunner(this);
        killer->run("pkill -9 cat", m_sshParams);
        break;
    }
    case TestingProcessChannels:
        m_remoteData += m_echoProcess->readAll();
        break;
    default:
        qFatal("%s: Unexpected state %d.", Q_FUNC_INFO, m_state);
    }

}

void RemoteProcessTest::handleReadyReadStdout()
{
    Q_ASSERT(m_state == TestingProcessChannels);

    std::cerr << "Error: Received unexpected stdout data." << std::endl;
    qApp->exit(EXIT_FAILURE);
}

void RemoteProcessTest::handleReadyReadStderr()
{
    Q_ASSERT(m_state == TestingProcessChannels);

    m_remoteStderr = "dummy";
}
