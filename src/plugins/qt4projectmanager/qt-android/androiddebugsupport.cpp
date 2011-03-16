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
#include "qt4androidtarget.h"

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

namespace Qt4ProjectManager {
namespace Internal {

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
    params.solibSearchPath.append(runConfig->activeQt4BuildConfiguration()->qtVersion()->sourcePath()+"/lib");
    QList<Qt4ProFileNode *> nodes = runConfig->androidTarget()->qt4Project()->allProFiles();
    foreach(Qt4ProFileNode * node, nodes)
        if (node->projectType() == ApplicationTemplate)
            params.solibSearchPath.append(node->targetInformation().buildDir);

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
#warning FIXME Android m_gdbServerPort(5039)
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

} // namespace Internal
} // namespace Qt4ProjectManager
