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

#ifndef DEPLOYABLEFILE_H
#define DEPLOYABLEFILE_H

#include "remotelinux_export.h"

#include <QtCore/QHash>
#include <QtCore/QString>

namespace RemoteLinux {

class REMOTELINUX_EXPORT DeployableFile
{
public:
    DeployableFile() {}

    DeployableFile(const QString &localFilePath, const QString &remoteDir)
        : localFilePath(localFilePath), remoteDir(remoteDir) {}

    bool operator==(const DeployableFile &other) const
    {
        return localFilePath == other.localFilePath
            && remoteDir == other.remoteDir;
    }

    QString localFilePath;
    QString remoteDir;
};

inline uint qHash(const DeployableFile &d)
{
    return qHash(qMakePair(d.localFilePath, d.remoteDir));
}

} // namespace RemoteLinux

#endif // DEPLOYABLEFILE_H
