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

#include "androiddebugsupport.h"

#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidrunner.h"

#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <debugger/debuggerengine.h>
#include <debugger/debuggerstartparameters.h>

#include <projectexplorer/toolchaintype.h>
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
    params.toolChainType = ProjectExplorer::ToolChain_GCC_ANDROID;
    params.dumperLibrary = runConfig->dumperLib();
    params.startMode = AttachToRemote;
    params.executable = runConfig->qt4Target()->qt4Project()->rootProjectNode()->buildDir()+"/app_process";
    qDebug()<<params.executable;
    params.debuggerCommand = runConfig->gdbCmd();
    params.remoteChannel = runConfig->remoteChannel();
    params.solibSearchPath.clear();
    params.solibSearchPath.append(runConfig->activeQt4BuildConfiguration()->qtVersion()->sourcePath()+"/lib");
    QList<Qt4ProFileNode *> nodes = runConfig->qt4Target()->qt4Project()->leafProFiles();
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
    disconnect(m_runner, SIGNAL(remoteProcessFinished(const QString &)),
        this,SLOT(handleRemoteProcessFinished(const QString &)));
    m_runControl->engine()->handleRemoteSetupDone(gdbServerPort, qmlPort);
}

void AndroidDebugSupport::handleRemoteProcessFinished(const QString & errorMsg)
{
    disconnect(m_runner, SIGNAL(remoteProcessStarted(int,int)),
        this,SLOT(handleRemoteProcessStarted(int,int)));
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
