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

#include "checkoutjobs.h"

#include <vcsbaseplugin.h>
#include <vcsbaseoutputwindow.h>

#include <QtCore/QDebug>
#include <QtCore/QQueue>
#include <QtCore/QDir>
#include <utils/synchronousprocess.h>
#include <utils/qtcassert.h>

enum { debug = 0 };
namespace VCSBase {

AbstractCheckoutJob::AbstractCheckoutJob(QObject *parent) :
    QObject(parent)
{
}

struct ProcessCheckoutJobStep
{
    ProcessCheckoutJobStep() {}
    explicit ProcessCheckoutJobStep(const QString &bin,
                                    const QStringList &args,
                                    const QString &workingDir,
                                    QProcessEnvironment env) :
             binary(bin), arguments(args), workingDirectory(workingDir), environment(env) {}

    QString binary;
    QStringList arguments;
    QString workingDirectory;
    QProcessEnvironment environment;
};

struct ProcessCheckoutJobPrivate {
    ProcessCheckoutJobPrivate();

    QSharedPointer<QProcess> process;
    QQueue<ProcessCheckoutJobStep> stepQueue;
    QString binary;
};

// Use a terminal-less process to suppress SSH prompts.
static inline QSharedPointer<QProcess> createProcess()
{
    unsigned flags = 0;
    if (VCSBasePlugin::isSshPromptConfigured())
        flags = Utils::SynchronousProcess::UnixTerminalDisabled;
    return Utils::SynchronousProcess::createProcess(flags);
}

ProcessCheckoutJobPrivate::ProcessCheckoutJobPrivate() :
    process(createProcess())
{    
}

ProcessCheckoutJob::ProcessCheckoutJob(QObject *parent) :
    AbstractCheckoutJob(parent),
    d(new ProcessCheckoutJobPrivate)
{
    connect(d->process.data(), SIGNAL(error(QProcess::ProcessError)), this, SLOT(slotError(QProcess::ProcessError)));
    connect(d->process.data(), SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotFinished(int,QProcess::ExitStatus)));
    connect(d->process.data(), SIGNAL(readyReadStandardOutput()), this, SLOT(slotOutput()));
    d->process->setProcessChannelMode(QProcess::MergedChannels);
    d->process->closeWriteChannel();
}

ProcessCheckoutJob::~ProcessCheckoutJob()
{
    delete d;
}

void ProcessCheckoutJob::addStep(const QString &binary,
                                const QStringList &args,
                                const QString &workingDirectory,
                                const QProcessEnvironment &env)
{
    if (debug)
        qDebug() << "ProcessCheckoutJob::addStep" << binary << args << workingDirectory;
    d->stepQueue.enqueue(ProcessCheckoutJobStep(binary, args, workingDirectory, env));
}

void ProcessCheckoutJob::slotOutput()
{
    const QByteArray data = d->process->readAllStandardOutput();
    const QString s = QString::fromLocal8Bit(data, data.endsWith('\n') ? data.size() - 1: data.size());
    if (debug)
        qDebug() << s;
    emit output(s);
}

void ProcessCheckoutJob::slotError(QProcess::ProcessError error)
{
    switch (error) {
    case QProcess::FailedToStart:
        emit failed(tr("Unable to start %1: %2").
                    arg(QDir::toNativeSeparators(d->binary), d->process->errorString()));
        break;
    default:
        emit failed(d->process->errorString());
        break;
    }
}

void ProcessCheckoutJob::slotFinished (int exitCode, QProcess::ExitStatus exitStatus)
{
    if (debug)
        qDebug() << "finished" << exitCode << exitStatus;

    switch (exitStatus) {
    case QProcess::NormalExit:
        emit output(tr("The process terminated with exit code %1.").arg(exitCode));
        if (exitCode == 0) {
            slotNext();
        } else {
            emit failed(tr("The process returned exit code %1.").arg(exitCode));
        }
        break;
    case QProcess::CrashExit:
        emit failed(tr("The process terminated in an abnormal way."));
        break;
    }
}

void ProcessCheckoutJob::start()
{
    QTC_ASSERT(!d->stepQueue.empty(), return)
    slotNext();
}

void ProcessCheckoutJob::slotNext()
{
    if (d->stepQueue.isEmpty()) {
        emit succeeded();
        return;
    }
    // Launch next
    const ProcessCheckoutJobStep step = d->stepQueue.dequeue();
    d->process->setWorkingDirectory(step.workingDirectory);

    // Set up SSH correctly.
    QProcessEnvironment processEnv = step.environment;
    VCSBasePlugin::setProcessEnvironment(&processEnv, false);
    d->process->setProcessEnvironment(processEnv);

    d->binary = step.binary;
    emit output(VCSBaseOutputWindow::msgExecutionLogEntry(step.workingDirectory, d->binary, step.arguments));
    d->process->start(d->binary, step.arguments);
}

void ProcessCheckoutJob::cancel()
{
    if (debug)
        qDebug() << "ProcessCheckoutJob::start";

    emit output(tr("Stopping..."));
    Utils::SynchronousProcess::stopProcess(*d->process);
}

} // namespace VCSBase
