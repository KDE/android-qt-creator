/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "androidrunner.h"

#include "androiddeploystep.h"
#include "androidconfigurations.h"
#include "androidglobal.h"
#include "androidrunconfiguration.h"
#include "androidtemplatesmanager.h"


#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>

#include <QtCore/QFileInfo>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

AndroidRunner::AndroidRunner(QObject *parent,
    AndroidRunConfiguration *runConfig, bool debugging)
    : QObject(parent),
      m_intentName(AndroidTemplatesManager::instance()->intentName(runConfig->qt4Target()->qt4Project()))
{
    m_debugingMode = debugging;
    m_packageName=m_intentName.left(m_intentName.indexOf('/'));
    m_deviceSerialNumber = runConfig->deployStep()->deviceSerialNumber();
    m_processPID = -1;
    m_gdbserverPID = -1;
    m_exitStatus = 0;
    connect(&m_checkPIDTimer, SIGNAL(timeout()), SLOT(checkPID()));
    connect(&m_adbLogcatProcess, SIGNAL(readyReadStandardOutput()), SLOT(logcatReadStandardOutput()));
    connect(&m_adbLogcatProcess, SIGNAL(readyReadStandardError()) , SLOT(logcatReadStandardError()));
}

AndroidRunner::~AndroidRunner() {}

void AndroidRunner::checkPID()
{
    qApp->processEvents();
    QProcess psProc;
    psProc.start(AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)+QLatin1String(" shell ps"));
    if (!psProc.waitForFinished(-1))
        return;
    qint64 pid=-1;
    QList<QByteArray> procs= psProc.readAll().split('\n');
    foreach(QByteArray proc, procs)
    {
        if (proc.trimmed().endsWith(m_packageName.toAscii()))
        {
            QRegExp rx("(\\d+)");
            if (rx.indexIn(proc, proc.indexOf(' ')) > 0)
            {
                pid=rx.cap(1).toLongLong();
                break;
            }
        }
    }

    if (-1 != m_processPID && pid==-1)
    {
        m_processPID = -1;
        emit remoteProcessFinished(tr("\n\n'%1' died").arg(m_packageName));
        return;
    }
    m_processPID = pid;
    if (!m_debugingMode)
        return;

    m_gdbserverPID = -1;
    foreach(QByteArray proc, procs)
    {
        if (proc.trimmed().endsWith("gdbserver"))
        {
            QRegExp rx("(\\d+)");
            if (rx.indexIn(proc, proc.indexOf(' ')) > 0)
            {
                m_gdbserverPID=rx.cap(1).toLongLong();
                break;
            }
        }
    }
}

void AndroidRunner::killPID()
{
    QProcess m_killProcess;
    do
    {
        checkPID();
        if (-1 != m_processPID)
        {
            m_killProcess.start(AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)+QString(" shell kill -9 %1").arg(m_processPID));
            m_killProcess.waitForFinished(-1);
        }
        if (-1 != m_gdbserverPID)
        {
            m_killProcess.start(AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)+QString(" shell kill -9 %1").arg(m_gdbserverPID));
            m_killProcess.waitForFinished(-1);
        }
    }while(-1 != m_processPID || m_gdbserverPID != -1);
}

void AndroidRunner::start()
{
    m_processPID = -1;
    m_exitStatus = 0;
    killPID(); // kill any process with this name
    QString debugString;
    QProcess adbStarProc;
    if (m_debugingMode)
    {
        qDebug()<<QString(" forward tcp:%1 localfilesystem:/data/data/%2/debug-socket").arg(5039).arg(m_packageName);
        adbStarProc.start(AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)+QString(" forward tcp:%1 localfilesystem:/data/data/%2/debug-socket").arg(5039).arg(m_packageName));
        if (!adbStarProc.waitForFinished(-1))
        {
            emit remoteProcessFinished(tr("Failed to forward debugging posts"));
            return;
        }
        debugString=QString("-e native_debug true -e gdbserver_socket \"+debug-socket\"");
    }

    adbStarProc.start(AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)+QString(" shell am start -n %1 %2").arg(m_intentName).arg(debugString));
    if (!adbStarProc.waitForFinished(-1))
    {
        emit remoteProcessFinished(tr("Unable to start '%1'").arg(m_packageName));
        return;
    }
    QTime startTime=QTime::currentTime();
    while(-1 == m_processPID && startTime.secsTo(QTime::currentTime())<5); // wait up to 5 seconds for application to start
    {
        checkPID();
    }
    if (-1== m_processPID)
    {
        m_exitStatus = -1;
        emit remoteProcessFinished(tr("Can't find %1 precess").arg(m_packageName));
        return;
    }

    if (m_debugingMode)
    {
        startTime=QTime::currentTime();
        while(m_gdbserverPID==-1 && startTime.secsTo(QTime::currentTime())<25); // wait up to 25 seconds to connect
        {
            checkPID();
        }
        sleep(1); // give gdbserver more time to start
    }

    m_exitStatus = 0;
    m_checkPIDTimer.start(5000); // check if the application is alive every 5 seconds
    m_adbLogcatProcess.start(AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)+QLatin1String(" logcat"));
    emit remoteProcessStarted(5039);
}

void AndroidRunner::stop()
{
    m_adbLogcatProcess.terminate();
    m_checkPIDTimer.stop();
    if (-1==m_processPID)
        return; // don't emit another signal
    killPID();
    emit remoteProcessFinished(tr("\n\n'%1' killed").arg(m_packageName));
}

void AndroidRunner::logcatReadStandardError()
{
    emit remoteErrorOutput(m_adbLogcatProcess.readAllStandardError());
}

void AndroidRunner::logcatReadStandardOutput()
{
    m_logcat+=m_adbLogcatProcess.readAllStandardOutput();
    bool keepLastLine=m_logcat.endsWith('\n');
    QByteArray line;
    QByteArray pid(QString("%1):").arg(m_processPID).toAscii());
    foreach(line, m_logcat.split('\n'))
    {
        if (!line.contains(pid))
            continue;
        if (line.startsWith("E/"))
            emit remoteErrorOutput(line);
        else
            emit remoteOutput(line);

    }
    if (keepLastLine)
        m_logcat=line;
}

QString AndroidRunner::displayName() const
{
    return m_packageName;
}

} // namespace Internal
} // namespace Qt4ProjectManager

