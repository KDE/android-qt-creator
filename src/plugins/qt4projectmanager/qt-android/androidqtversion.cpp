/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidqtversion.h"
#include "qt4projectmanagerconstants.h"

#include <qtsupport/qtsupportconstants.h>

#include <QtCore/QCoreApplication>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

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
    return QtSupport::Constants::ANDROIDQT;
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
    }
    return m_qtAbis;
}

bool AndroidQtVersion::supportsTargetId(const QString &id) const
{
    return id == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID);
}

QSet<QString> AndroidQtVersion::supportedTargetIds() const
{
    return QSet<QString>() << QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID);
}

QString AndroidQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion", "Android", "Qt Version is meant for Android");
}

