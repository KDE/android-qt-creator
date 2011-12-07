/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "embeddedlinuxqtversion.h"

#include "remotelinux_constants.h"

#include <QtCore/QCoreApplication>

namespace RemoteLinux {
namespace Internal {

EmbeddedLinuxQtVersion::EmbeddedLinuxQtVersion()
    : BaseQtVersion()
{ }

EmbeddedLinuxQtVersion::EmbeddedLinuxQtVersion(const Utils::FileName &path, bool isAutodetected, const QString &autodetectionSource)
    : BaseQtVersion(path, isAutodetected, autodetectionSource)
{ }

EmbeddedLinuxQtVersion::~EmbeddedLinuxQtVersion()
{ }

EmbeddedLinuxQtVersion *EmbeddedLinuxQtVersion::clone() const
{
    return new EmbeddedLinuxQtVersion(*this);
}

QString EmbeddedLinuxQtVersion::type() const
{
    return RemoteLinux::Constants::EMBEDDED_LINUX_QT;
}

QString EmbeddedLinuxQtVersion::warningReason() const
{
    if (qtAbis().count() == 1 && qtAbis().first().isNull())
        return QCoreApplication::translate("QtVersion", "ABI detection failed: Make sure to use a matching tool chain when building.");
    return QString();
}

QList<ProjectExplorer::Abi> EmbeddedLinuxQtVersion::detectQtAbis() const
{
    return qtAbisFromLibrary(qtCorePath(versionInfo(), qtVersionString()));
}

bool EmbeddedLinuxQtVersion::supportsTargetId(const QString &id) const
{
    return id == QLatin1String(Constants::EMBEDDED_LINUX_TARGET_ID);
}

QSet<QString> EmbeddedLinuxQtVersion::supportedTargetIds() const
{
    return QSet<QString>() << QLatin1String(Constants::EMBEDDED_LINUX_TARGET_ID);
}

QString EmbeddedLinuxQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion", "Embedded Linux", "Qt Version is used for embedded Linux development");
}

} // namespace Internal
} // namespace RemoteLinux
