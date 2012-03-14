/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DEBUGGER_DEBUGGERSTARTPARAMETERS_H
#define DEBUGGER_DEBUGGERSTARTPARAMETERS_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <utils/ssh/sshconnection.h>
#include <utils/environment.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QMetaType>

namespace Debugger {

// Note: This is part of the "soft interface" of the debugger plugin.
// Do not add anything that needs implementation in a .cpp file.

class DEBUGGER_EXPORT DebuggerStartParameters
{
public:
    enum CommunicationChannel {
        CommunicationChannelTcpIp,
        CommunicationChannelUsb
    };

    DebuggerStartParameters()
      : isSnapshot(false),
        attachPID(-1),
        useTerminal(false),
        breakOnMain(false),
        languages(AnyLanguage),
        qmlServerAddress(QLatin1String("127.0.0.1")),
        qmlServerPort(ProjectExplorer::Constants::QML_DEFAULT_DEBUG_SERVER_PORT),
        useServerStartScript(false),
        connParams(Utils::SshConnectionParameters::NoProxy),
        requestRemoteSetup(false),
        startMode(NoStartMode),
        executableUid(0),
        communicationChannel(CommunicationChannelTcpIp),
        serverPort(0),
        testReceiver(0),
        testCallback(0),
        testCase(0)
    {}

    QString executable;
    QString displayName; // Used in the Snapshots view.
    QString startMessage; // First status message shown.
    QString coreFile;
    QString overrideStartScript; // Used in attach to core and remote debugging
    bool isSnapshot; // Set if created internally.
    QString processArgs;
    Utils::Environment environment;
    QString workingDirectory;
    qint64 attachPID;
    bool useTerminal;
    bool breakOnMain;
    DebuggerLanguages languages;

    // Used by AttachCrashedExternal.
    QString crashParameter;

    // Used by Qml debugging.
    QString qmlServerAddress;
    quint16 qmlServerPort;
    QString projectSourceDirectory;
    QString projectBuildDirectory;
    QStringList projectSourceFiles;


    QString qtInstallPath;
    // Used by remote debugging.
    QString remoteChannel;
    QString remoteArchitecture;
    QString gnuTarget;
    QString symbolFileName;
    bool useServerStartScript;
    QString serverStartScript;
    QString sysroot;
    QString searchPath; // Gdb "set solib-search-path"
    QString debugInfoLocation; // Gdb "set-debug-file-directory".
    QStringList debugSourceLocation; // Gdb "directory"
    QByteArray remoteDumperLib;
    QByteArray remoteSourcesDir;
    QString remoteMountPoint;
    QString localMountDir;
    Utils::SshConnectionParameters connParams;
    bool requestRemoteSetup;

    QString debuggerCommand;
    ProjectExplorer::Abi toolChainAbi;

    QString dumperLibrary;
    QStringList solibSearchPath;
    QStringList dumperLibraryLocations;
    DebuggerStartMode startMode;

    // For Symbian debugging.
    quint32 executableUid;
    CommunicationChannel communicationChannel;
    QString serverAddress;
    quint16 serverPort;

    // For Debugger testing.
    QObject *testReceiver;
    const char *testCallback;
    int testCase;
};

} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::DebuggerStartParameters)

#endif // DEBUGGER_DEBUGGERSTARTPARAMETERS_H

