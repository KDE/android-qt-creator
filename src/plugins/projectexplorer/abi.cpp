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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "abi.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QtEndian>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QSysInfo>

/*!
    \class ProjectExplorer::Abi

    \brief Represents the Application Binary Interface (ABI) of a target platform.

    \sa ProjectExplorer::ToolChain
*/

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------

static Abi macAbiForCpu(quint32 type) {
    switch (type) {
    case 7: // CPU_TYPE_X86, CPU_TYPE_I386
        return Abi(Abi::X86Architecture, Abi::MacOS, Abi::GenericMacFlavor, Abi::MachOFormat, 32);
    case 0x01000000 +  7: // CPU_TYPE_X86_64
        return Abi(Abi::X86Architecture, Abi::MacOS, Abi::GenericMacFlavor, Abi::MachOFormat, 64);
    case 18: // CPU_TYPE_POWERPC
        return Abi(Abi::PowerPCArchitecture, Abi::MacOS, Abi::GenericMacFlavor, Abi::MachOFormat, 32);
    case 0x01000000 + 18: // CPU_TYPE_POWERPC64
        return Abi(Abi::PowerPCArchitecture, Abi::MacOS, Abi::GenericMacFlavor, Abi::MachOFormat, 32);
    case 12: // CPU_TYPE_ARM
        return Abi(Abi::ArmArchitecture, Abi::MacOS, Abi::GenericMacFlavor, Abi::MachOFormat, 32);
    default:
        return Abi();
    }
}

static QList<Abi> parseCoffHeader(const QByteArray &data)
{
    QList<Abi> result;
    if (data.size() < 20)
        return result;

    Abi::Architecture arch = Abi::UnknownArchitecture;
    Abi::OSFlavor flavor = Abi::UnknownFlavor;
    int width = 0;

    // Get machine field from COFF file header
    quint16 machine = (data.at(1) << 8) + data.at(0);
    switch (machine) {
    case 0x8664: // x86_64
        arch = Abi::X86Architecture;
        width = 64;
        break;
    case 0x014c: // i386
        arch = Abi::X86Architecture;
        width = 32;
        break;
    case 0x0166: // MIPS, little endian
        arch = Abi::MipsArcitecture;
        width = 32;
        break;
    case 0x0200: // ia64
        arch = Abi::ItaniumArchitecture;
        width = 64;
        break;
    }

    if (data.size() >= 68) {
        // Get Major and Minor Image Version from optional header fields
        quint32 image = (data.at(67) << 24) + (data.at(66) << 16) + (data.at(65) << 8) + data.at(64);
        if (image == 1) { // Image is 1 for mingw and higher for MSVC (4.something in some encoding)
            flavor = Abi::WindowsMSysFlavor;
        } else {
            switch (data.at(22)) {
            case 8:
                flavor = Abi::WindowsMsvc2005Flavor;
                break;
            case 9:
                flavor = Abi::WindowsMsvc2008Flavor;
                break;
            case 10:
                flavor = Abi::WindowsMsvc2010Flavor;
                break;
            default:
                // Keep unknown flavor
                break;
            }
        }
    }

    if (arch != Abi::UnknownArchitecture && width != 0)
        result.append(Abi(arch, Abi::WindowsOS, flavor, Abi::PEFormat, width));

    return result;
}

