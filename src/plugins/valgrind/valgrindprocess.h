/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#ifndef VALGRIND_RUNNER_P_H
#define VALGRIND_RUNNER_P_H

#include <utils/qtcprocess.h>
#include <utils/ssh/sshremoteprocess.h>
#include <utils/ssh/sshconnection.h>
#include <utils/outputformat.h>

namespace Valgrind {

/**
 * Abstract process that can be subclassed to supply local and remote valgrind runs
 */
class ValgrindProcess : public QObject
{
    Q_OBJECT

public:
    explicit ValgrindProcess(QObject *parent = 0);

    virtual bool isRunning() const = 0;

    virtual void run(const QString &valgrindExecutable, const QStringList &valgrindArguments,
                     const QString &debuggeeExecutable, const QString &debuggeeArguments) = 0;
    virtual void close() = 0;

    virtual QString errorString() const = 0;
    virtual QProcess::ProcessError error() const = 0;

    virtual void setProcessChannelMode(QProcess::ProcessChannelMode mode) = 0;
    virtual void setWorkingDirectory(const QString &path) = 0;
    virtual QString workingDirectory() const = 0;
    virtual void setEnvironment(const Utils::Environment &environment) = 0;

    virtual qint64 pid() const = 0;

signals:
    void started();
    void finished(int, QProcess::ExitStatus);
    void error(QProcess::ProcessError);
    void processOutput(const QByteArray &, Utils::OutputFormat format);
};

/**
 * Run valgrind on the local machine
 */
class LocalValgrindProcess : public ValgrindProcess
{
    Q_OBJECT

public:
    explicit LocalValgrindProcess(QObject *parent = 0);

    virtual bool isRunning() const;

    virtual void run(const QString &valgrindExecutable, const QStringList &valgrindArguments,
                     const QString &debuggeeExecutable, const QString &debuggeeArguments);
    virtual void close();

    virtual QString errorString() const;
    QProcess::ProcessError error() const;

    virtual void setProcessChannelMode(QProcess::ProcessChannelMode mode);
    virtual void setWorkingDirectory(const QString &path);
    virtual QString workingDirectory() const;
    virtual void setEnvironment(const Utils::Environment &environment);

    virtual qint64 pid() const;

private slots:
    void readyReadStandardError();
    void readyReadStandardOutput();

private:
    Utils::QtcProcess m_process;
    qint64 m_pid;
};

/**
 * Run valgrind on a remote machine via SSH
 */
class RemoteValgrindProcess : public ValgrindProcess
{
    Q_OBJECT

public:
    explicit RemoteValgrindProcess(const Utils::SshConnectionParameters &sshParams,
                                   QObject *parent = 0);
    explicit RemoteValgrindProcess(const Utils::SshConnection::Ptr &connection,
                                   QObject *parent = 0);

    virtual bool isRunning() const;

    virtual void run(const QString &valgrindExecutable, const QStringList &valgrindArguments,
                     const QString &debuggeeExecutable, const QString &debuggeeArguments);
    virtual void close();

    virtual QString errorString() const;
    QProcess::ProcessError error() const;

    virtual void setProcessChannelMode(QProcess::ProcessChannelMode mode);
    virtual void setWorkingDirectory(const QString &path);
    virtual QString workingDirectory() const;
    virtual void setEnvironment(const Utils::Environment &environment);

    virtual qint64 pid() const;

    Utils::SshConnection::Ptr connection() const;

private slots:
    void closed(int);
    void connected();
    void error(Utils::SshError error);
    void processStarted();
    void findPIDOutputReceived(const QByteArray &output);
    void standardOutput(const QByteArray &output);
    void standardError(const QByteArray &output);

private:
    Utils::SshConnectionParameters m_params;
    Utils::SshConnection::Ptr m_connection;
    Utils::SshRemoteProcess::Ptr m_process;
    QString m_workingDir;
    QString m_valgrindExe;
    QStringList m_valgrindArgs;
    QString m_debuggee;
    QString m_debuggeeArgs;
    QString m_errorString;
    QProcess::ProcessError m_error;
    qint64 m_pid;
    Utils::SshRemoteProcess::Ptr m_findPID;
};

} // namespace Valgrind

#endif // VALGRIND_RUNNER_P_H
