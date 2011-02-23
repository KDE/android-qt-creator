/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef MAEMOSSHRUNNER_H
#define MAEMOSSHRUNNER_H

#include "androidconfigurations.h"

#include <utils/environment.h>

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QProcess>


namespace Qt4ProjectManager {
namespace Internal {
class AndroidRunConfiguration;

class AndroidRunner : public QObject
{
    Q_OBJECT
public:
    AndroidRunner(QObject *parent, AndroidRunConfiguration *runConfig,
        bool debugging);
    ~AndroidRunner();

    QString displayName() const;

public slots:
    void start();
    void stop();


signals:
    void remoteProcessStarted(int gdbServerPort=-1, int qmlPort=-1);
    void remoteProcessFinished(const QString &errString="");

    void remoteOutput(const QByteArray &output);
    void remoteErrorOutput(const QByteArray &output);

private slots:
    void killPID();
    void checkPID();
    void logcatReadStandardError();
    void logcatReadStandardOutput();

private:
    int m_exitStatus;
    bool    m_debugingMode;
    QProcess m_adbLogcatProcess;
    QByteArray m_logcat;
    QString m_intentName;
    QString m_packageName;
    QString m_deviceSerialNumber;
    qint64 m_processPID;
    qint64 m_gdbserverPID;
    QTimer m_checkPIDTimer;
    AndroidRunConfiguration *m_runConfig;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOSSHRUNNER_H
