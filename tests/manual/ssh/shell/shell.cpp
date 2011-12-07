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
#include "shell.h"

#include <utils/ssh/sshconnection.h>
#include <utils/ssh/sshremoteprocess.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QSocketNotifier>

#include <cstdlib>
#include <iostream>

using namespace Utils;

Shell::Shell(const Utils::SshConnectionParameters &parameters, QObject *parent)
    : QObject(parent),
      m_connection(SshConnection::create(parameters)),
      m_stdin(new QFile(this))
{
    connect(m_connection.data(), SIGNAL(connected()), SLOT(handleConnected()));
    connect(m_connection.data(), SIGNAL(dataAvailable(QString)), SLOT(handleShellMessage(QString)));
    connect(m_connection.data(), SIGNAL(error(Utils::SshError)), SLOT(handleConnectionError()));
}

Shell::~Shell()
{
}

void Shell::run()
{
    if (!m_stdin->open(stdin, QIODevice::ReadOnly | QIODevice::Unbuffered)) {
        std::cerr << "Error: Cannot read from standard input." << std::endl;
        qApp->exit(EXIT_FAILURE);
        return;
    }

    m_connection->connectToHost();
}

void Shell::handleConnectionError()
{
    std::cerr << "SSH connection error: " << qPrintable(m_connection->errorString()) << std::endl;
    qApp->exit(EXIT_FAILURE);
}

void Shell::handleShellMessage(const QString &message)
{
    std::cout << qPrintable(message);
}

void Shell::handleConnected()
{
    m_shell = m_connection->createRemoteShell();
    connect(m_shell.data(), SIGNAL(started()), SLOT(handleShellStarted()));
    connect(m_shell.data(), SIGNAL(readyReadStandardOutput()), SLOT(handleRemoteStdout()));
    connect(m_shell.data(), SIGNAL(readyReadStandardError()), SLOT(handleRemoteStderr()));
    connect(m_shell.data(), SIGNAL(closed(int)), SLOT(handleChannelClosed(int)));
    m_shell->start();
}

void Shell::handleShellStarted()
{
    QSocketNotifier * const notifier = new QSocketNotifier(0, QSocketNotifier::Read, this);
    connect(notifier, SIGNAL(activated(int)), SLOT(handleStdin()));
}

void Shell::handleRemoteStdout()
{
    std::cout << m_shell->readAllStandardOutput().data() << std::flush;
}

void Shell::handleRemoteStderr()
{
    std::cerr << m_shell->readAllStandardError().data() << std::flush;
}

void Shell::handleChannelClosed(int exitStatus)
{
    std::cerr << "Shell closed. Exit status was " << exitStatus << ", exit code was "
        << m_shell->exitCode() << "." << std::endl;
    qApp->exit(exitStatus == SshRemoteProcess::ExitedNormally && m_shell->exitCode() == 0
        ? EXIT_SUCCESS : EXIT_FAILURE);
}

void Shell::handleStdin()
{
    m_shell->write(m_stdin->readLine());
}
