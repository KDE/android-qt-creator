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
#include "maemoqtversion.h"

#include "maemoconstants.h"
#include "maemoglobal.h"

#include <projectexplorer/profileinformation.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qtsupport/qtsupportconstants.h>

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QTextStream>

using namespace Qt4ProjectManager;

namespace Madde {
namespace Internal {

MaemoQtVersion::MaemoQtVersion()
    : QtSupport::BaseQtVersion(),
      m_isvalidVersion(false),
      m_initialized(false)
{

}

MaemoQtVersion::MaemoQtVersion(const Utils::FileName &path, bool isAutodetected, const QString &autodetectionSource)
    : QtSupport::BaseQtVersion(path, isAutodetected, autodetectionSource),
      m_deviceType(MaemoGlobal::deviceType(path.toString())),
      m_isvalidVersion(false),
      m_initialized(false)
{

}

MaemoQtVersion::~MaemoQtVersion()
{

}

void MaemoQtVersion::fromMap(const QVariantMap &map)
{
    QtSupport::BaseQtVersion::fromMap(map);
    QString path = qmakeCommand().toString();
    m_deviceType = MaemoGlobal::deviceType(path);
}

QString MaemoQtVersion::type() const
{
    return QtSupport::Constants::MAEMOQT;
}

bool MaemoQtVersion::isValid() const
{
    if (!BaseQtVersion::isValid())
        return false;
    if (!m_initialized) {
        m_isvalidVersion = MaemoGlobal::isValidMaemoQtVersion(qmakeCommand().toString(), m_deviceType);
        m_initialized = true;
    }
    return m_isvalidVersion;
}

MaemoQtVersion *MaemoQtVersion::clone() const
{
    return new MaemoQtVersion(*this);
}

QList<ProjectExplorer::Abi> MaemoQtVersion::detectQtAbis() const
{
    QList<ProjectExplorer::Abi> result;
    if (!isValid())
        return result;
    if (m_deviceType == Core::Id(Maemo5OsType)) {
        result.append(ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::LinuxOS,
                                           ProjectExplorer::Abi::MaemoLinuxFlavor, ProjectExplorer::Abi::ElfFormat,
                                           32));
    } else if (m_deviceType == Core::Id(HarmattanOsType)) {
        result.append(ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::LinuxOS,
                                           ProjectExplorer::Abi::HarmattanLinuxFlavor,
                                           ProjectExplorer::Abi::ElfFormat,
                                           32));
    } else if (m_deviceType == Core::Id(MeeGoOsType)) {
        result.append(ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::LinuxOS,
                                           ProjectExplorer::Abi::MeegoLinuxFlavor,
                                           ProjectExplorer::Abi::ElfFormat, 32));
    }
    return result;
}

QString MaemoQtVersion::description() const
{
    if (m_deviceType == Core::Id(Maemo5OsType))
        return QCoreApplication::translate("QtVersion", "Maemo", "Qt Version is meant for Maemo5");
    else if (m_deviceType == Core::Id(HarmattanOsType))
        return QCoreApplication::translate("QtVersion", "Harmattan ", "Qt Version is meant for Harmattan");
    else if (m_deviceType == Core::Id(MeeGoOsType))
        return QCoreApplication::translate("QtVersion", "MeeGo", "Qt Version is meant for MeeGo");
    return QString();
}

bool MaemoQtVersion::supportsShadowBuilds() const
{
#ifdef Q_OS_WIN
    return false;
#endif
    return true;
}

Core::Id MaemoQtVersion::deviceType() const
{
    return m_deviceType;
}

Core::FeatureSet MaemoQtVersion::availableFeatures() const
{
    Core::FeatureSet features = QtSupport::BaseQtVersion::availableFeatures();
    if (qtVersion() >= QtSupport::QtVersionNumber(4, 7, 4)) //no reliable test for components, yet.
        features |= Core::FeatureSet(QtSupport::Constants::FEATURE_QTQUICK_COMPONENTS_MEEGO);
    features |= Core::FeatureSet(QtSupport::Constants::FEATURE_MOBILE);

    if (deviceType() != Core::Id(Maemo5OsType)) //Only Maemo5 has proper support for Widgets
        features.remove(Core::Feature(QtSupport::Constants::FEATURE_QWIDGETS));

    return features;
}

QString MaemoQtVersion::platformName() const
{
    if (m_deviceType == Maemo5OsType)
        return QLatin1String(QtSupport::Constants::MAEMO_FREMANTLE_PLATFORM);
    if (m_deviceType == MeeGoOsType)
        return QLatin1String(QtSupport::Constants::MEEGO_PLATFORM);
    return QLatin1String(QtSupport::Constants::MEEGO_HARMATTAN_PLATFORM);
}

QString MaemoQtVersion::platformDisplayName() const
{
    if (m_deviceType == Maemo5OsType)
        return QLatin1String(QtSupport::Constants::MAEMO_FREMANTLE_PLATFORM_TR);
    if (m_deviceType == MeeGoOsType)
        return QLatin1String(QtSupport::Constants::MEEGO_PLATFORM_TR);
    return QLatin1String(QtSupport::Constants::MEEGO_HARMATTAN_PLATFORM_TR);
}

void MaemoQtVersion::addToEnvironment(const ProjectExplorer::Profile *p, Utils::Environment &env) const
{
    Q_UNUSED(p);
    const QString maddeRoot = MaemoGlobal::maddeRoot(qmakeCommand().toString());

    // Needed to make pkg-config stuff work.
    Utils::FileName sysRoot = ProjectExplorer::SysRootProfileInformation::sysRoot(p);
    env.prependOrSet(QLatin1String("SYSROOT_DIR"), sysRoot.toUserOutput());
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/madbin")
        .arg(maddeRoot)));
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/madlib")
        .arg(maddeRoot)));
    env.prependOrSet(QLatin1String("PERL5LIB"),
        QDir::toNativeSeparators(QString("%1/madlib/perl5").arg(maddeRoot)));

    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/bin").arg(maddeRoot)));
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/bin")
        .arg(MaemoGlobal::targetRoot(qmakeCommand().toString()))));

    // Actually this is tool chain related, but since we no longer have a tool chain...
    const QString manglePathsKey = QLatin1String("GCCWRAPPER_PATHMANGLE");
    if (!env.hasKey(manglePathsKey)) {
        const QStringList pathsToMangle = QStringList() << QLatin1String("/lib")
            << QLatin1String("/opt") << QLatin1String("/usr");
        env.set(manglePathsKey, pathsToMangle.join(QLatin1String(":")));
    }
}

} // namespace Internal
} // namespace Madde
