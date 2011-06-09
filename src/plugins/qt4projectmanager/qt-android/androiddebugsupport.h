/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDDEBUGSUPPORT_H
#define ANDROIDDEBUGSUPPORT_H

#include "androidrunconfiguration.h"

#include <utils/environment.h>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>


namespace Debugger {
class DebuggerRunControl;
}

namespace ProjectExplorer { class RunControl; }

namespace Qt4ProjectManager {

class QtVersion;

namespace Internal {

class AndroidRunConfiguration;
class AndroidRunner;

class AndroidDebugSupport : public QObject
{
    Q_OBJECT
public:
    static ProjectExplorer::RunControl *createDebugRunControl(AndroidRunConfiguration *runConfig);

    AndroidDebugSupport(AndroidRunConfiguration *runConfig,
        Debugger::DebuggerRunControl *runControl);
    ~AndroidDebugSupport();

private slots:
    void handleRemoteProcessStarted(int gdbServerPort=-1, int qmlPort=-1);
    void handleRemoteProcessFinished(const QString & errorMsg);

    void handleRemoteOutput(const QByteArray &output);
    void handleRemoteErrorOutput(const QByteArray &output);

private:
    static QStringList qtSoPaths(QtVersion * qtVersion);

private:
    const QPointer<Debugger::DebuggerRunControl> m_runControl;
    const QPointer<AndroidRunConfiguration> m_runConfig;
    AndroidRunner * const m_runner;
    const AndroidRunConfiguration::DebuggingType m_debuggingType;
    const QString m_dumperLib;

    int m_gdbServerPort;
    int m_qmlPort;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDDEBUGSUPPORT_H
