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

#include "localplaingdbadapter.h"

#include "gdbengine.h"
#include "debuggerstartparameters.h"
#include "procinterrupt.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"

#include <projectexplorer/abi.h>

#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtGui/QMessageBox>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// PlainGdbAdapter
//
///////////////////////////////////////////////////////////////////////

LocalPlainGdbAdapter::LocalPlainGdbAdapter(GdbEngine *engine)
    : AbstractPlainGdbAdapter(engine)
{
    // Output
    connect(&m_outputCollector, SIGNAL(byteDelivery(QByteArray)),
        engine, SLOT(readDebugeeOutput(QByteArray)));
}

AbstractGdbAdapter::DumperHandling LocalPlainGdbAdapter::dumperHandling() const
{
    // LD_PRELOAD fails for System-Qt on Mac.
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    return DumperLoadedByGdb;
#else
    return DumperLoadedByGdbPreload;
#endif
}

void LocalPlainGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));

    if (!prepareCommand())
        return;

    QStringList gdbArgs;

    if (!m_outputCollector.listen()) {
        m_engine->handleAdapterStartFailed(tr("Cannot set up communication with child process: %1")
                .arg(m_outputCollector.errorString()), QString());
        return;
    }
    gdbArgs.append(_("--tty=") + m_outputCollector.serverName());

    if (!startParameters().workingDirectory.isEmpty())
        m_gdbProc.setWorkingDirectory(startParameters().workingDirectory);
    if (startParameters().environment.size())
        m_gdbProc.setEnvironment(startParameters().environment.toStringList());

    if (!m_engine->startGdb(gdbArgs)) {
        m_outputCollector.shutdown();
        return;
    }

    checkForReleaseBuild();
    m_engine->handleAdapterStarted();
}

void LocalPlainGdbAdapter::setupInferior()
{
    AbstractPlainGdbAdapter::setupInferior();
}

void LocalPlainGdbAdapter::runEngine()
{
    AbstractPlainGdbAdapter::runEngine();
}

void LocalPlainGdbAdapter::shutdownInferior()
{
    m_engine->defaultInferiorShutdown("kill");
}

void LocalPlainGdbAdapter::shutdownAdapter()
{
    showMessage(_("PLAIN ADAPTER SHUTDOWN %1").arg(state()));
    m_outputCollector.shutdown();
    m_engine->notifyAdapterShutdownOk();
}

void LocalPlainGdbAdapter::checkForReleaseBuild()
{
#ifndef Q_OS_MAC
    // There is usually no objdump on Mac, and if there is,
    // there are no .debug_info sections.
    QString objDump = _("objdump");
    // Windows: Locate objdump in the debuggee's (MinGW) environment
    if (ProjectExplorer::Abi::hostAbi().os() == ProjectExplorer::Abi::WindowsOS
        && startParameters().environment.size()) {
        objDump = startParameters().environment.searchInPath(objDump);
    } else {
        objDump = Utils::Environment::systemEnvironment().searchInPath(objDump);
    }
    if (objDump.isEmpty()) {
        showMessage(_("Could not locate objdump command for release build check"), LogWarning);
        return;
    }
    // Quick check for a "release" build
    QProcess proc;
    QStringList args;
    args.append(_("-h"));
    args.append(_("-j"));
    args.append(_(".debug_info"));
    args.append(startParameters().executable);
    proc.start(objDump, args);
    proc.closeWriteChannel();
    if (!proc.waitForStarted()) {
        showMessage(_("OBJDUMP PROCESS COULD NOT BE STARTED. "
            "RELEASE BUILD CHECK WILL FAIL"));
        return;
    }
    proc.waitForFinished();
    QByteArray ba = proc.readAllStandardOutput();
    // This should yield something like
    // "debuggertest:     file format elf32-i386\n\n"
    // "Sections:\nIdx Name          Size      VMA       LMA       File off  Algn\n"
    // "30 .debug_info   00087d36  00000000  00000000  0006bbd5  2**0\n"
    // " CONTENTS, READONLY, DEBUGGING"
    if (ba.contains("Sections:") && !ba.contains(".debug_info")) {
        showMessageBox(QMessageBox::Information, tr("Warning"),
           tr("This does not seem to be a \"Debug\" build.\n"
              "Setting breakpoints by file name and line number may fail."));
    }
#endif
}

void LocalPlainGdbAdapter::interruptInferior()
{
    const qint64 attachedPID = m_engine->inferiorPid();
    if (attachedPID <= 0) {
        showMessage(_("TRYING TO INTERRUPT INFERIOR BEFORE PID WAS OBTAINED"));
        return;
    }

    if (interruptProcess(attachedPID)) {
        showMessage(_("INTERRUPTED %1").arg(attachedPID));
    } else {
        showMessage(_("CANNOT INTERRUPT %1").arg(attachedPID));
        m_engine->notifyInferiorStopFailed();
    }
}

QByteArray LocalPlainGdbAdapter::execFilePath() const
{
    return QFileInfo(startParameters().executable)
            .absoluteFilePath().toLocal8Bit();
}

QByteArray LocalPlainGdbAdapter::toLocalEncoding(const QString &s) const
{
    return s.toLocal8Bit();
}

QString LocalPlainGdbAdapter::fromLocalEncoding(const QByteArray &b) const
{
    return QString::fromLocal8Bit(b);
}

} // namespace Internal
} // namespace Debugger
