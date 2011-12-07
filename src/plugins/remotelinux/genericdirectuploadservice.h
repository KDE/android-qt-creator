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
#ifndef DIRECTDEVICEUPLOADACTION_H
#define DIRECTDEVICEUPLOADACTION_H

#include "abstractremotelinuxdeployservice.h"
#include "remotelinux_export.h"

#include <utils/ssh/sftpdefs.h>

#include <QtCore/QList>

QT_FORWARD_DECLARE_CLASS(QString)

namespace RemoteLinux {
class DeployableFile;
namespace Internal { class GenericDirectUploadServicePrivate; }

class REMOTELINUX_EXPORT GenericDirectUploadService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT
public:
    GenericDirectUploadService(QObject *parent = 0);

    void setDeployableFiles(const QList<DeployableFile> &deployableFiles);
    void setIncrementalDeployment(bool incremental);

  protected:
    bool isDeploymentNecessary() const;

    void doDeviceSetup();
    void stopDeviceSetup();

    void doDeploy();
    void stopDeployment();

private slots:
    void handleSftpInitialized();
    void handleSftpInitializationFailed(const QString &errorMessage);
    void handleUploadFinished(Utils::SftpJobId jobId, const QString &errorMsg);
    void handleMkdirFinished(int exitStatus);
    void handleLnFinished(int exitStatus);
    void handleStdOutData();
    void handleStdErrData();

private:
    void checkDeploymentNeeded(const DeployableFile &file) const;
    void setFinished();
    void uploadNextFile();

    Internal::GenericDirectUploadServicePrivate * const d;
};

} //namespace RemoteLinux

#endif // DIRECTDEVICEUPLOADACTION_H
