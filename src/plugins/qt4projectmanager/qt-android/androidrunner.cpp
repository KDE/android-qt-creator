/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidrunner.h"

#include "androiddeploystep.h"
#include "androidconfigurations.h"
#include "androidglobal.h"
#include "androidrunconfiguration.h"
#include "qt4androidtarget.h"


#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>

#include <QtCore/QFileInfo>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>

#if defined(_WIN32)
#include <iostream>
#include <windows.h>
#define sleep(_n) Sleep(1000 * (_n))
#endif
using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

AndroidRunner::AndroidRunner(QObject *parent,
    AndroidRunConfiguration *runConfig, bool debugging)
    : QObject(parent),
      m_intentName(runConfig->androidTarget()->intentName())
{
    m_runConfig = runConfig;
    m_debugingMode = debugging;
    m_packageName=m_intentName.left(m_intentName.indexOf('/'));
    m_deviceSerialNumber = m_runConfig->deployStep()->deviceSerialNumber();
    m_processPID = -1;
    m_gdbserverPID = -1;
    m_exitStatus = 0;
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
    qApp->processEvents();
    QProcess psProc;
    psProc.start(AndroidConfigurations::instance().adbToolPath(),QStringList()<<"-s"<<m_deviceSerialNumber<<"shell"<<"ps");
    if (!psProc.waitForFinished(-1))
    {
        psProc.terminate();
        return;
    }
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
    checkPID(); //updates m_processPID and m_gdbserverPID
    for (int tries=0; tries < 10 && (m_processPID != -1 || m_gdbserverPID != -1); ++tries) {

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
    m_processPID = -1;
    m_exitStatus = 0;
    killPID(); // kill any process with this name
    QString extraParams;
    QProcess adbStarProc;
    if (m_debugingMode)
    {
        adbStarProc.start(AndroidConfigurations::instance().adbToolPath(),QStringList()<<"-s"<<m_deviceSerialNumber<<"forward"<<QString("tcp%1").arg(m_runConfig->remoteChannel())<<QString("localfilesystem:/data/data/%1/debug-socket").arg(m_packageName));
        if (!adbStarProc.waitForStarted()) {
            emit remoteProcessFinished(tr("Failed to forward debugging ports. Reason: $1").arg(adbStarProc.errorString()));
            return;
        }
        if (!adbStarProc.waitForFinished(-1))
        {
            emit remoteProcessFinished(tr("Failed to forward debugging ports"));
            return;
        }
        extraParams="-e native_debug true -e gdbserver_socket +debug-socket";
    }

    if (m_runConfig->deployStep()->useLocalQtLibs())
    {
        extraParams+=" -e use_local_qt_libs true";
        extraParams+=" -e load_local_libs "+m_runConfig->androidTarget()->loadLocalLibs(m_runConfig->deployStep()->deviceAPILevel());
    }
    adbStarProc.start(AndroidConfigurations::instance().adbToolPath(),QStringList()<<"-s"<<m_deviceSerialNumber<<"shell"<<"am"<<"start"<<"-n"<<m_intentName<<extraParams.trimmed().split(" "));
    if (!adbStarProc.waitForStarted()) {
        emit remoteProcessFinished(tr("Failed to forward debugging ports. Reason: $1").arg(adbStarProc.errorString()));
        return;
    }
    if (!adbStarProc.waitForFinished(-1))
    {
        adbStarProc.terminate();
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
        emit remoteProcessFinished(tr("Can't find %1 process").arg(m_packageName));
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
    m_checkPIDTimer.start(1000); // check if the application is alive every 1 seconds
    m_adbLogcatProcess.start(AndroidConfigurations::instance().adbToolPath(),QStringList()<<"-s"<<m_deviceSerialNumber<<"logcat");
#ifdef __GNUC__
#warning FIXME Android m_gdbServerPort(5039)
#endif
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

void AndroidRunner::adbKill(qint64 pid, const QString &device, int timeout, const QString &runAsPackageName)
{
    QProcess process;
    QStringList arguments;

    arguments << "-s" << device;
    arguments << "shell";
    if (runAsPackageName.size())
        arguments << "run-as" << runAsPackageName;
    arguments << "kill" << "-9";
    arguments << QString::number(pid);

    process.start(AndroidConfigurations::instance().adbToolPath(), arguments);
    if (!process.waitForFinished(timeout))
        process.terminate();

//    qDebug() << "Tried running: adb" << arguments.join(" ") << "\nWhich gave the output: " << process.readAllStandardOutput() << "\nAnd gave the exitcode of: " << process.exitCode();
}

QString AndroidRunner::displayName() const
{
    return m_packageName;
}

} // namespace Internal
} // namespace Qt4ProjectManager

