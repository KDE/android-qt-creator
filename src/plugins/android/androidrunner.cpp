/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidrunner.h"

#include "androiddeploystep.h"
#include "androidconfigurations.h"
#include "androidglobal.h"
#include "androidrunconfiguration.h"
#include "androidmanager.h"

#include <projectexplorer/target.h>

#include <QTime>
#include <QtConcurrentRun>

namespace Android {
namespace Internal {

AndroidRunner::AndroidRunner(QObject *parent,
    AndroidRunConfiguration *runConfig, bool debugging)
    : QThread(parent)
{
    m_remoteChannel = runConfig->remoteChannel();
    ProjectExplorer::Target *target = runConfig->target();
    AndroidDeployStep *ds = runConfig->deployStep();
    if ((m_useLocalQtLibs = ds->useLocalQtLibs())) {
        m_localLibs = AndroidManager::loadLocalLibs(target, ds->deviceAPILevel());
        m_localJars = AndroidManager::loadLocalJars(target, ds->deviceAPILevel());
    }
    m_intentName = AndroidManager::intentName(target);
    m_debugingMode = debugging;
    m_packageName = m_intentName.left(m_intentName.indexOf(QLatin1Char('/')));
    m_deviceSerialNumber = ds->deviceSerialNumber();
    m_processPID = -1;
    m_gdbserverPID = -1;
    connect(&m_checkPIDTimer, SIGNAL(timeout()), SLOT(checkPID()));
    connect(&m_adbLogcatProcess, SIGNAL(readyReadStandardOutput()), SLOT(logcatReadStandardOutput()));
    connect(&m_adbLogcatProcess, SIGNAL(readyReadStandardError()) , SLOT(logcatReadStandardError()));
}

AndroidRunner::~AndroidRunner()
{
    stop();
}

void AndroidRunner::checkPID()
{
    QProcess psProc;
    psProc.start(AndroidConfigurations::instance().adbToolPath().toString(),
                 QStringList() << QLatin1String("-s") << m_deviceSerialNumber
                 << QLatin1String("shell") << QLatin1String("ps"));
    if (!psProc.waitForFinished(-1)) {
        psProc.terminate();
        return;
    }
    qint64 pid = -1;
    QList<QByteArray> procs = psProc.readAll().split('\n');
    foreach (const QByteArray &proc, procs) {
        if (proc.trimmed().endsWith(m_packageName.toAscii())) {
            QRegExp rx(QLatin1String("(\\d+)"));
            if (rx.indexIn(QLatin1String(proc), proc.indexOf(' ')) > 0) {
                pid = rx.cap(1).toLongLong();
                break;
            }
        }
    }

    if (-1 != m_processPID && pid == -1) {
        m_processPID = -1;
        emit remoteProcessFinished(tr("\n\n'%1' died").arg(m_packageName));
        return;
    }
    m_processPID = pid;
    if (!m_debugingMode)
        return;

    m_gdbserverPID = -1;
    foreach (const QByteArray &proc, procs) {
        if (proc.trimmed().endsWith("gdbserver")) {
            QRegExp rx(QLatin1String("(\\d+)"));
            if (rx.indexIn(QLatin1String(proc), proc.indexOf(' ')) > 0) {
                m_gdbserverPID = rx.cap(1).toLongLong();
                break;
            }
        }
    }
}

void AndroidRunner::killPID()
{
    checkPID(); //updates m_processPID and m_gdbserverPID
    for (int tries = 0; tries < 10 && (m_processPID != -1 || m_gdbserverPID != -1); ++tries) {
        if (m_processPID != -1) {
            adbKill(m_processPID, m_deviceSerialNumber, 2000);
            adbKill(m_processPID, m_deviceSerialNumber, 2000, m_packageName);
        }

        if (m_gdbserverPID != -1) {
            adbKill(m_gdbserverPID, m_deviceSerialNumber, 2000);
            adbKill(m_gdbserverPID, m_deviceSerialNumber, 2000, m_packageName);
        }
        checkPID();
    }
}

void AndroidRunner::start()
{
    QtConcurrent::run(this,&AndroidRunner::asyncStart);
}

void AndroidRunner::asyncStart()
{
    QMutexLocker locker(&m_mutex);
    m_processPID = -1;
    killPID(); // kill any process with this name
    QString extraParams;
    QProcess adbStarProc;
    if (m_debugingMode) {
        QStringList arguments;
        arguments << QLatin1String("-s") << m_deviceSerialNumber
                  << QLatin1String("forward") << QString::fromLatin1("tcp%1").arg(m_remoteChannel)
                  << QString::fromLatin1("localfilesystem:/data/data/%1/debug-socket").arg(m_packageName);
        adbStarProc.start(AndroidConfigurations::instance().adbToolPath().toString(), arguments);
        if (!adbStarProc.waitForStarted()) {
            emit remoteProcessFinished(tr("Failed to forward debugging ports. Reason: $1").arg(adbStarProc.errorString()));
            return;
        }
        if (!adbStarProc.waitForFinished(-1)) {
            emit remoteProcessFinished(tr("Failed to forward debugging ports"));
            return;
        }
        extraParams = QLatin1String("-e native_debug true -e gdbserver_socket +debug-socket");
    }

    if (m_useLocalQtLibs) {
        extraParams += QLatin1String(" -e use_local_qt_libs true");
        extraParams += QLatin1String(" -e libs_prefix /data/local/qt/");
        extraParams += QLatin1String(" -e load_local_libs ") + m_localLibs;
        extraParams += QLatin1String(" -e load_local_jars ") + m_localJars;
    }

    extraParams = extraParams.trimmed();
    QStringList arguments;
    arguments << QLatin1String("-s") << m_deviceSerialNumber
              << QLatin1String("shell") << QLatin1String("am")
              << QLatin1String("start") << QLatin1String("-n") << m_intentName;

    if (extraParams.length())
        arguments << extraParams.split(QLatin1Char(' '));

    adbStarProc.start(AndroidConfigurations::instance().adbToolPath().toString(), arguments);
    if (!adbStarProc.waitForStarted()) {
        emit remoteProcessFinished(tr("Failed to start the activity. Reason: $1").arg(adbStarProc.errorString()));
        return;
    }
    if (!adbStarProc.waitForFinished(-1)) {
        adbStarProc.terminate();
        emit remoteProcessFinished(tr("Unable to start '%1'").arg(m_packageName));
        return;
    }
    QTime startTime = QTime::currentTime();
    while (m_processPID == -1 && startTime.secsTo(QTime::currentTime()) < 10) { // wait up to 10 seconds for application to start
        checkPID();
    }
    if (m_processPID == -1) {
        emit remoteProcessFinished(tr("Cannot find %1 process").arg(m_packageName));
        return;
    }

    if (m_debugingMode) {
        startTime = QTime::currentTime();
        while (m_gdbserverPID == -1 && startTime.secsTo(QTime::currentTime()) < 25) { // wait up to 25 seconds to connect
            checkPID();
        }
        msleep(200); // give gdbserver more time to start
    }

    QMetaObject::invokeMethod(this, "startLogcat", Qt::QueuedConnection);
}

void AndroidRunner::startLogcat()
{
    m_checkPIDTimer.start(1000); // check if the application is alive every 1 seconds
    m_adbLogcatProcess.start(AndroidConfigurations::instance().adbToolPath().toString(),
                             QStringList() << QLatin1String("-s") << m_deviceSerialNumber
                             << QLatin1String("logcat"));
    emit remoteProcessStarted(5039);
}

void AndroidRunner::stop()
{
    QMutexLocker locker(&m_mutex);
    m_adbLogcatProcess.terminate();
    m_adbLogcatProcess.waitForFinished(-1);
    m_checkPIDTimer.stop();
    if (m_processPID == -1)
        return; // don't emit another signal
    QtConcurrent::run(this, &AndroidRunner::asyncStop);
}
void AndroidRunner::asyncStop()
{
    killPID();
    emit remoteProcessFinished(tr("\n\n'%1' killed").arg(m_packageName));
}

void AndroidRunner::logcatReadStandardError()
{
    emit remoteErrorOutput(m_adbLogcatProcess.readAllStandardError());
}

void AndroidRunner::logcatReadStandardOutput()
{
    m_logcat += m_adbLogcatProcess.readAllStandardOutput();
    bool keepLastLine = m_logcat.endsWith('\n');
    QByteArray line;
    QByteArray pid(QString::fromLatin1("%1):").arg(m_processPID).toAscii());
    foreach (line, m_logcat.split('\n')) {
        if (!line.contains(pid))
            continue;
        if (line.startsWith("E/"))
            emit remoteErrorOutput(line);
        else
            emit remoteOutput(line);

    }
    if (keepLastLine)
        m_logcat = line;
}

void AndroidRunner::adbKill(qint64 pid, const QString &device, int timeout, const QString &runAsPackageName)
{
    QProcess process;
    QStringList arguments;

    arguments << QLatin1String("-s") << device;
    arguments << QLatin1String("shell");
    if (runAsPackageName.size())
        arguments << QLatin1String("run-as") << runAsPackageName;
    arguments << QLatin1String("kill") << QLatin1String("-9");
    arguments << QString::number(pid);

    process.start(AndroidConfigurations::instance().adbToolPath().toString(), arguments);
    if (!process.waitForFinished(timeout))
        process.terminate();
}

QString AndroidRunner::displayName() const
{
    return m_packageName;
}

} // namespace Internal
} // namespace Qt4ProjectManager
