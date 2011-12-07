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
#include "maemoremotecopyfacility.h"

#include "maemoglobal.h"

#include <remotelinux/linuxdeviceconfiguration.h>
#include <utils/ssh/sshconnection.h>
#include <utils/ssh/sshremoteprocessrunner.h>

#include <QtCore/QDir>

using namespace RemoteLinux;
using namespace Utils;

namespace Madde {
namespace Internal {

MaemoRemoteCopyFacility::MaemoRemoteCopyFacility(QObject *parent) :
    QObject(parent), m_copyRunner(0), m_killProcess(0), m_isCopying(false)
{
}

MaemoRemoteCopyFacility::~MaemoRemoteCopyFacility() {}

void MaemoRemoteCopyFacility::copyFiles(const SshConnection::Ptr &connection,
    const LinuxDeviceConfiguration::ConstPtr &devConf,
    const QList<DeployableFile> &deployables, const QString &mountPoint)
{
    Q_ASSERT(connection->state() == SshConnection::Connected);
    Q_ASSERT(!m_isCopying);

    m_devConf = devConf;
    m_deployables = deployables;
    m_mountPoint = mountPoint;

    if (!m_copyRunner)
        m_copyRunner = new SshRemoteProcessRunner(this);
    connect(m_copyRunner, SIGNAL(connectionError()), SLOT(handleConnectionError()));
    connect(m_copyRunner, SIGNAL(processOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdout(QByteArray)));
    connect(m_copyRunner, SIGNAL(processErrorOutputAvailable(QByteArray)),
        SLOT(handleRemoteStderr(QByteArray)));
    connect(m_copyRunner, SIGNAL(processClosed(int)), SLOT(handleCopyFinished(int)));

    m_isCopying = true;
    copyNextFile();
}

void MaemoRemoteCopyFacility::cancel()
{
    Q_ASSERT(m_isCopying);

    if (!m_killProcess)
        m_killProcess = new SshRemoteProcessRunner(this);
    m_killProcess->run("pkill cp", m_devConf->sshParameters());
    setFinished();
}

void MaemoRemoteCopyFacility::handleConnectionError()
{
    setFinished();
    emit finished(tr("Connection failed: %1").arg(m_copyRunner->lastConnectionErrorString()));
}

void MaemoRemoteCopyFacility::handleRemoteStdout(const QByteArray &output)
{
    emit stdoutData(QString::fromUtf8(output));
}

void MaemoRemoteCopyFacility::handleRemoteStderr(const QByteArray &output)
{
    emit stderrData(QString::fromUtf8(output));
}

void MaemoRemoteCopyFacility::handleCopyFinished(int exitStatus)
{
    if (!m_isCopying)
        return;

    if (exitStatus != SshRemoteProcess::ExitedNormally
            || m_copyRunner->processExitCode() != 0) {
        setFinished();
        emit finished(tr("Error: Copy command failed."));
    } else {
        emit fileCopied(m_deployables.takeFirst());
        copyNextFile();
    }
}

void MaemoRemoteCopyFacility::copyNextFile()
{
    Q_ASSERT(m_isCopying);

    if (m_deployables.isEmpty()) {
        setFinished();
        emit finished();
        return;
    }

    const DeployableFile &d = m_deployables.first();
    QString sourceFilePath = m_mountPoint;
#ifdef Q_OS_WIN
    const QString localFilePath = QDir::fromNativeSeparators(d.localFilePath);
    sourceFilePath += QLatin1Char('/') + localFilePath.at(0).toLower()
        + localFilePath.mid(2);
#else
    sourceFilePath += d.localFilePath;
#endif

    QString command = QString::fromLatin1("%1 mkdir -p %3 && %1 cp -a %2 %3")
        .arg(MaemoGlobal::remoteSudo(m_devConf->osType(), m_devConf->sshParameters().userName),
            sourceFilePath, d.remoteDir);
    emit progress(tr("Copying file '%1' to directory '%2' on the device...")
        .arg(d.localFilePath, d.remoteDir));
    m_copyRunner->run(command.toUtf8(), m_devConf->sshParameters());
}

void MaemoRemoteCopyFacility::setFinished()
{
    disconnect(m_copyRunner, 0, this, 0);
    m_deployables.clear();
    m_isCopying = false;
}

} // namespace Internal
} // namespace Madde