static QList<Abi> abiOf(const QByteArray &data)
{
    QList<Abi> result;

    if (data.size() >= 20
            && static_cast<unsigned char>(data.at(0)) == 0x7f && static_cast<unsigned char>(data.at(1)) == 'E'
            && static_cast<unsigned char>(data.at(2)) == 'L' && static_cast<unsigned char>(data.at(3)) == 'F') {
        // ELF format:
        bool isLsbEncoded = (static_cast<quint8>(data.at(5)) == 1);
        quint16 machine = (data.at(19) << 8) + data.at(18);
        if (!isLsbEncoded)
            machine = qFromBigEndian(machine);
        quint8 osAbi = static_cast<quint8>(data.at(7));

        Abi::OS os = Abi::UnixOS;
        Abi::OSFlavor flavor = Abi::GenericUnixFlavor;
        // http://www.sco.com/developers/gabi/latest/ch4.eheader.html#elfid
        switch (osAbi) {
        case 2: // NetBSD:
            os = Abi::BsdOS;
            flavor = Abi::NetBsdFlavor;
            break;
        case 3: // Linux:
        case 0: // no extra info available: Default to Linux:
            os = Abi::LinuxOS;
            flavor = Abi::GenericLinuxFlavor;
            break;
        case 6: // Solaris:
            os = Abi::UnixOS;
            flavor = Abi::SolarisUnixFlavor;
            break;
        case 9: // FreeBSD:
            os = Abi::BsdOS;
            flavor = Abi::FreeBsdFlavor;
            break;
        case 12: // OpenBSD:
            os = Abi::BsdOS;
            flavor = Abi::OpenBsdFlavor;
        }

        switch (machine) {
        case 3: // EM_386
            result.append(Abi(Abi::X86Architecture, os, flavor, Abi::ElfFormat, 32));
            break;
        case 8: // EM_MIPS
            result.append(Abi(Abi::MipsArcitecture, os, flavor, Abi::ElfFormat, 32));
            break;
        case 20: // EM_PPC
            result.append(Abi(Abi::PowerPCArchitecture, os, flavor, Abi::ElfFormat, 32));
            break;
        case 21: // EM_PPC64
            result.append(Abi(Abi::PowerPCArchitecture, os, flavor, Abi::ElfFormat, 64));
            break;
        case 40: // EM_ARM
            result.append(Abi(Abi::ArmArchitecture, os, flavor, Abi::ElfFormat, 32));
            break;
        case 62: // EM_X86_64
            result.append(Abi(Abi::X86Architecture, os, flavor, Abi::ElfFormat, 64));
            break;
        case 50: // EM_IA_64
            result.append(Abi(Abi::ItaniumArchitecture, os, flavor, Abi::ElfFormat, 64));
            break;
        default:
            ;;
        }
    } else if (data.size() >= 8
               && (static_cast<unsigned char>(data.at(0)) == 0xce || static_cast<unsigned char>(data.at(0)) == 0xcf)
               && static_cast<unsigned char>(data.at(1)) == 0xfa
               && static_cast<unsigned char>(data.at(2)) == 0xed && static_cast<unsigned char>(data.at(3)) == 0xfe) {
        // Mach-O format (Mac non-fat binary, 32 and 64bit magic)
        quint32 type = (data.at(7) << 24) + (data.at(6) << 16) + (data.at(5) << 8) + data.at(4);
        result.append(macAbiForCpu(type));
    } else if (data.size() >= 8
               && static_cast<unsigned char>(data.at(0)) == 0xca && static_cast<unsigned char>(data.at(1)) == 0xfe
               && static_cast<unsigned char>(data.at(2)) == 0xba && static_cast<unsigned char>(data.at(3)) == 0xbe) {
        // Mac fat binary:
        quint32 count = (data.at(4) << 24) + (data.at(5) << 16) + (data.at(6) << 8) + data.at(7);
        int pos = 8;
        for (quint32 i = 0; i < count; ++i) {
            if (data.size() <= pos + 4)
                break;

            quint32 type = (data.at(pos) << 24) + (data.at(pos + 1) << 16) + (data.at(pos + 2) << 8) + data.at(pos + 3);
            result.append(macAbiForCpu(type));
            pos += 20;
        }
    } else if (data.size() >= 20
               && static_cast<unsigned char>(data.at(16)) == 'E' && static_cast<unsigned char>(data.at(17)) == 'P'
               && static_cast<unsigned char>(data.at(18)) == 'O' && static_cast<unsigned char>(data.at(19)) == 'C') {
        result.append(Abi(Abi::ArmArchitecture, Abi::SymbianOS, Abi::SymbianDeviceFlavor, Abi::ElfFormat, 32));
    } else {
        // Windows PE
        // Windows can have its magic bytes everywhere...
        int pePos = data.indexOf(QByteArray("PE\0\0", 4));
        if (pePos >= 0)
            result = parseCoffHeader(data.mid(pePos + 4));
    }
    return result;
}

