/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "tcpportsgatherer.h"
#include "qtcassert.h"

#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QStringList>

#ifdef Q_OS_WIN
#include <QLibrary>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif

#if defined(Q_OS_WIN) && defined(Q_CC_MINGW)

// Missing declaration for MinGW.org.
// MinGW-w64 already has it.
#ifndef __MINGW64_VERSION_MAJOR
typedef enum { } MIB_TCP_STATE;
#endif

typedef struct _MIB_TCP6ROW {
    MIB_TCP_STATE State;
    IN6_ADDR LocalAddr;
    DWORD dwLocalScopeId;
    DWORD dwLocalPort;
    IN6_ADDR RemoteAddr;
    DWORD dwRemoteScopeId;
    DWORD dwRemotePort;
} MIB_TCP6ROW, *PMIB_TCP6ROW;

typedef struct _MIB_TCP6TABLE {
    DWORD dwNumEntries;
    MIB_TCP6ROW table[ANY_SIZE];
} MIB_TCP6TABLE, *PMIB_TCP6TABLE;

#endif // defined(Q_OS_WIN) && defined(Q_CC_MINGW)

namespace Utils {
namespace Internal {

class TcpPortsGathererPrivate
{
public:
    TcpPortsGathererPrivate(TcpPortsGatherer::ProtocolFlags protocolFlags)
        : protocolFlags(protocolFlags) {}

    TcpPortsGatherer::ProtocolFlags protocolFlags;
    PortList usedPorts;

    void updateWin(TcpPortsGatherer::ProtocolFlags protocolFlags);
    void updateLinux(TcpPortsGatherer::ProtocolFlags protocolFlags);
    void updateNetstat(TcpPortsGatherer::ProtocolFlags protocolFlags);
};

#ifdef Q_OS_WIN
template <typename Table >
QSet<int> usedTcpPorts(ULONG (__stdcall *Func)(Table*, PULONG, BOOL))
{
    Table *table = static_cast<Table*>(malloc(sizeof(Table)));
    DWORD dwSize = sizeof(Table);

    // get the necessary size into the dwSize variable
    DWORD dwRetVal = Func(table, &dwSize, false);
    if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
        free(table);
        table = static_cast<Table*>(malloc(dwSize));
    }

    // get the actual data
    QSet<int> result;
    dwRetVal = Func(table, &dwSize, false);
    if (dwRetVal == NO_ERROR) {
        for (quint32 i = 0; i < table->dwNumEntries; i++) {
            quint16 port = ntohs(table->table[i].dwLocalPort);
            if (!result.contains(port))
                result.insert(port);
        }
    } else {
        qWarning() << "TcpPortsGatherer: GetTcpTable failed with" << dwRetVal;
    }

    free(table);
    return result;
}
#endif

void TcpPortsGathererPrivate::updateWin(TcpPortsGatherer::ProtocolFlags protocolFlags)
{
#ifdef Q_OS_WIN
    QSet<int> ports;

    if (protocolFlags & TcpPortsGatherer::IPv4Protocol)
        ports.unite(usedTcpPorts<MIB_TCPTABLE>(GetTcpTable));

    //Dynamically load symbol for GetTcp6Table for systems that dont have support for IPV6,
    //eg Windows XP
    typedef ULONG (__stdcall *GetTcp6TablePtr)(PMIB_TCP6TABLE, PULONG, BOOL);
    static GetTcp6TablePtr getTcp6TablePtr = 0;

    if (!getTcp6TablePtr)
        getTcp6TablePtr = (GetTcp6TablePtr)QLibrary::resolve(QLatin1String("Iphlpapi.dll"),
                                                             "GetTcp6Table");

    if (getTcp6TablePtr && (protocolFlags & TcpPortsGatherer::IPv6Protocol))
        ports.unite(usedTcpPorts<MIB_TCP6TABLE>(getTcp6TablePtr));

    foreach (int port, ports) {
        if (!usedPorts.contains(port))
            usedPorts.addPort(port);
    }
#endif
    Q_UNUSED(protocolFlags);
}

void TcpPortsGathererPrivate::updateLinux(TcpPortsGatherer::ProtocolFlags protocolFlags)
{
    QStringList filePaths;
    if (protocolFlags & TcpPortsGatherer::IPv4Protocol)
        filePaths.append(QLatin1String("/proc/net/tcp"));
    if (protocolFlags & TcpPortsGatherer::IPv6Protocol)
        filePaths.append(QLatin1String("/proc/net/tcp6"));

    foreach (const QString &filePath, filePaths) {
        QFile file(filePath);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            qWarning() << "TcpPortsGatherer: Cannot open file"
                       << filePath << ":" << file.errorString();
            continue;
        }

        if (file.atEnd()) // read first line describing the output
            file.readLine();

        static QRegExp pattern(QLatin1String("^\\s*" // start of line, whitespace
                                             "\\d+:\\s*" // integer, colon, space
                                             "[0-9A-Fa-f]+:" // hexadecimal number (ip), colon
                                             "([0-9A-Fa-f]+)"  // hexadecimal number (port!)
                                             ));
        while (!file.atEnd()) {
            QByteArray line = file.readLine();
            if (pattern.indexIn(line) != -1) {
                bool isNumber;
                quint16 port = pattern.cap(1).toUShort(&isNumber, 16);
                QTC_ASSERT(isNumber, continue);
                if (!usedPorts.contains(port))
                    usedPorts.addPort(port);
            } else {
                qWarning() << "TcpPortsGatherer: File" << filePath << "has unexpected format.";
                continue;
            }
        }
    }
}

