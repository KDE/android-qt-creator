/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "launcher.h"
#include "communicationstarter.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QSharedPointer>
#include <QtCore/QDebug>
#include <QtCore/QStringList>

static const char usageC[] =
"\n"
"Usage: %1 [options] <trk_port_name>\n"
"       %1 [options] <trk_port_name> <remote_executable_name> [-- args]\n"
"       %1 [options] -i <trk_port_name> remote_sis_file\n"
"       %1 [options] -I local_sis_file remote_sis_file] [<remote_executable_name>] [-- args]\n"
"       %1 [options] -c local_file remote_file\n"
"       %1 [options] -C remote_file [local_file|-]\n"
"\nOptions:\n    -v verbose\n"
            "    -q quiet\n"
            "    -b Prompt for Bluetooth connect (Linux only)\n"
            "    -f turn serial message frame off (Bluetooth)\n"
"\nPing:\n"
"%1 COM5\n"
"\nRemote launch:\n"
"%1 COM5 C:\\sys\\bin\\test.exe\n"
"\nInstallation:\n"
"%1 -i COM5 C:\\Data\\test_gcce_udeb.sisx\n"
"\nInstallation and remote launch:\n"
"%1 -i COM5 C:\\Data\\test_gcce_udeb.sisx C:\\sys\\bin\\test.exe\n"
"\nCopy from local file, installation:\n"
"%1 -I COM5 C:\\Projects\\test\\test_gcce_udeb.sisx C:\\Data\\test_gcce_udeb.sisx\n"
"\nCopy from local file, installation and remote launch:\n"
"%1 -I COM5 C:\\Projects\\test\\test_gcce_udeb.sisx C:\\Data\\test_gcce_udeb.sisx C:\\sys\\bin\\test.exe\n"
"\nCopy from local file\n"
"%1 -c COM5 c:\\foo.dat C:\\Data\\foo.dat\n"
"\nCopy to local file\n"
"%1 -C COM5 C:\\Data\\foo.dat c:\\foo.dat\n";

static void usage()
{
    const QString msg = QString::fromLatin1(usageC).arg(QCoreApplication::applicationName());
    qWarning("%s", qPrintable(msg));
}

typedef QSharedPointer<trk::Launcher> TrkLauncherPtr;

// Parse arguments, return pointer or a null none.

static inline TrkLauncherPtr createLauncher(trk::Launcher::Actions actions,
                                            const QString &serverName,
                                            bool serialFrame,
                                            int verbosity)
{
    TrkLauncherPtr launcher(new trk::Launcher(actions));
    launcher->setTrkServerName(serverName);
    launcher->setSerialFrame(serialFrame);
    launcher->setVerbose(verbosity);
    return launcher;
}

