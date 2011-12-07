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

#ifndef MAEMOREMOTECOPYFACILITY_H
#define MAEMOREMOTECOPYFACILITY_H

#include <remotelinux/deployablefile.h>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

namespace Utils {
class SshConnection;
class SshRemoteProcessRunner;
}

namespace RemoteLinux {
class LinuxDeviceConfiguration;
}

namespace Madde {
namespace Internal {

class MaemoRemoteCopyFacility : public QObject
{
    Q_OBJECT
public:
    explicit MaemoRemoteCopyFacility(QObject *parent = 0);
    ~MaemoRemoteCopyFacility();

    void copyFiles(const QSharedPointer<Utils::SshConnection> &connection,
        const QSharedPointer<const RemoteLinux::LinuxDeviceConfiguration> &devConf,
        const QList<RemoteLinux::DeployableFile> &deployables, const QString &mountPoint);
    void cancel();

signals:
    void stdoutData(const QString &output);
    void stderrData(const QString &output);
    void progress(const QString &message);
    void fileCopied(const RemoteLinux::DeployableFile &deployable);
    void finished(const QString &errorMsg = QString());

private slots:
    void handleConnectionError();
    void handleCopyFinished(int exitStatus);
    void handleRemoteStdout(const QByteArray &output);
    void handleRemoteStderr(const QByteArray &output);

private:
    void copyNextFile();
    void setFinished();

    Utils::SshRemoteProcessRunner *m_copyRunner;
    Utils::SshRemoteProcessRunner *m_killProcess;
    QSharedPointer<const RemoteLinux::LinuxDeviceConfiguration> m_devConf;
    QList<RemoteLinux::DeployableFile> m_deployables;
    QString m_mountPoint;
    bool m_isCopying; // TODO: Redundant due to being in sync with m_copyRunner?
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOREMOTECOPYFACILITY_H
