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

#ifndef MAEMOPACKAGEUPLOADER_H
#define MAEMOPACKAGEUPLOADER_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

#include <utils/ssh/sftpdefs.h>

namespace Utils {
class SftpChannel;
class SshConnection;
}

namespace RemoteLinux {
namespace Internal {

class MaemoPackageUploader : public QObject
{
    Q_OBJECT
public:
    explicit MaemoPackageUploader(QObject *parent = 0);
    ~MaemoPackageUploader();

    // Connection has to be established already.
    void uploadPackage(const QSharedPointer<Utils::SshConnection> &connection,
        const QString &localFilePath, const QString &remoteFilePath);
    void cancelUpload();

signals:
    void progress(const QString &message);
    void uploadFinished(const QString &errorMsg = QString());

private slots:
    void handleConnectionFailure();
    void handleSftpChannelInitialized();
    void handleSftpChannelInitializationFailed(const QString &error);
    void handleSftpJobFinished(Utils::SftpJobId job, const QString &error);

private:
    enum State { InitializingSftp, Uploading, Inactive };

    void cleanup();
    void setState(State newState);

    State m_state;
    QSharedPointer<Utils::SshConnection> m_connection;
    QSharedPointer<Utils::SftpChannel> m_uploader;
    QString m_localFilePath;
    QString m_remoteFilePath;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMOPACKAGEUPLOADER_H
