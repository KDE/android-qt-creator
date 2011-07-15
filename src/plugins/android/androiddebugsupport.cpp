/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androiddebugsupport.h"

#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidrunner.h"
#include "androidtarget.h"

#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <debugger/debuggerengine.h>
#include <debugger/debuggerstartparameters.h>

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4target.h>
#include <qt4projectmanager/qt4project.h>


#include <QtCore/QDir>
#include <QtCore/QFileInfo>

using namespace Core;
using namespace Debugger;
using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace Android {
namespace Internal {

static const char * const qMakeVariables[] = {
         "QT_INSTALL_LIBS",
         "QT_INSTALL_PLUGINS",
         "QT_INSTALL_IMPORTS"
};


RunControl *AndroidDebugSupport::createDebugRunControl(AndroidRunConfiguration *runConfig)
{
    DebuggerStartParameters params;
    params.toolChainAbi = runConfig->abi();
    params.dumperLibrary = runConfig->dumperLib();
    params.startMode = AttachToRemote;
    params.executable = runConfig->androidTarget()->qt4Project()->rootProjectNode()->buildDir()+"/app_process";
    params.debuggerCommand = runConfig->gdbCmd();
    params.remoteChannel = runConfig->remoteChannel();
    params.displayName = runConfig->androidTarget()->packageName();

    params.solibSearchPath.clear();

    QList<Qt4ProFileNode *> nodes = runConfig->androidTarget()->qt4Project()->allProFiles();
    foreach(Qt4ProFileNode * node, nodes)
        if (node->projectType() == ApplicationTemplate)
            params.solibSearchPath.append(node->targetInformation().buildDir);

    params.solibSearchPath.append(qtSoPaths(runConfig->activeQt4BuildConfiguration()->qtVersion()));

    params.useServerStartScript = true;
    params.remoteArchitecture = QLatin1String("arm");

    DebuggerRunControl * const debuggerRunControl
        = DebuggerPlugin::createDebugger(params, runConfig);
    new AndroidDebugSupport(runConfig, debuggerRunControl);
    return debuggerRunControl;
}

AndroidDebugSupport::AndroidDebugSupport(AndroidRunConfiguration *runConfig,
    DebuggerRunControl *runControl)
    : QObject(runControl), m_runControl(runControl), m_runConfig(runConfig),
      m_runner(new AndroidRunner(this, runConfig, true)),
      m_debuggingType(runConfig->debuggingType()),
      m_gdbServerPort(5039), m_qmlPort(-1)
{
#ifdef __GNUC__
#warning FIXME Android m_gdbServerPort(5039)
#endif
    connect(m_runControl->engine(), SIGNAL(requestRemoteSetup()), m_runner,
        SLOT(start()));
    connect(m_runControl, SIGNAL(finished()), m_runner,
            SLOT(stop()));

    connect(m_runner, SIGNAL(remoteProcessStarted(int,int)),
        SLOT(handleRemoteProcessStarted(int,int)));
    connect(m_runner, SIGNAL(remoteProcessFinished(const QString &)),
        SLOT(handleRemoteProcessFinished(const QString &)));

    connect(m_runner, SIGNAL(remoteErrorOutput(QByteArray)),
        SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(m_runner, SIGNAL(remoteOutput(QByteArray)),
        SLOT(handleRemoteOutput(QByteArray)));
}

AndroidDebugSupport::~AndroidDebugSupport()
{
}

void AndroidDebugSupport::handleRemoteProcessStarted(int gdbServerPort, int qmlPort)
{
    disconnect(m_runner, SIGNAL(remoteProcessStarted(int,int)),
        this, SLOT(handleRemoteProcessStarted(int,int)));
    m_runControl->engine()->handleRemoteSetupDone(gdbServerPort, qmlPort);
}

void AndroidDebugSupport::handleRemoteProcessFinished(const QString & errorMsg)
{
    disconnect(m_runner, SIGNAL(remoteProcessFinished(const QString &)),
        this,SLOT(handleRemoteProcessFinished(const QString &)));
    m_runControl->engine()->handleRemoteSetupFailed(errorMsg);
}

void AndroidDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    if (m_runControl)
        m_runControl->showMessage(QString::fromUtf8(output), AppOutput);
}

void AndroidDebugSupport::handleRemoteErrorOutput(const QByteArray &output)
{
   if (m_runControl)
        m_runControl->showMessage(QString::fromUtf8(output), AppError);
}

QStringList AndroidDebugSupport::qtSoPaths(QtSupport::BaseQtVersion * qtVersion)
{
    QSet<QString> paths;
    for (uint i = 0; i < sizeof qMakeVariables / sizeof qMakeVariables[0]; ++i)
    {
        if (!qtVersion->versionInfo().contains(qMakeVariables[i]))
            continue;
        QDirIterator it(qtVersion->versionInfo()[qMakeVariables[i]], QStringList()<<"*.so", QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
        {
            it.next();
            paths.insert(it.fileInfo().absolutePath());
        }
    }
    return paths.toList();
}

} // namespace Internal
} // namespace Qt4ProjectManager