// --------------------------------------------------------------------------
// Abi
// --------------------------------------------------------------------------

Abi::Abi(const Architecture &a, const OS &o,
         const OSFlavor &of, const BinaryFormat &f, unsigned char w) :
    m_architecture(a), m_os(o), m_osFlavor(of), m_binaryFormat(f), m_wordWidth(w)
{
    switch (m_os) {
    case ProjectExplorer::Abi::UnknownOS:
        m_osFlavor = UnknownFlavor;
        break;
    case ProjectExplorer::Abi::LinuxOS:
        if (m_osFlavor < GenericLinuxFlavor || m_osFlavor > MeegoLinuxFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case ProjectExplorer::Abi::BsdOS:
        m_osFlavor = FreeBsdFlavor;
        break;
    case ProjectExplorer::Abi::MacOS:
        if (m_osFlavor < GenericMacFlavor || m_osFlavor > GenericMacFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case ProjectExplorer::Abi::SymbianOS:
        if (m_osFlavor < SymbianDeviceFlavor || m_osFlavor > SymbianEmulatorFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case ProjectExplorer::Abi::UnixOS:
        if (m_osFlavor < GenericUnixFlavor || m_osFlavor > GenericUnixFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case ProjectExplorer::Abi::WindowsOS:
        if (m_osFlavor < WindowsMsvc2005Flavor || m_osFlavor > WindowsCEFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    }
}

Abi::Abi(const QString &abiString) :
    m_architecture(UnknownArchitecture), m_os(UnknownOS),
    m_osFlavor(UnknownFlavor), m_binaryFormat(UnknownFormat), m_wordWidth(0)
{
    QStringList abiParts = abiString.split(QLatin1Char('-'));
    if (abiParts.count() >= 1) {
        if (abiParts.at(0) == QLatin1String("unknown"))
            m_architecture = UnknownArchitecture;
        else if (abiParts.at(0) == QLatin1String("arm"))
            m_architecture = ArmArchitecture;
        else if (abiParts.at(0) == QLatin1String("x86"))
            m_architecture = X86Architecture;
        else if (abiParts.at(0) == QLatin1String("mips"))
            m_architecture = MipsArcitecture;
        else if (abiParts.at(0) == QLatin1String("ppc"))
            m_architecture = PowerPCArchitecture;
        else if (abiParts.at(0) == QLatin1String("itanium"))
            m_architecture = ItaniumArchitecture;
        else
            return;
    }

    if (abiParts.count() >= 2) {
        if (abiParts.at(1) == QLatin1String("unknown"))
            m_os = UnknownOS;
        else if (abiParts.at(1) == QLatin1String("linux"))
            m_os = LinuxOS;
        else if (abiParts.at(1) == QLatin1String("bsd"))
            m_os = BsdOS;
        else if (abiParts.at(1) == QLatin1String("macos"))
            m_os = MacOS;
        else if (abiParts.at(1) == QLatin1String("symbian"))
            m_os = SymbianOS;
        else if (abiParts.at(1) == QLatin1String("unix"))
            m_os = UnixOS;
        else if (abiParts.at(1) == QLatin1String("windows"))
            m_os = WindowsOS;
        else

            return;
    }

    if (abiParts.count() >= 3) {
        if (abiParts.at(2) == QLatin1String("unknown"))
            m_osFlavor = UnknownFlavor;
        else if (abiParts.at(2) == QLatin1String("generic") && m_os == LinuxOS)
            m_osFlavor = GenericLinuxFlavor;
        else if (abiParts.at(2) == QLatin1String("android") && m_os == LinuxOS)
            m_osFlavor = AndroidLinuxFlavor;
        else if (abiParts.at(2) == QLatin1String("freebsd") && m_os == BsdOS)
            m_osFlavor = FreeBsdFlavor;
        else if (abiParts.at(2) == QLatin1String("netbsd") && m_os == BsdOS)
            m_osFlavor = NetBsdFlavor;
        else if (abiParts.at(2) == QLatin1String("openbsd") && m_os == BsdOS)
            m_osFlavor = OpenBsdFlavor;
        else if (abiParts.at(2) == QLatin1String("maemo") && m_os == LinuxOS)
            m_osFlavor = MaemoLinuxFlavor;
        else if (abiParts.at(2) == QLatin1String("meego") && m_os == LinuxOS)
            m_osFlavor = MeegoLinuxFlavor;
        else if (abiParts.at(2) == QLatin1String("generic") && m_os == MacOS)
            m_osFlavor = GenericMacFlavor;
        else if (abiParts.at(2) == QLatin1String("device") && m_os == SymbianOS)
            m_osFlavor = SymbianDeviceFlavor;
        else if (abiParts.at(2) == QLatin1String("emulator") && m_os == SymbianOS)
            m_osFlavor = SymbianEmulatorFlavor;
        else if (abiParts.at(2) == QLatin1String("generic") && m_os == UnixOS)
            m_osFlavor = GenericUnixFlavor;
        else if (abiParts.at(2) == QLatin1String("solaris") && m_os == UnixOS)
            m_osFlavor = SolarisUnixFlavor;
        else if (abiParts.at(2) == QLatin1String("msvc2005") && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2005Flavor;
        else if (abiParts.at(2) == QLatin1String("msvc2008") && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2008Flavor;
        else if (abiParts.at(2) == QLatin1String("msvc2010") && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2010Flavor;
        else if (abiParts.at(2) == QLatin1String("msys") && m_os == WindowsOS)
            m_osFlavor = WindowsMSysFlavor;
        else if (abiParts.at(2) == QLatin1String("ce") && m_os == WindowsOS)
            m_osFlavor = WindowsCEFlavor;
        else
            return;
    }

    if (abiParts.count() >= 4) {
        if (abiParts.at(3) == QLatin1String("unknown"))
            m_binaryFormat = UnknownFormat;
        else if (abiParts.at(3) == QLatin1String("elf"))
            m_binaryFormat = ElfFormat;
        else if (abiParts.at(3) == QLatin1String("pe"))
            m_binaryFormat = PEFormat;
        else if (abiParts.at(3) == QLatin1String("mach_o"))
            m_binaryFormat = MachOFormat;
        else if (abiParts.at(3) == QLatin1String("qml_rt"))
            m_binaryFormat = RuntimeQmlFormat;
        else
            return;
    }

    if (abiParts.count() >= 5) {
        const QString &bits = abiParts.at(4);
        if (!bits.endsWith(QLatin1String("bit")))
            return;

        bool ok = false;
        int bitCount = bits.left(bits.count() - 3).toInt(&ok);
        if (!ok)
            return;
        if (bitCount != 8 && bitCount != 16 && bitCount != 32 && bitCount != 64)
            return;
        m_wordWidth = bitCount;
    }
}

QString Abi::toString() const
{
    QStringList dn;
    dn << toString(m_architecture);
    dn << toString(m_os);
    dn << toString(m_osFlavor);
    dn << toString(m_binaryFormat);
    dn << toString(m_wordWidth);

    return dn.join(QLatin1String("-"));
}

bool Abi::operator != (const Abi &other) const
{
    return !operator ==(other);
}

bool Abi::operator == (const Abi &other) const
{
    return m_architecture == other.m_architecture
            && m_os == other.m_os
            && m_osFlavor == other.m_osFlavor
            && m_binaryFormat == other.m_binaryFormat
            && m_wordWidth == other.m_wordWidth;
}

bool Abi::isCompatibleWith(const Abi &other) const
{
    bool isCompat = (architecture() == other.architecture() || other.architecture() == Abi::UnknownArchitecture)
                     && (os() == other.os() || other.os() == Abi::UnknownOS)
                     && (osFlavor() == other.osFlavor() || other.osFlavor() == Abi::UnknownFlavor)
                     && (binaryFormat() == other.binaryFormat() || other.binaryFormat() == Abi::UnknownFormat)
                     && ((wordWidth() == other.wordWidth() && wordWidth() != 0) || other.wordWidth() == 0);
    // *-linux-generic-* is compatible with *-linux-*:
    if (!isCompat && architecture() == other.architecture()
                 && os() == other.os()
                 && osFlavor() == GenericLinuxFlavor
                 && other.os() == LinuxOS
                 && binaryFormat() == other.binaryFormat()
                 && wordWidth() == other.wordWidth())
        isCompat = true;
    if (osFlavor() == AndroidLinuxFlavor || other.osFlavor()==AndroidLinuxFlavor)
        isCompat = (osFlavor() == other.osFlavor() && architecture() == other.architecture());
    return isCompat;
}

bool Abi::isValid() const
{
    return m_architecture != UnknownArchitecture
            && m_os != UnknownOS
            && m_osFlavor != UnknownFlavor
            && m_binaryFormat != UnknownFormat
            && m_wordWidth != 0;
}

QString Abi::toString(const Architecture &a)
{
    switch (a) {
    case ArmArchitecture:
        return QLatin1String("arm");
    case X86Architecture:
        return QLatin1String("x86");
    case MipsArcitecture:
        return QLatin1String("mips");
    case PowerPCArchitecture:
        return QLatin1String("ppc");
    case ItaniumArchitecture:
        return QLatin1String("itanium");
    case UnknownArchitecture: // fall through!
    default:
        return QLatin1String("unknown");
    }
}

QString Abi::toString(const OS &o)
{
    switch (o) {
    case LinuxOS:
        return QLatin1String("linux");
    case BsdOS:
        return QLatin1String("bsd");
    case MacOS:
        return QLatin1String("macos");
    case SymbianOS:
        return QLatin1String("symbian");
    case UnixOS:
        return QLatin1String("unix");
    case WindowsOS:
        return QLatin1String("windows");
    case UnknownOS: // fall through!
    default:
        return QLatin1String("unknown");
    };
}

QString Abi::toString(const OSFlavor &of)
{
    switch (of) {
    case ProjectExplorer::Abi::GenericLinuxFlavor:
        return QLatin1String("generic");
    case ProjectExplorer::Abi::AndroidLinuxFlavor:
        return QLatin1String("android");
    case ProjectExplorer::Abi::FreeBsdFlavor:
        return QLatin1String("freebsd");
    case ProjectExplorer::Abi::NetBsdFlavor:
        return QLatin1String("netbsd");
    case ProjectExplorer::Abi::OpenBsdFlavor:
        return QLatin1String("openbsd");
    case ProjectExplorer::Abi::MaemoLinuxFlavor:
        return QLatin1String("maemo");
    case ProjectExplorer::Abi::HarmattanLinuxFlavor:
        return QLatin1String("harmattan");
    case ProjectExplorer::Abi::MeegoLinuxFlavor:
        return QLatin1String("meego");
    case ProjectExplorer::Abi::GenericMacFlavor:
        return QLatin1String("generic");
    case ProjectExplorer::Abi::SymbianDeviceFlavor:
        return QLatin1String("device");
    case ProjectExplorer::Abi::SymbianEmulatorFlavor:
        return QLatin1String("emulator");
    case ProjectExplorer::Abi::GenericUnixFlavor:
        return QLatin1String("generic");
    case ProjectExplorer::Abi::SolarisUnixFlavor:
        return QLatin1String("solaris");
    case ProjectExplorer::Abi::WindowsMsvc2005Flavor:
        return QLatin1String("msvc2005");
    case ProjectExplorer::Abi::WindowsMsvc2008Flavor:
        return QLatin1String("msvc2008");
    case ProjectExplorer::Abi::WindowsMsvc2010Flavor:
        return QLatin1String("msvc2010");
    case ProjectExplorer::Abi::WindowsMSysFlavor:
        return QLatin1String("msys");
    case ProjectExplorer::Abi::WindowsCEFlavor:
        return QLatin1String("ce");
    case ProjectExplorer::Abi::UnknownFlavor: // fall through!
    default:
        return QLatin1String("unknown");
    }
}

QString Abi::toString(const BinaryFormat &bf)
{
    switch (bf) {
    case ElfFormat:
        return QLatin1String("elf");
    case PEFormat:
        return QLatin1String("pe");
    case MachOFormat:
        return QLatin1String("mach_o");
    case RuntimeQmlFormat:
        return QLatin1String("qml_rt");
    case UnknownFormat: // fall through!
    default:
        return QLatin1String("unknown");
    }
}

QString Abi::toString(int w)
{
    if (w == 0)
        return QLatin1String("unknown");
    return QString::fromLatin1("%1bit").arg(w);
}

QList<Abi::OSFlavor> Abi::flavorsForOs(const Abi::OS &o)
{
    QList<OSFlavor> result;
    switch (o) {
    case BsdOS:
        return result << FreeBsdFlavor << OpenBsdFlavor << NetBsdFlavor;
    case LinuxOS:
        return result << GenericLinuxFlavor << HarmattanLinuxFlavor << MaemoLinuxFlavor << MeegoLinuxFlavor;
    case MacOS:
        return result << GenericMacFlavor;
    case SymbianOS:
        return  result << SymbianDeviceFlavor << SymbianEmulatorFlavor;
    case UnixOS:
        return result << GenericUnixFlavor << SolarisUnixFlavor;
    case WindowsOS:
        return result << WindowsMsvc2005Flavor << WindowsMsvc2008Flavor << WindowsMsvc2010Flavor
                      << WindowsMSysFlavor << WindowsCEFlavor;
    case UnknownOS:
        return result << UnknownFlavor;
    default:
        break;
    }
    return result;
}

Abi Abi::hostAbi()
{
    Architecture arch = QTC_CPU; // define set by qmake
    OS os = UnknownOS;
    OSFlavor subos = UnknownFlavor;
    BinaryFormat format = UnknownFormat;

#if defined (Q_OS_WIN)
    os = WindowsOS;
#if _MSC_VER == 1600
    subos = WindowsMsvc2010Flavor;
#elif _MSC_VER == 1500
    subos = WindowsMsvc2008Flavor;
#elif _MSC_VER == 1400
    subos = WindowsMsvc2005Flavor;
#elif defined (mingw32)
    subos = WindowsMSysFlavor;
#endif
    format = PEFormat;
#elif defined (Q_OS_LINUX)
    os = LinuxOS;
    subos = GenericLinuxFlavor;
    format = ElfFormat;
#elif defined (Q_OS_MAC)
    os = MacOS;
    subos = GenericMacFlavor;
    format = MachOFormat;
#endif

    return Abi(arch, os, subos, format, QSysInfo::WordSize);
}

QList<Abi> Abi::abisOfBinary(const QString &path)
{
    QList<Abi> tmp;
    if (path.isEmpty())
        return tmp;

    QFile f(path);
    if (!f.exists())
        return tmp;

    f.open(QFile::ReadOnly);
    QByteArray data = f.read(1024);
    if (data.size() >= 67
            && static_cast<unsigned char>(data.at(0)) == '!' && static_cast<unsigned char>(data.at(1)) == '<'
            && static_cast<unsigned char>(data.at(2)) == 'a' && static_cast<unsigned char>(data.at(3)) == 'r'
            && static_cast<unsigned char>(data.at(4)) == 'c' && static_cast<unsigned char>(data.at(5)) == 'h'
            && static_cast<unsigned char>(data.at(6)) == '>' && static_cast<unsigned char>(data.at(7)) == 0x0a) {
        // We got an ar file: possibly a static lib for ELF, PE or Mach-O

        data = data.mid(8); // Cut of ar file magic
        quint64 offset = 8;

        while (!data.isEmpty()) {
            if ((data.at(58) != 0x60 || data.at(59) != 0x0a)) {
                qWarning() << path << ": Thought it was an ar-file, but it is not!";
                break;
            }

            const QString fileName = QString::fromLocal8Bit(data.mid(0, 16));
            quint64 fileNameOffset = 0;
            if (fileName.startsWith(QLatin1String("#1/")))
                fileNameOffset = fileName.mid(3).toInt();
            const QString fileLength = QString::fromAscii(data.mid(48, 10));

            int toSkip = 60 + fileNameOffset;
            offset += fileLength.toInt() + 60 /* header */;

            tmp.append(abiOf(data.mid(toSkip)));
            if (tmp.isEmpty() && fileName == QLatin1String("/0              "))
                tmp = parseCoffHeader(data.mid(toSkip, 20)); // This might be windws...

            if (!tmp.isEmpty()
                    && tmp.at(0).binaryFormat() != Abi::MachOFormat)
                break;

            offset += (offset % 2); // ar is 2 byte aligned
            f.seek(offset);
            data = f.read(1024);
        }
    } else {
        tmp = abiOf(data);
    }
    f.close();

    // Remove duplicates:
    QList<Abi> result;
    foreach (const Abi &a, tmp) {
        if (!result.contains(a))
            result.append(a);
    }

    return result;
}

} // namespace ProjectExplorer

// Unit tests:
#ifdef WITH_TESTS
#   include <QTest>
#   include <QtCore/QFileInfo>

#   include "projectexplorer.h"

void ProjectExplorer::ProjectExplorerPlugin::testAbiOfBinary_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QStringList>("abis");

    QTest::newRow("no file")
            << QString()
            << (QStringList());
    QTest::newRow("non existing file")
            << QString::fromLatin1("/does/not/exist")
            << (QStringList());

    // Set up prefix for test data now that we can be sure to have some tests to run:
    QString prefix = qgetenv("QTC_TEST_EXTRADATALOCATION");
    if (prefix.isEmpty())
        return;
    prefix += "/projectexplorer/abi";

    QFileInfo fi(prefix);
    if (!fi.exists() || !fi.isDir())
        return;
    prefix = fi.absoluteFilePath();

    QTest::newRow("text file")
            << QString::fromLatin1("%1/broken/text.txt").arg(prefix)
            << (QStringList());

    QTest::newRow("static QtCore: win msvc2008")
            << QString::fromLatin1("%1/static/win-msvc2008-release.lib").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-unknown-pe-32bit"));
    QTest::newRow("static QtCore: win msvc2008 II")
            << QString::fromLatin1("%1/static/win-msvc2008-release2.lib").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-unknown-pe-64bit"));
    QTest::newRow("static QtCore: win msvc2008 (debug)")
            << QString::fromLatin1("%1/static/win-msvc2008-debug.lib").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-unknown-pe-32bit"));
    QTest::newRow("static QtCore: win mingw")
            << QString::fromLatin1("%1/static/win-mingw.a").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-unknown-pe-32bit"));
    QTest::newRow("static QtCore: mac (debug)")
            << QString::fromLatin1("%1/static/mac-32bit-debug.a").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-macos-generic-mach_o-32bit"));
    QTest::newRow("static QtCore: linux 32bit")
            << QString::fromLatin1("%1/static/linux-32bit-release.a").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-linux-generic-elf-32bit"));
    QTest::newRow("static QtCore: linux 64bit")
            << QString::fromLatin1("%1/static/linux-64bit-release.a").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-linux-generic-elf-64bit"));

    QTest::newRow("static stdc++: mac fat")
            << QString::fromLatin1("%1/static/mac-fat.a").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-macos-generic-mach_o-32bit")
                              << QString::fromLatin1("ppc-macos-generic-mach_o-32bit")
                              << QString::fromLatin1("x86-macos-generic-mach_o-64bit"));

    QTest::newRow("dynamic QtCore: symbian")
            << QString::fromLatin1("%1/dynamic/symbian.dll").arg(prefix)
            << (QStringList() << QString::fromLatin1("arm-symbian-device-elf-32bit"));
    QTest::newRow("dynamic QtCore: win msvc2010 64bit")
            << QString::fromLatin1("%1/dynamic/win-msvc2010-64bit.dll").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msvc2010-pe-64bit"));
    QTest::newRow("dynamic QtCore: win msvc2008 32bit")
            << QString::fromLatin1("%1/dynamic/win-msvc2008-32bit.dll").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msvc2008-pe-32bit"));
    QTest::newRow("dynamic QtCore: win msvc2005 32bit")
            << QString::fromLatin1("%1/dynamic/win-msvc2005-32bit.dll").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msvc2005-pe-32bit"));
    QTest::newRow("dynamic QtCore: win msys 32bit")
            << QString::fromLatin1("%1/dynamic/win-mingw-32bit.dll").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msys-pe-32bit"));
    QTest::newRow("dynamic QtCore: wince msvc2005 32bit")
            << QString::fromLatin1("%1/dynamic/wince-32bit.dll").arg(prefix)
            << (QStringList() << QString::fromLatin1("mips-windows-msvc2005-pe-32bit"));
    QTest::newRow("dynamic stdc++: mac fat")
            << QString::fromLatin1("%1/dynamic/mac-fat.dylib").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-macos-generic-mach_o-32bit")
                              << QString::fromLatin1("ppc-macos-generic-mach_o-32bit")
                              << QString::fromLatin1("x86-macos-generic-mach_o-64bit"));
    QTest::newRow("dynamic QtCore: arm linux 32bit")
            << QString::fromLatin1("%1/dynamic/arm-linux.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("arm-linux-generic-elf-32bit"));
    QTest::newRow("dynamic QtCore: mips linux 32bit")
            << QString::fromLatin1("%1/dynamic/mips-linux.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("mips-linux-generic-elf-32bit"));
    QTest::newRow("dynamic QtCore: projectexplorer/abi/static/win-msvc2010-32bit.libppc be linux 32bit")
            << QString::fromLatin1("%1/dynamic/ppcbe-linux-32bit.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("ppc-linux-generic-elf-32bit"));
    QTest::newRow("dynamic QtCore: x86 freebsd 64bit")
            << QString::fromLatin1("%1/dynamic/freebsd-elf-64bit.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-bsd-freebsd-elf-64bit"));
    QTest::newRow("dynamic QtCore: x86 freebsd 64bit")
            << QString::fromLatin1("%1/dynamic/freebsd-elf-64bit.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-bsd-freebsd-elf-64bit"));
    QTest::newRow("dynamic QtCore: x86 freebsd 32bit")
            << QString::fromLatin1("%1/dynamic/freebsd-elf-32bit.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-bsd-freebsd-elf-32bit"));
}

void ProjectExplorer::ProjectExplorerPlugin::testAbiOfBinary()
{
    QFETCH(QString, file);
    QFETCH(QStringList, abis);

    QList<ProjectExplorer::Abi> result = Abi::abisOfBinary(file);
    QCOMPARE(result.count(), abis.count());
    for (int i = 0; i < abis.count(); ++i)
        QCOMPARE(result.at(i).toString(), abis.at(i));
}

void ProjectExplorer::ProjectExplorerPlugin::testFlavorForOs()
{
    QList<QList<ProjectExplorer::Abi::OSFlavor> > flavorLists;
    for (int i = 0; i != static_cast<int>(Abi::UnknownOS); ++i)
        flavorLists.append(Abi::flavorsForOs(static_cast<Abi::OS>(i)));

    int foundCounter = 0;
    for (int i = 0; i != Abi::UnknownFlavor; ++i) {
        foundCounter = 0;
        // make sure i is in exactly on of the flavor lists!
        foreach (const QList<Abi::OSFlavor> &l, flavorLists) {
            QVERIFY(!l.contains(Abi::UnknownFlavor));
            if (l.contains(static_cast<Abi::OSFlavor>(i)))
                ++foundCounter;
        }
        QCOMPARE(foundCounter, 1);
    }
}

#endif
