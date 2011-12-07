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

#include "perforcechecker.h"

#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>

#include <QtCore/QRegExp>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

#include <QtGui/QApplication>
#include <QtGui/QCursor>

namespace Perforce {
namespace Internal {

PerforceChecker::PerforceChecker(QObject *parent) :
    QObject(parent),
    m_timeOutMS(-1),
    m_timedOut(false),
    m_useOverideCursor(false),
    m_isOverrideCursor(false)
{
    connect(&m_process, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(slotError(QProcess::ProcessError)));
    connect(&m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
                this, SLOT(slotFinished(int,QProcess::ExitStatus)));
}

PerforceChecker::~PerforceChecker()
{
    m_process.kill();
    m_process.waitForFinished();
    resetOverrideCursor();
}

bool PerforceChecker::isRunning() const
{
    return m_process.state() == QProcess::Running;
}

void PerforceChecker::resetOverrideCursor()
{
    if (m_isOverrideCursor) {
        QApplication::restoreOverrideCursor();
        m_isOverrideCursor = false;
    }
}

void PerforceChecker::start(const QString &binary,
                            const QStringList &basicArgs,
                            int timeoutMS)
{
    if (isRunning()) {
        emitFailed(QLatin1String("Internal error: process still running"));
        return;
    }
    if (binary.isEmpty()) {
        emitFailed(tr("No executable specified"));
        return;
    }
    m_binary = binary;
    QStringList args = basicArgs;
    args << QLatin1String("client") << QLatin1String("-o");
    m_process.start(m_binary, args);
    m_process.closeWriteChannel();
    // Timeout handling
    m_timeOutMS = timeoutMS;
    m_timedOut = false;
    if (timeoutMS > 0)
        QTimer::singleShot(m_timeOutMS, this, SLOT(slotTimeOut()));
    // Cursor
    if (m_useOverideCursor) {
        m_isOverrideCursor = true;
        QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    }
}

void PerforceChecker::slotTimeOut()
{
    if (!isRunning())
        return;
    m_timedOut = true;
    Utils::SynchronousProcess::stopProcess(m_process);
    emitFailed(tr("\"%1\" timed out after %2ms.").
               arg(m_binary).arg(m_timeOutMS));
}

void PerforceChecker::slotError(QProcess::ProcessError error)
{
    if (m_timedOut)
        return;
    switch (error) {
    case QProcess::FailedToStart:
        emitFailed(tr("Unable to launch \"%1\": %2").
                   arg(QDir::toNativeSeparators(m_binary), m_process.errorString()));
        break;
    case QProcess::Crashed: // Handled elsewhere
    case QProcess::Timedout:
        break;
    case QProcess::ReadError:
    case QProcess::WriteError:
    case QProcess::UnknownError:
        Utils::SynchronousProcess::stopProcess(m_process);
        break;
    }
}

void PerforceChecker::slotFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_timedOut)
        return;
    switch (exitStatus) {
    case QProcess::CrashExit:
        emitFailed(tr("\"%1\" crashed.").arg(QDir::toNativeSeparators(m_binary)));
        break;
    case QProcess::NormalExit:
        if (exitCode) {
            const QString stdErr = QString::fromLocal8Bit(m_process.readAllStandardError());
            emitFailed(tr("\"%1\" terminated with exit code %2: %3").
                       arg(QDir::toNativeSeparators(m_binary)).arg(exitCode).arg(stdErr));
        } else {
            parseOutput(QString::fromLocal8Bit(m_process.readAllStandardOutput()));
        }
        break;
    }
}

// Parse p4 client output for the top level
static inline QString clientRootFromOutput(const QString &in)
{
    QRegExp regExp(QLatin1String("(\\n|\\r\\n|\\r)Root:\\s*(.*)(\\n|\\r\\n|\\r)"));
    QTC_ASSERT(regExp.isValid(), return QString());
    regExp.setMinimal(true);
    // Normalize slashes and capitalization of Windows drive letters for caching.
    if (regExp.indexIn(in) != -1)
        return QFileInfo(regExp.cap(2).trimmed()).absoluteFilePath();
    return QString();
}

void PerforceChecker::parseOutput(const QString &response)
{
    if (!response.contains(QLatin1String("View:")) && !response.contains(QLatin1String("//depot/"))) {
        emitFailed(tr("The client does not seem to contain any mapped files."));
        return;
    }
    const QString repositoryRoot = clientRootFromOutput(response);
    if (repositoryRoot.isEmpty()) {
        //: Unable to determine root of the p4 client installation
        emitFailed(tr("Unable to determine the client root."));
        return;
    }
    // Check existence. No precise check here, might be a symlink
    const QFileInfo fi(repositoryRoot);
    if (fi.exists()) {
        emitSucceeded(repositoryRoot);
    } else {
        emitFailed(tr("The repository \"%1\" does not exist.").
                   arg(QDir::toNativeSeparators(repositoryRoot)));
    }
}

void PerforceChecker::emitFailed(const QString &m)
{
    resetOverrideCursor();
    emit failed(m);
}

void PerforceChecker::emitSucceeded(const QString &m)
{
    resetOverrideCursor();
    emit succeeded(m);
}

bool PerforceChecker::useOverideCursor() const
{
    return m_useOverideCursor;
}

void PerforceChecker::setUseOverideCursor(bool v)
{
    m_useOverideCursor = v;
}

}
}

