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
#ifndef ABSTRACTUPLOADANDINSTALLACTION_H
#define ABSTRACTUPLOADANDINSTALLACTION_H

#include "abstractremotelinuxdeployservice.h"
#include "remotelinux_export.h"

namespace RemoteLinux {
class AbstractRemoteLinuxPackageInstaller;

namespace Internal {
class AbstractUploadAndInstallPackageServicePrivate;
}

class REMOTELINUX_EXPORT AbstractUploadAndInstallPackageService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT

public:
    void setPackageFilePath(const QString &filePath);

protected:
    explicit AbstractUploadAndInstallPackageService(QObject *parent);
    ~AbstractUploadAndInstallPackageService();

    QString packageFilePath() const;

private slots:
    void handleUploadFinished(const QString &errorMsg);
    void handleInstallationFinished(const QString &errorMsg);

private:
    virtual AbstractRemoteLinuxPackageInstaller *packageInstaller() const = 0;
    virtual QString uploadDir() const; // Defaults to remote user's home directory.

    bool isDeploymentNecessary() const;
    void doDeviceSetup();
    void stopDeviceSetup();
    void doDeploy();
    void stopDeployment();

    void setFinished();

    Internal::AbstractUploadAndInstallPackageServicePrivate * const d;
};

} // namespace RemoteLinux

#endif // ABSTRACTUPLOADANDINSTALLACTION_H
