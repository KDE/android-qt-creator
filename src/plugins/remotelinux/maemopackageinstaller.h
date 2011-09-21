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

#ifndef ABSTRACTMAEMOPACKAGEINSTALLER_H
#define ABSTRACTMAEMOPACKAGEINSTALLER_H

#include <remotelinux/remotelinuxpackageinstaller.h>

namespace RemoteLinux {
namespace Internal {

class MaemoDebianPackageInstaller: public RemoteLinux::AbstractRemoteLinuxPackageInstaller
{
    Q_OBJECT
public:
    explicit MaemoDebianPackageInstaller(QObject *parent);

private slots:
    void handleInstallerErrorOutput(const QString &output);

private:
    void prepareInstallation();
    QString errorString() const;
    QString installCommandLine(const QString &packageFilePath) const;
    QString cancelInstallationCommandLine() const;

    QString m_installerStderr;
};


class MaemoRpmPackageInstaller : public RemoteLinux::AbstractRemoteLinuxPackageInstaller
{
    Q_OBJECT
public:
    MaemoRpmPackageInstaller(QObject *parent);

private:
    QString installCommandLine(const QString &packageFilePath) const;
    QString cancelInstallationCommandLine() const;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // ABSTRACTMAEMOPACKAGEINSTALLER_H