// Only works with FreeBSD version of netstat like we have on Mac OS X
void TcpPortsGathererPrivate::updateNetstat(TcpPortsGatherer::ProtocolFlags protocolFlags)
{
    QStringList netstatArgs;

    netstatArgs.append(QLatin1String("-a"));     // show also sockets of server processes
    netstatArgs.append(QLatin1String("-n"));     // show network addresses as numbers
    netstatArgs.append(QLatin1String("-p"));
    netstatArgs.append(QLatin1String("tcp"));
    if (protocolFlags != TcpPortsGatherer::AnyIPProcol) {
        netstatArgs.append(QLatin1String("-f")); // limit to address family
        if (protocolFlags == TcpPortsGatherer::IPv4Protocol)
            netstatArgs.append(QLatin1String("inet"));
        else
            netstatArgs.append(QLatin1String("inet6"));
    }

    QProcess netstatProcess;
    netstatProcess.start(QLatin1String("netstat"), netstatArgs);
    if (!netstatProcess.waitForFinished(30000)) {
        qWarning() << "TcpPortsGatherer: netstat did not return in time.";
        return;
    }

    QList<QByteArray> output = netstatProcess.readAllStandardOutput().split('\n');
    foreach (const QByteArray &line, output) {
        static QRegExp pattern(QLatin1String("^tcp[46]+" // "tcp", followed by "4", "6", "46"
                                             "\\s+\\d+"           // whitespace, number (Recv-Q)
                                             "\\s+\\d+"           // whitespace, number (Send-Q)
                                             "\\s+(\\S+)"));       // whitespace, Local Address
        if (pattern.indexIn(line) != -1) {
            QString localAddress = pattern.cap(1);

            // Examples of local addresses:
            // '*.56501' , '*.*' 'fe80::1%lo0.123'
            int portDelimiterPos = localAddress.lastIndexOf(".");
            if (portDelimiterPos == -1)
                continue;

            localAddress = localAddress.mid(portDelimiterPos + 1);
            bool isNumber;
            quint16 port = localAddress.toUShort(&isNumber);
            if (!isNumber)
                continue;

            if (!usedPorts.contains(port))
                usedPorts.addPort(port);
        }
    }
}

} // namespace Internal


/*!
  \class Utils::TcpPortsGatherer

  \brief Gather the list of local TCP ports already in use.

  Query the system for the list of local TCP ports already in use. This information can be used
  to select a port for use in a range.
*/

TcpPortsGatherer::TcpPortsGatherer(TcpPortsGatherer::ProtocolFlags protocolFlags)
    : d(new Internal::TcpPortsGathererPrivate(protocolFlags))
{
    update();
}

TcpPortsGatherer::~TcpPortsGatherer()
{
    delete d;
}

void TcpPortsGatherer::update()
{
    d->usedPorts = PortList();

#if defined(Q_OS_WIN)
    d->updateWin(d->protocolFlags);
#elif defined(Q_OS_LINUX)
    d->updateLinux(d->protocolFlags);
#else
    d->updateNetstat(d->protocolFlags);
#endif
}

PortList TcpPortsGatherer::usedPorts() const
{
    return d->usedPorts;
}

/*!
  Select a port out of \a freePorts that is not yet used.

  Returns the port, or 0 if no free port is available.
  */
quint16 TcpPortsGatherer::getNextFreePort(PortList *freePorts)
{
    QTC_ASSERT(freePorts, return 0);
    while (freePorts->hasMore()) {
        const int port = freePorts->getNext();
        if (!d->usedPorts.contains(port))
            return port;
    }
    return 0;
}

} // namespace Utils
