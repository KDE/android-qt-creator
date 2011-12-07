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

#include "embeddedlinuxqtversionfactory.h"

#include "embeddedlinuxqtversion.h"
#include "remotelinux_constants.h"

#include <QtCore/QFileInfo>

namespace RemoteLinux {
namespace Internal {

EmbeddedLinuxQtVersionFactory::EmbeddedLinuxQtVersionFactory(QObject *parent) : QtSupport::QtVersionFactory(parent)
{ }

EmbeddedLinuxQtVersionFactory::~EmbeddedLinuxQtVersionFactory()
{ }

bool EmbeddedLinuxQtVersionFactory::canRestore(const QString &type)
{
    return type == QLatin1String(RemoteLinux::Constants::EMBEDDED_LINUX_QT);
}

QtSupport::BaseQtVersion *EmbeddedLinuxQtVersionFactory::restore(const QString &type, const QVariantMap &data)
{
    if (!canRestore(type))
        return 0;
    EmbeddedLinuxQtVersion *v = new EmbeddedLinuxQtVersion;
    v->fromMap(data);
    return v;
}

int EmbeddedLinuxQtVersionFactory::priority() const
{
    return 10;
}

QtSupport::BaseQtVersion *EmbeddedLinuxQtVersionFactory::create(const Utils::FileName &qmakePath,
                                                                ProFileEvaluator *evaluator,
                                                                bool isAutoDetected,
                                                                const QString &autoDetectionSource)
{
    Q_UNUSED(evaluator);

    QFileInfo fi = qmakePath.toFileInfo();
    if (!fi.exists() || !fi.isExecutable() || !fi.isFile())
        return 0;

    EmbeddedLinuxQtVersion *version = new EmbeddedLinuxQtVersion(qmakePath, isAutoDetected, autoDetectionSource);

    QList<ProjectExplorer::Abi> abis = version->qtAbis();
    // Note: This fails for e.g. intel/meego cross builds on x86 linux machines.
    if (abis.count() == 1
            && abis.at(0).os() == ProjectExplorer::Abi::LinuxOS
            && !ProjectExplorer::Abi::hostAbi().isCompatibleWith(abis.at(0)))
        return version;

    delete version;
    return 0;
}

} // namespace Internal
} // namespace RemoteLinux
