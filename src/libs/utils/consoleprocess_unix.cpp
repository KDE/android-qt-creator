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

#include "consoleprocess.h"

#include "environment.h"
#include "qtcprocess.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryFile>

#include <QtNetwork/QLocalSocket>
#include <QtNetwork/QLocalServer>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

namespace Utils {
struct ConsoleProcessPrivate {
    ConsoleProcessPrivate();

    ConsoleProcess::Mode m_mode;
    qint64 m_appPid;
    qint64 m_appMainThreadId;
    int m_appCode;
    QString m_executable;
    QProcess::ExitStatus m_appStatus;
    QLocalServer m_stubServer;
    QLocalSocket *m_stubSocket;
    QTemporaryFile *m_tempFile;

    QProcess m_process;
    QByteArray m_stubServerDir;
    QSettings *m_settings;
};

ConsoleProcessPrivate::ConsoleProcessPrivate() :
    m_mode(ConsoleProcess::Run),
    m_appPid(0),
    m_stubSocket(0),
    m_tempFile(0),
    m_settings(0)
{
}
ConsoleProcess::ConsoleProcess(QObject *parent)  :
    QObject(parent), d(new ConsoleProcessPrivate)
{
    connect(&d->m_stubServer, SIGNAL(newConnection()), SLOT(stubConnectionAvailable()));

    d->m_process.setProcessChannelMode(QProcess::ForwardedChannels);
    connect(&d->m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            SLOT(stubExited()));
}

ConsoleProcess::~ConsoleProcess()
{
    stop();
}

void ConsoleProcess::setMode(Mode m)
{
    d->m_mode = m;
}

ConsoleProcess::Mode ConsoleProcess::mode() const
{
    return d->m_mode;
}

qint64 ConsoleProcess::applicationPID() const
{
    return d->m_appPid;
}

int ConsoleProcess::exitCode() const
{
    return d->m_appCode;
} // This will be the signal number if exitStatus == CrashExit

QProcess::ExitStatus ConsoleProcess::exitStatus() const
{
    return d->m_appStatus;
}

void ConsoleProcess::setSettings(QSettings *settings)
{
    d->m_settings = settings;
}

bool ConsoleProcess::start(const QString &program, const QString &args)
{
    if (isRunning())
        return false;

    QtcProcess::SplitError perr;
    QStringList pargs = QtcProcess::prepareArgs(args, &perr, &m_environment, &m_workingDir);
    QString pcmd;
    if (perr == QtcProcess::SplitOk) {
        pcmd = program;
    } else {
        if (perr != QtcProcess::FoundMeta) {
            emit processMessage(tr("Quoting error in command."), true);
            return false;
        }
        if (d->m_mode == Debug) {
            // FIXME: QTCREATORBUG-2809
            emit processMessage(tr("Debugging complex shell commands in a terminal"
                                   " is currently not supported."), true);
            return false;
        }
        pcmd = QLatin1String("/bin/sh");
        pargs << QLatin1String("-c") << (QtcProcess::quoteArg(program) + QLatin1Char(' ') + args);
    }

    QtcProcess::SplitError qerr;
    QStringList xtermArgs = QtcProcess::prepareArgs(terminalEmulator(d->m_settings), &qerr,
                                                    &m_environment, &m_workingDir);
    if (qerr != QtcProcess::SplitOk) {
        emit processMessage(qerr == QtcProcess::BadQuoting
                            ? tr("Quoting error in terminal command.")
                            : tr("Terminal command may not be a shell command."), true);
        return false;
    }

    const QString err = stubServerListen();
    if (!err.isEmpty()) {
        emit processMessage(msgCommChannelFailed(err), true);
        return false;
    }

    QStringList env = m_environment.toStringList();
    if (!env.isEmpty()) {
        d->m_tempFile = new QTemporaryFile();
        if (!d->m_tempFile->open()) {
            stubServerShutdown();
            emit processMessage(msgCannotCreateTempFile(d->m_tempFile->errorString()), true);
            delete d->m_tempFile;
            d->m_tempFile = 0;
            return false;
        }
        QByteArray contents;
        foreach (const QString &var, env) {
            QByteArray l8b = var.toLocal8Bit();
            contents.append(l8b.constData(), l8b.size() + 1);
        }
        if (d->m_tempFile->write(contents) != contents.size() || !d->m_tempFile->flush()) {
            stubServerShutdown();
            emit processMessage(msgCannotWriteTempFile(), true);
            delete d->m_tempFile;
            d->m_tempFile = 0;
            return false;
        }
    }

    xtermArgs
#ifdef Q_OS_MAC
              << (QCoreApplication::applicationDirPath() + QLatin1String("/../Resources/qtcreator_process_stub"))
#else
              << (QCoreApplication::applicationDirPath() + QLatin1String("/qtcreator_process_stub"))
#endif
              << modeOption(d->m_mode)
              << d->m_stubServer.fullServerName()
              << msgPromptToClose()
              << workingDirectory()
              << (d->m_tempFile ? d->m_tempFile->fileName() : QString())
              << pcmd << pargs;

    QString xterm = xtermArgs.takeFirst();
    d->m_process.start(xterm, xtermArgs);
    if (!d->m_process.waitForStarted()) {
        stubServerShutdown();
        emit processMessage(tr("Cannot start the terminal emulator '%1'.").arg(xterm), true);
        delete d->m_tempFile;
        d->m_tempFile = 0;
        return false;
    }
    d->m_executable = program;
    emit wrapperStarted();
    return true;
}

void ConsoleProcess::stop()
{
    if (!isRunning())
        return;
    stubServerShutdown();
    d->m_appPid = 0;
    d->m_process.terminate();
    if (!d->m_process.waitForFinished(1000))
        d->m_process.kill();
    d->m_process.waitForFinished();
}

bool ConsoleProcess::isRunning() const
{
    return d->m_process.state() != QProcess::NotRunning;
}

QString ConsoleProcess::stubServerListen()
{
    // We need to put the socket in a private directory, as some systems simply do not
    // check the file permissions of sockets.
    QString stubFifoDir;
    forever {
        {
            QTemporaryFile tf;
            if (!tf.open())
                return msgCannotCreateTempFile(tf.errorString());
            stubFifoDir = QFile::encodeName(tf.fileName());
        }
        // By now the temp file was deleted again
        d->m_stubServerDir = QFile::encodeName(stubFifoDir);
        if (!::mkdir(d->m_stubServerDir.constData(), 0700))
            break;
        if (errno != EEXIST)
            return msgCannotCreateTempDir(stubFifoDir, QString::fromLocal8Bit(strerror(errno)));
    }
    const QString stubServer  = stubFifoDir + "/stub-socket";
    if (!d->m_stubServer.listen(stubServer)) {
        ::rmdir(d->m_stubServerDir.constData());
        return tr("Cannot create socket '%1': %2").arg(stubServer, d->m_stubServer.errorString());
    }
    return QString();
}

void ConsoleProcess::stubServerShutdown()
{
    delete d->m_stubSocket;
    d->m_stubSocket = 0;
    if (d->m_stubServer.isListening()) {
        d->m_stubServer.close();
        ::rmdir(d->m_stubServerDir.constData());
    }
}

void ConsoleProcess::stubConnectionAvailable()
{
    d->m_stubSocket = d->m_stubServer.nextPendingConnection();
    connect(d->m_stubSocket, SIGNAL(readyRead()), SLOT(readStubOutput()));
}

static QString errorMsg(int code)
{
    return QString::fromLocal8Bit(strerror(code));
}

void ConsoleProcess::readStubOutput()
{
    while (d->m_stubSocket->canReadLine()) {
        QByteArray out = d->m_stubSocket->readLine();
        out.chop(1); // \n
        if (out.startsWith("err:chdir ")) {
            emit processMessage(msgCannotChangeToWorkDir(workingDirectory(), errorMsg(out.mid(10).toInt())), true);
        } else if (out.startsWith("err:exec ")) {
            emit processMessage(msgCannotExecute(d->m_executable, errorMsg(out.mid(9).toInt())), true);
        } else if (out.startsWith("pid ")) {
            // Will not need it any more
            delete d->m_tempFile;
            d->m_tempFile = 0;

            d->m_appPid = out.mid(4).toInt();
            emit processStarted();
        } else if (out.startsWith("exit ")) {
            d->m_appStatus = QProcess::NormalExit;
            d->m_appCode = out.mid(5).toInt();
            d->m_appPid = 0;
            emit processStopped();
        } else if (out.startsWith("crash ")) {
            d->m_appStatus = QProcess::CrashExit;
            d->m_appCode = out.mid(6).toInt();
            d->m_appPid = 0;
            emit processStopped();
        } else {
            emit processMessage(msgUnexpectedOutput(out), true);
            d->m_process.terminate();
            break;
        }
    }
}

void ConsoleProcess::stubExited()
{
    // The stub exit might get noticed before we read the error status.
    if (d->m_stubSocket && d->m_stubSocket->state() == QLocalSocket::ConnectedState)
        d->m_stubSocket->waitForDisconnected();
    stubServerShutdown();
    delete d->m_tempFile;
    d->m_tempFile = 0;
    if (d->m_appPid) {
        d->m_appStatus = QProcess::CrashExit;
        d->m_appCode = -1;
        d->m_appPid = 0;
        emit processStopped(); // Maybe it actually did not, but keep state consistent
    }
    emit wrapperStopped();
}

QString ConsoleProcess::defaultTerminalEmulator()
{
// FIXME: enable this once runInTerminal works nicely
#if 0 //def Q_OS_MAC
    return QDir::cleanPath(QCoreApplication::applicationDirPath()
                           + QLatin1String("/../Resources/runInTerminal.command"));
#else
    return QLatin1String("xterm");
#endif
}

QString ConsoleProcess::terminalEmulator(const QSettings *settings)
{
    const QString dflt = defaultTerminalEmulator() + QLatin1String(" -e");
    if (!settings)
        return dflt;
    return settings->value(QLatin1String("General/TerminalEmulator"), dflt).toString();
}

void ConsoleProcess::setTerminalEmulator(QSettings *settings, const QString &term)
{
    return settings->setValue(QLatin1String("General/TerminalEmulator"), term);
}
} // namespace Utils
