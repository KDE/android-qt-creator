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

#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>

#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>

#include <QtCore/QFileInfo>
#include <QtCore/QThread>

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
    m_processPID = -1;
    m_exitStatus = 0;
    connect(&m_checkPIDTimer, SIGNAL(timeout()), SLOT(checkPID()));
    connect(&m_adbLogcatProcess, SIGNAL(readyReadStandardOutput()), SLOT(logcatReadStandardOutput()));
    connect(&m_adbLogcatProcess, SIGNAL(readyReadStandardError()) , SLOT(logcatReadStandardError()));
}

AndroidRunner::~AndroidRunner() {}

void AndroidRunner::checkPID()
{
    QProcess psProc;
    psProc.start(AndroidConfigurations::instance().adbToolPath()+QLatin1String(" shell ps"));
    if (!psProc.waitForFinished(-1))
        return;
    QList<QByteArray> procs= psProc.readAll().split('\n');
    foreach(QByteArray proc, procs)
    {
        if (proc.trimmed().endsWith(m_packageName.toAscii()))
        {
            QRegExp rx("(\\d+)");
            if (rx.indexIn(proc, proc.indexOf(' ')) > 0)
            {
                m_processPID=rx.cap(1).toLongLong();
                return;
            }
        }
    }
    if (-1 != m_processPID)
    {
        m_processPID = -1;
        emit remoteProcessFinished(tr("\n\n'%1' died").arg(m_packageName));
    }
}

void AndroidRunner::start()
{
    m_exitStatus = 0;
    stop(); // kill any process with this name
    if (m_debugingMode)
    ;// start debuging socket listener
    QProcess adbStarProc;
    adbStarProc.start(AndroidConfigurations::instance().adbToolPath()+QLatin1String(" shell am start -n ")+m_intentName);
    if (!adbStarProc.waitForFinished(-1))
    {
        emit remoteProcessFinished(tr("Unable to start '%1'").arg(m_packageName));
        return;
    }
    int retires=50;
    while(-1 == m_processPID && --retires)
    {
        checkPID();
    }
    if (-1== m_processPID)
    {
        m_exitStatus = -1;
        emit remoteProcessFinished(tr("Can't find %1 precess").arg(m_packageName));
        return;
    }
    m_exitStatus = 0;
    m_checkPIDTimer.start(5000); // check the application every 5 seconds
    m_adbLogcatProcess.start(AndroidConfigurations::instance().adbToolPath()+QLatin1String(" logcat"));
    if (!m_debugingMode)
        emit remoteProcessStarted();
    else
        emit remoteProcessStarted();
}

void AndroidRunner::stop()
{
    m_adbLogcatProcess.terminate();
    checkPID();
    m_checkPIDTimer.stop();
    if (-1 == m_processPID)
        return;
    QProcess adbStopProc;
    adbStopProc.start(AndroidConfigurations::instance().adbToolPath()+QString(" shell kill -9 %1").arg(m_processPID));
    m_processPID = -1;
    adbStopProc.waitForFinished(-1);
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

} // namespace Internal
} // namespace Qt4ProjectManager