static TrkLauncherPtr parseArguments(const QStringList &arguments, bool *bluetooth)
{
    enum Mode { Ping, RemoteLaunch, Install, CustomInstall, Copy, Download };
    // Parse away options
    Mode mode = Ping;
    bool serialFrame = true;
    const int argCount = arguments.size();
    int verbosity = 1;
    *bluetooth = false;
    QStringList remoteArguments;
    trk::Launcher::Actions actions = trk::Launcher::ActionPingOnly;
    int a = 1;
    for ( ; a < argCount; a++) {
        const QString option = arguments.at(a);
        if (!option.startsWith(QLatin1Char('-')))
            break;
        if (option.size() != 2)
            return TrkLauncherPtr();
        switch (option.at(1).toAscii()) {
        case 'v':
            verbosity++;
            break;
        case 'q':
            verbosity = 0;
            break;
        case 'f':
            serialFrame = false;
            break;
        case 'b':
            *bluetooth = true;
            break;
        case 'i':
            mode = Install;
            actions = trk::Launcher::ActionInstall;
            break;
        case 'I':
            mode = CustomInstall;
            actions = trk::Launcher::ActionCopyInstall;
            break;
        case 'c':
            mode = Copy;
            actions = trk::Launcher::ActionCopy;
            break;
        case 'C':
            mode = Download;
            actions = trk::Launcher::ActionDownload;
            break;
        default:
            return TrkLauncherPtr();
        }
    }
    // Parse for '--' delimiter for remote executable argunment
    int pastArguments = a;
    for ( ; pastArguments < argCount && arguments.at(pastArguments) != QLatin1String("--"); pastArguments++) ;
    if (pastArguments != argCount)
        for (int ra = pastArguments + 1; ra < argCount; ra++)
            remoteArguments.push_back(arguments.at(ra));
    // Evaluate arguments
    const int remainingArgsCount = pastArguments -a ;
    // Ping and launch are only distinguishable by argument counts
    if (mode == Ping && remainingArgsCount > 1)
        mode = RemoteLaunch;
    switch (mode) {
    case Ping:
        if (remainingArgsCount == 1)
            return createLauncher(actions, arguments.at(a), serialFrame, verbosity);
        break;
    case RemoteLaunch:
        if (remainingArgsCount == 2) {
            // remote exec
            TrkLauncherPtr launcher = createLauncher(actions, arguments.at(a), serialFrame, verbosity);
            launcher->addStartupActions(trk::Launcher::ActionRun);
            launcher->setFileName(arguments.at(a + 1));
            launcher->setCommandLineArgs(remoteArguments.join(" "));
            return launcher;

        }
        break;
    case Install:
        if (remainingArgsCount == 3 || remainingArgsCount == 2) {
            TrkLauncherPtr launcher = createLauncher(actions, arguments.at(a), serialFrame, verbosity);
            launcher->setInstallFileNames(QStringList() << arguments.at(a + 1));
            if (remainingArgsCount == 3) {
                launcher->addStartupActions(trk::Launcher::ActionRun);
                launcher->setFileName(arguments.at(a + 2));
                launcher->setCommandLineArgs(remoteArguments.join(" "));
            }
            return launcher;
        }
        break;
    case CustomInstall:
        if (remainingArgsCount == 4 || remainingArgsCount == 3) {
            TrkLauncherPtr launcher = createLauncher(actions, arguments.at(a), serialFrame, verbosity);
            launcher->setCopyFileNames(QStringList() << arguments.at(a + 1), QStringList() << arguments.at(a + 2));
            launcher->setInstallFileNames(QStringList() << arguments.at(a + 2));
            if (remainingArgsCount == 4) {
                launcher->addStartupActions(trk::Launcher::ActionRun);
                launcher->setFileName(arguments.at(a + 3));
                launcher->setCommandLineArgs(remoteArguments.join(" "));
            }
            return launcher;
        }
        break;
    case Copy:
        if (remainingArgsCount == 3) {
            TrkLauncherPtr launcher = createLauncher(actions, arguments.at(a), serialFrame, verbosity);
            launcher->setCopyFileNames(QStringList() << arguments.at(a + 1), QStringList() << arguments.at(a + 2));
            return launcher;
        }
        break;
    case Download:
        if (remainingArgsCount == 3) {
            TrkLauncherPtr launcher = createLauncher(actions, arguments.at(a), serialFrame, verbosity);
            launcher->setDownloadFileName(arguments.at(a + 1), arguments.at(a + 2));
            return launcher;
        }
        break;
    }
    return TrkLauncherPtr();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QLatin1String("trklauncher"));
    QCoreApplication::setOrganizationName(QLatin1String("Nokia"));

    bool bluetooth;
    const TrkLauncherPtr launcher = parseArguments(app.arguments(), &bluetooth);
    if (launcher.isNull()) {
        usage();
        return 1;
    }
    QObject::connect(launcher.data(), SIGNAL(finished()), &app, SLOT(quit()));
    QObject::connect(launcher.data(), SIGNAL(processStopped(uint,uint,uint,QString)),
                     launcher.data(), SLOT(terminate()));
    // BLuetooth: Open with prompt
    QString errorMessage;
    if (bluetooth && !trk::ConsoleBluetoothStarter::startBluetooth(launcher->trkDevice(),
                                                     launcher.data(),
                                                     30, &errorMessage)) {
        qWarning("%s\n", qPrintable(errorMessage));
        return -1;
    }
    if (launcher->startServer(&errorMessage))
        return app.exec();
    qWarning("%s\n", qPrintable(errorMessage));
    return 4;
}
