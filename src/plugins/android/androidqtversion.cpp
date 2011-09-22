/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidqtversion.h"
#include "androidconstants.h"
#include "qt4projectmanager/qt4projectmanagerconstants.h"

#include <qtsupport/qtsupportconstants.h>

#include <QtCore/QCoreApplication>

using namespace Android::Internal;

AndroidQtVersion::AndroidQtVersion()
    : QtSupport::BaseQtVersion(),
      m_qtAbisUpToDate(false)
{

}

AndroidQtVersion::AndroidQtVersion(const QString &path, bool isAutodetected, const QString &autodetectionSource)
    : QtSupport::BaseQtVersion(path, isAutodetected, autodetectionSource),
      m_qtAbisUpToDate(false)
{
}

AndroidQtVersion::~AndroidQtVersion()
{

}

AndroidQtVersion *AndroidQtVersion::clone() const
{
    return new AndroidQtVersion(*this);
}

QString AndroidQtVersion::type() const
{
    return Constants::ANDROIDQT;
}

bool AndroidQtVersion::isValid() const
{
    if (!BaseQtVersion::isValid())
        return false;
    if (qtAbis().isEmpty())
        return false;
    return true;
}

QString AndroidQtVersion::invalidReason() const
{
    QString tmp = BaseQtVersion::invalidReason();
    if (tmp.isEmpty() && qtAbis().isEmpty())
        return QCoreApplication::translate("QtVersion", "Failed to detect the ABI(s) used by the Qt version.");
    return tmp;
}

QList<ProjectExplorer::Abi> AndroidQtVersion::qtAbis() const
{
    if (!m_qtAbisUpToDate) {
        m_qtAbisUpToDate = true;
        ensureMkSpecParsed();
        m_qtAbis = qtAbisFromLibrary(qtCorePath(versionInfo(), qtVersionString()));
        for (int i = 0; i < m_qtAbis.size(); i++)
            m_qtAbis[i]=ProjectExplorer::Abi(m_qtAbis[i].architecture(),ProjectExplorer::Abi::LinuxOS,
                                                                   ProjectExplorer::Abi::AndroidLinuxFlavor,
                                                                   ProjectExplorer::Abi::ElfFormat,
                                                                   32);
    }
    return m_qtAbis;
}

QList<ProjectExplorer::Abi> AndroidQtVersion::detectQtAbis() const
{
    return QList<ProjectExplorer::Abi>()
            << ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::LinuxOS,
                                    ProjectExplorer::Abi::AndroidLinuxFlavor,
                                    ProjectExplorer::Abi::ElfFormat,
                                    32);
}

bool AndroidQtVersion::supportsTargetId(const QString &id) const
{
    return id == QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID);
}

QSet<QString> AndroidQtVersion::supportedTargetIds() const
{
    return QSet<QString>() << QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID);
}

QString AndroidQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion", "Android", "Qt Version is meant for Android");
}

