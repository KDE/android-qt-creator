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

#ifndef PROJECTEXPLORER_ABI_H
#define PROJECTEXPLORER_ABI_H

#include "projectexplorer_export.h"

#include <utils/fileutils.h>

#include <QList>

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// ABI (documentation inside)
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT Abi
{
public:
    enum Architecture {
        ArmArchitecture,
        X86Architecture,
        ItaniumArchitecture,
        MipsArchitecture,
        PowerPCArchitecture,
        ShArchitecture,
        UnknownArchitecture
    };

    enum OS {
        BsdOS,
        LinuxOS,
        MacOS,
        SymbianOS,
        UnixOS,
        WindowsOS,
        UnknownOS
    };

    enum OSFlavor {
        // BSDs
        FreeBsdFlavor,
        NetBsdFlavor,
        OpenBsdFlavor,

        // Linux
        GenericLinuxFlavor,
        AndroidLinuxFlavor,
        HarmattanLinuxFlavor,
        MaemoLinuxFlavor,
        MeegoLinuxFlavor,

        // Mac
        GenericMacFlavor,

        // Symbian
        SymbianDeviceFlavor,
        SymbianEmulatorFlavor,

        // Unix
        GenericUnixFlavor,
        SolarisUnixFlavor,

        // Windows
        WindowsMsvc2005Flavor,
        WindowsMsvc2008Flavor,
        WindowsMsvc2010Flavor,
        WindowsMSysFlavor,
        WindowsCEFlavor,

        UnknownFlavor
    };

    enum BinaryFormat {
        ElfFormat,
        MachOFormat,
        PEFormat,
        RuntimeQmlFormat,
        UnknownFormat
    };

    Abi() :
        m_architecture(UnknownArchitecture), m_os(UnknownOS),
        m_osFlavor(UnknownFlavor), m_binaryFormat(UnknownFormat), m_wordWidth(0)
    { }

    Abi(const Architecture &a, const OS &o,
        const OSFlavor &so, const BinaryFormat &f, unsigned char w);
    Abi(const QString &abiString);

    bool operator != (const Abi &other) const;
    bool operator == (const Abi &other) const;
    bool isCompatibleWith(const Abi &other) const;

    bool isValid() const;
    bool isNull() const;

    Architecture architecture() const { return m_architecture; }
    OS os() const { return m_os; }
    OSFlavor osFlavor() const { return m_osFlavor; }
    BinaryFormat binaryFormat() const { return m_binaryFormat; }
    unsigned char wordWidth() const { return m_wordWidth; }

    QString toString() const;

    static QString toString(const Architecture &a);
    static QString toString(const OS &o);
    static QString toString(const OSFlavor &of);
    static QString toString(const BinaryFormat &bf);
    static QString toString(int w);

    static QList<OSFlavor> flavorsForOs(const OS &o);

    static Abi hostAbi();
    static QList<Abi> abisOfBinary(const Utils::FileName &path);

private:
    Architecture m_architecture;
    OS m_os;
    OSFlavor m_osFlavor;
    BinaryFormat m_binaryFormat;
    unsigned char m_wordWidth;
};

} // namespace ProjectExplorer

#endif // PROJECTEXPLORER_ABI_H
