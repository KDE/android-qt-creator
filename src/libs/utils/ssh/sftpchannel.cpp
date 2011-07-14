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

#include "sftpchannel.h"
#include "sftpchannel_p.h"

#include "sshexception_p.h"
#include "sshincomingpacket_p.h"
#include "sshsendfacility_p.h"

#include <QtCore/QDir>
#include <QtCore/QFile>

/*!
    \class Utils::SftpChannel

    \brief This class provides SFTP operations.

    Objects are created via SshConnection::createSftpChannel().
    The channel needs to be initialized with
    a call to initialize() and is closed via closeChannel(). After closing
    a channel, no more operations are possible. It cannot be re-opened
    using initialize(); use SshConnection::createSftpChannel() if you need
    a new one.

    After the initialized() signal has been emitted, operations can be started.
    All SFTP operations are asynchronous (non-blocking) and can be in-flight
    simultaneously (though callers must ensure that concurrently running jobs
    are independent of each other, e.g. they must not write to the same file).
    Operations are identified by their job id, which is returned by
    the respective member function. If the function can right away detect that
    the operation cannot succeed, it returns SftpInvalidJob. If an error occurs
    later, the finished() signal is emitted for the respective job with a
    non-empty error string.

    Note that directory names must not have a trailing slash.
*/

namespace Utils {
namespace Internal {
namespace {
    const quint32 ProtocolVersion = 3;

    QString errorMessage(const QString &serverMessage,
        const QString &alternativeMessage)
    {
        return serverMessage.isEmpty() ? alternativeMessage : serverMessage;
    }

    QString errorMessage(const SftpStatusResponse &response,
        const QString &alternativeMessage)
    {
        return response.status == SSH_FX_OK ? QString()
            : errorMessage(response.errorString, alternativeMessage);
    }
} // anonymous namespace
} // namespace Internal

SftpChannel::SftpChannel(quint32 channelId,
    Internal::SshSendFacility &sendFacility)
    : d(new Internal::SftpChannelPrivate(channelId, sendFacility, this))
{
    connect(d, SIGNAL(initialized()), this, SIGNAL(initialized()),
        Qt::QueuedConnection);
    connect(d, SIGNAL(initializationFailed(QString)), this,
        SIGNAL(initializationFailed(QString)), Qt::QueuedConnection);
    connect(d, SIGNAL(dataAvailable(Utils::SftpJobId, QString)), this,
        SIGNAL(dataAvailable(Utils::SftpJobId, QString)), Qt::QueuedConnection);
    connect(d, SIGNAL(finished(Utils::SftpJobId,QString)), this,
        SIGNAL(finished(Utils::SftpJobId,QString)), Qt::QueuedConnection);
    connect(d, SIGNAL(closed()), this, SIGNAL(closed()), Qt::QueuedConnection);
}

SftpChannel::State SftpChannel::state() const
{
    switch (d->channelState()) {
    case Internal::AbstractSshChannel::Inactive:
        return Uninitialized;
    case Internal::AbstractSshChannel::SessionRequested:
        return Initializing;
    case Internal::AbstractSshChannel::CloseRequested:
        return Closing;
    case Internal::AbstractSshChannel::Closed:
        return Closed;
    case Internal::AbstractSshChannel::SessionEstablished:
        return d->m_sftpState == Internal::SftpChannelPrivate::Initialized
            ? Initialized : Initializing;
    default:
        Q_ASSERT(!"Oh no, we forgot to handle a channel state!");
        return Closed; // For the compiler.
    }
}

void SftpChannel::initialize()
{
    d->requestSessionStart();
    d->m_sftpState = Internal::SftpChannelPrivate::SubsystemRequested;
}

void SftpChannel::closeChannel()
{
    d->closeChannel();
}

SftpJobId SftpChannel::listDirectory(const QString &path)
{
    return d->createJob(Internal::SftpListDir::Ptr(
        new Internal::SftpListDir(++d->m_nextJobId, path)));
}

SftpJobId SftpChannel::createDirectory(const QString &path)
{
    return d->createJob(Internal::SftpMakeDir::Ptr(
        new Internal::SftpMakeDir(++d->m_nextJobId, path)));
}

SftpJobId SftpChannel::removeDirectory(const QString &path)
{
    return d->createJob(Internal::SftpRmDir::Ptr(
        new Internal::SftpRmDir(++d->m_nextJobId, path)));
}

SftpJobId SftpChannel::removeFile(const QString &path)
{
    return d->createJob(Internal::SftpRm::Ptr(
        new Internal::SftpRm(++d->m_nextJobId, path)));
}

SftpJobId SftpChannel::renameFileOrDirectory(const QString &oldPath,
    const QString &newPath)
{
    return d->createJob(Internal::SftpRename::Ptr(
        new Internal::SftpRename(++d->m_nextJobId, oldPath, newPath)));
}

SftpJobId SftpChannel::createFile(const QString &path, SftpOverwriteMode mode)
{
    return d->createJob(Internal::SftpCreateFile::Ptr(
        new Internal::SftpCreateFile(++d->m_nextJobId, path, mode)));
}

SftpJobId SftpChannel::uploadFile(const QString &localFilePath,
    const QString &remoteFilePath, SftpOverwriteMode mode)
{
    QSharedPointer<QFile> localFile(new QFile(localFilePath));
    if (!localFile->open(QIODevice::ReadOnly))
        return SftpInvalidJob;
    return d->createJob(Internal::SftpUploadFile::Ptr(
        new Internal::SftpUploadFile(++d->m_nextJobId, remoteFilePath, localFile, mode)));
}

SftpJobId SftpChannel::downloadFile(const QString &remoteFilePath,
    const QString &localFilePath, SftpOverwriteMode mode)
{
    QSharedPointer<QFile> localFile(new QFile(localFilePath));
    if (mode == SftpSkipExisting && localFile->exists())
        return SftpInvalidJob;
    QIODevice::OpenMode openMode = QIODevice::WriteOnly;
    if (mode == SftpOverwriteExisting)
        openMode |= QIODevice::Truncate;
    else if (mode == SftpAppendToExisting)
        openMode |= QIODevice::Append;
    if (!localFile->open(openMode))
        return SftpInvalidJob;
    return d->createJob(Internal::SftpDownload::Ptr(
        new Internal::SftpDownload(++d->m_nextJobId, remoteFilePath, localFile)));
}

SftpJobId SftpChannel::uploadDir(const QString &localDirPath,
    const QString &remoteParentDirPath)
{
    if (state() != Initialized)
        return SftpInvalidJob;
    const QDir localDir(localDirPath);
    if (!localDir.exists() || !localDir.isReadable())
        return SftpInvalidJob;
    const Internal::SftpUploadDir::Ptr uploadDirOp(
        new Internal::SftpUploadDir(++d->m_nextJobId));
    const QString remoteDirPath
        = remoteParentDirPath + QLatin1Char('/') + localDir.dirName();
    const Internal::SftpMakeDir::Ptr mkdirOp(
        new Internal::SftpMakeDir(++d->m_nextJobId, remoteDirPath, uploadDirOp));
    uploadDirOp->mkdirsInProgress.insert(mkdirOp,
        Internal::SftpUploadDir::Dir(localDirPath, remoteDirPath));
    d->createJob(mkdirOp);
    return uploadDirOp->jobId;
}

SftpChannel::~SftpChannel()
{
    delete d;
}


namespace Internal {

SftpChannelPrivate::SftpChannelPrivate(quint32 channelId,
    SshSendFacility &sendFacility, SftpChannel *sftp)
    : AbstractSshChannel(channelId, sendFacility),
      m_nextJobId(0), m_sftpState(Inactive), m_sftp(sftp)
{
}

SftpJobId SftpChannelPrivate::createJob(const AbstractSftpOperation::Ptr &job)
{
   if (m_sftp->state() != SftpChannel::Initialized)
       return SftpInvalidJob;
   m_jobs.insert(job->jobId, job);
   sendData(job->initialPacket(m_outgoingPacket).rawData());
   return job->jobId;
}

void SftpChannelPrivate::handleChannelSuccess()
{
    if (channelState() == CloseRequested)
        return;
#ifdef CREATOR_SSH_DEBUG
    qDebug("sftp subsystem initialized");
#endif
    sendData(m_outgoingPacket.generateInit(ProtocolVersion).rawData());
    m_sftpState = InitSent;
}

void SftpChannelPrivate::handleChannelFailure()
{
    if (channelState() == CloseRequested)
        return;

    if (m_sftpState != SubsystemRequested) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_MSG_CHANNEL_FAILURE packet.");
    }
    emit initializationFailed(tr("Server could not start sftp subsystem."));
    closeChannel();
}

void SftpChannelPrivate::handleChannelDataInternal(const QByteArray &data)
{
    if (channelState() == CloseRequested)
        return;

    m_incomingData += data;
    m_incomingPacket.consumeData(m_incomingData);
    while (m_incomingPacket.isComplete()) {
        handleCurrentPacket();
        m_incomingPacket.clear();
        m_incomingPacket.consumeData(m_incomingData);
    }
}

void SftpChannelPrivate::handleChannelExtendedDataInternal(quint32 type,
    const QByteArray &data)
{
    qWarning("Unexpected extended data '%s' of type %d on SFTP channel.",
        data.data(), type);
}

void SftpChannelPrivate::handleExitStatus(const SshChannelExitStatus &exitStatus)
{
    const char * const message = "Remote SFTP service exited with exit code %d";
#ifdef CREATOR_SSH_DEBUG
    qDebug(message, exitStatus.exitStatus);
#else
    if (exitStatus.exitStatus != 0)
        qWarning(message, exitStatus.exitStatus);
#endif
}

void SftpChannelPrivate::handleExitSignal(const SshChannelExitSignal &signal)
{
    qWarning("Remote SFTP service killed; signal was %s", signal.signal.data());
}

void SftpChannelPrivate::handleCurrentPacket()
{
#ifdef CREATOR_SSH_DEBUG
    qDebug("Handling SFTP packet of type %d", m_incomingPacket.type());
#endif
    switch (m_incomingPacket.type()) {
    case SSH_FXP_VERSION:
        handleServerVersion();
        break;
    case SSH_FXP_HANDLE:
        handleHandle();
        break;
    case SSH_FXP_NAME:
        handleName();
        break;
    case SSH_FXP_STATUS:
        handleStatus();
        break;
    case SSH_FXP_DATA:
        handleReadData();
        break;
    case SSH_FXP_ATTRS:
        handleAttrs();
        break;
    default:
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected packet.",
            tr("Unexpected packet of type %1.").arg(m_incomingPacket.type()));
    }
}

void SftpChannelPrivate::handleServerVersion()
{
    checkChannelActive();
    if (m_sftpState != InitSent) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_VERSION packet.");
    }

#ifdef CREATOR_SSH_DEBUG
    qDebug("sftp init received");
#endif
    const quint32 serverVersion = m_incomingPacket.extractServerVersion();
    if (serverVersion != ProtocolVersion) {
        emit initializationFailed(tr("Protocol version mismatch: Expected %1, got %2")
            .arg(serverVersion).arg(ProtocolVersion));
        closeChannel();
    } else {
        m_sftpState = Initialized;
        emit initialized();
    }
}

void SftpChannelPrivate::handleHandle()
{
    const SftpHandleResponse &response = m_incomingPacket.asHandleResponse();
    JobMap::Iterator it = lookupJob(response.requestId);
    const QSharedPointer<AbstractSftpOperationWithHandle> job
        = it.value().dynamicCast<AbstractSftpOperationWithHandle>();
    if (job.isNull()) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_HANDLE packet.");
    }
    if (job->state != AbstractSftpOperationWithHandle::OpenRequested) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_HANDLE packet.");
    }
    job->remoteHandle = response.handle;
    job->state = AbstractSftpOperationWithHandle::Open;

    switch (it.value()->type()) {
    case AbstractSftpOperation::ListDir:
        handleLsHandle(it);
        break;
    case AbstractSftpOperation::CreateFile:
        handleCreateFileHandle(it);
        break;
    case AbstractSftpOperation::Download:
        handleGetHandle(it);
        break;
    case AbstractSftpOperation::UploadFile:
        handlePutHandle(it);
        break;
    default:
        Q_ASSERT(!"Oh no, I forgot to handle an SFTP operation type!");
    }
}

void SftpChannelPrivate::handleLsHandle(const JobMap::Iterator &it)
{
    SftpListDir::Ptr op = it.value().staticCast<SftpListDir>();
    sendData(m_outgoingPacket.generateReadDir(op->remoteHandle,
        op->jobId).rawData());
}

void SftpChannelPrivate::handleCreateFileHandle(const JobMap::Iterator &it)
{
    SftpCreateFile::Ptr op = it.value().staticCast<SftpCreateFile>();
    sendData(m_outgoingPacket.generateCloseHandle(op->remoteHandle,
        op->jobId).rawData());
}

void SftpChannelPrivate::handleGetHandle(const JobMap::Iterator &it)
{
    SftpDownload::Ptr op = it.value().staticCast<SftpDownload>();
    sendData(m_outgoingPacket.generateFstat(op->remoteHandle,
        op->jobId).rawData());
    op->statRequested = true;
}

void SftpChannelPrivate::handlePutHandle(const JobMap::Iterator &it)
{
    SftpUploadFile::Ptr op = it.value().staticCast<SftpUploadFile>();
    if (op->parentJob && op->parentJob->hasError)
        sendTransferCloseHandle(op, it.key());

    // OpenSSH does not implement the RFC's append functionality, so we
    // have to emulate it.
    if (op->mode == SftpAppendToExisting) {
        sendData(m_outgoingPacket.generateFstat(op->remoteHandle,
            op->jobId).rawData());
        op->statRequested = true;
    } else {
        spawnWriteRequests(it);
    }
}

void SftpChannelPrivate::handleStatus()
{
    const SftpStatusResponse &response = m_incomingPacket.asStatusResponse();
#ifdef CREATOR_SSH_DEBUG
    qDebug("%s: status = %d", Q_FUNC_INFO, response.status);
#endif
    JobMap::Iterator it = lookupJob(response.requestId);
    switch (it.value()->type()) {
    case AbstractSftpOperation::ListDir:
        handleLsStatus(it, response);
        break;
    case AbstractSftpOperation::Download:
        handleGetStatus(it, response);
        break;
    case AbstractSftpOperation::UploadFile:
        handlePutStatus(it, response);
        break;
    case AbstractSftpOperation::MakeDir:
        handleMkdirStatus(it, response);
        break;
    case AbstractSftpOperation::RmDir:
    case AbstractSftpOperation::Rm:
    case AbstractSftpOperation::Rename:
    case AbstractSftpOperation::CreateFile:
        handleStatusGeneric(it, response);
        break;
    }
}

void SftpChannelPrivate::handleStatusGeneric(const JobMap::Iterator &it,
    const SftpStatusResponse &response)
{
    AbstractSftpOperation::Ptr op = it.value();
    const QString error = errorMessage(response, tr("Unknown error."));
    emit finished(op->jobId, error);
    m_jobs.erase(it);
}

void SftpChannelPrivate::handleMkdirStatus(const JobMap::Iterator &it,
    const SftpStatusResponse &response)
{
    SftpMakeDir::Ptr op = it.value().staticCast<SftpMakeDir>();
    if (op->parentJob == SftpUploadDir::Ptr()) {
        handleStatusGeneric(it, response);
        return;
    }
    if (op->parentJob->hasError) {
        m_jobs.erase(it);
        return;
    }

    typedef QMap<SftpMakeDir::Ptr, SftpUploadDir::Dir>::Iterator DirIt;
    DirIt dirIt = op->parentJob->mkdirsInProgress.find(op);
    Q_ASSERT(dirIt != op->parentJob->mkdirsInProgress.end());
    const QString &remoteDir = dirIt.value().remoteDir;
    if (response.status == SSH_FX_OK) {
        emit dataAvailable(op->parentJob->jobId,
            tr("Created remote directory '%1'.").arg(remoteDir));
    } else if (response.status == SSH_FX_FAILURE) {
        emit dataAvailable(op->parentJob->jobId,
            tr("Remote directory '%1' already exists.").arg(remoteDir));
    } else {
        op->parentJob->setError();
        emit finished(op->parentJob->jobId,
            tr("Error creating directory '%1': %2")
            .arg(remoteDir, response.errorString));
        m_jobs.erase(it);
        return;
    }

    QDir localDir(dirIt.value().localDir);
    const QFileInfoList &dirInfos
        = localDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QFileInfo &dirInfo, dirInfos) {
        const QString remoteSubDir = remoteDir + '/' + dirInfo.fileName();
        const SftpMakeDir::Ptr mkdirOp(
            new SftpMakeDir(++m_nextJobId, remoteSubDir, op->parentJob));
        op->parentJob->mkdirsInProgress.insert(mkdirOp,
            SftpUploadDir::Dir(dirInfo.absoluteFilePath(), remoteSubDir));
        createJob(mkdirOp);
    }

    const QFileInfoList &fileInfos = localDir.entryInfoList(QDir::Files);
    foreach (const QFileInfo &fileInfo, fileInfos) {
        QSharedPointer<QFile> localFile(new QFile(fileInfo.absoluteFilePath()));
        if (!localFile->open(QIODevice::ReadOnly)) {
            op->parentJob->setError();
            emit finished(op->parentJob->jobId,
                tr("Could not open local file '%1': %2")
                .arg(fileInfo.absoluteFilePath(), localFile->errorString()));
            m_jobs.erase(it);
            return;
        }

        const QString remoteFilePath = remoteDir + '/' + fileInfo.fileName();
        SftpUploadFile::Ptr uploadFileOp(new SftpUploadFile(++m_nextJobId,
            remoteFilePath, localFile, SftpOverwriteExisting, op->parentJob));
        createJob(uploadFileOp);
        op->parentJob->uploadsInProgress.append(uploadFileOp);
    }

    op->parentJob->mkdirsInProgress.erase(dirIt);
    if (op->parentJob->mkdirsInProgress.isEmpty()
        && op->parentJob->uploadsInProgress.isEmpty())
        emit finished(op->parentJob->jobId);
    m_jobs.erase(it);
}

void SftpChannelPrivate::handleLsStatus(const JobMap::Iterator &it,
    const SftpStatusResponse &response)
{
    SftpListDir::Ptr op = it.value().staticCast<SftpListDir>();
    switch (op->state) {
    case SftpListDir::OpenRequested:
        emit finished(op->jobId, errorMessage(response.errorString,
            tr("Remote directory could not be opened for reading.")));
        m_jobs.erase(it);
        break;
    case SftpListDir::Open:
        if (response.status != SSH_FX_EOF)
            reportRequestError(op, errorMessage(response.errorString,
                tr("Failed to list remote directory contents.")));
        op->state = SftpListDir::CloseRequested;
        sendData(m_outgoingPacket.generateCloseHandle(op->remoteHandle,
            op->jobId).rawData());
        break;
    case SftpListDir::CloseRequested:
        if (!op->hasError) {
            const QString error = errorMessage(response,
                tr("Failed to close remote directory."));
            emit finished(op->jobId, error);
        }
        m_jobs.erase(it);
        break;
    default:
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_STATUS packet.");
    }
}

void SftpChannelPrivate::handleGetStatus(const JobMap::Iterator &it,
    const SftpStatusResponse &response)
{
    SftpDownload::Ptr op = it.value().staticCast<SftpDownload>();
    switch (op->state) {
    case SftpDownload::OpenRequested:
        emit finished(op->jobId,
            errorMessage(response.errorString,
                tr("Failed to open remote file for reading.")));
        m_jobs.erase(it);
        break;
    case SftpDownload::Open:
        if (op->statRequested) {
            reportRequestError(op, errorMessage(response.errorString,
                tr("Failed retrieve information on the remote file ('stat' failed).")));
            sendTransferCloseHandle(op, response.requestId);
        } else {
            if ((response.status != SSH_FX_EOF || response.requestId != op->eofId)
                && !op->hasError)
                reportRequestError(op, errorMessage(response.errorString,
                    tr("Failed to read remote file.")));
            finishTransferRequest(it);
        }
        break;
    case SftpDownload::CloseRequested:
        Q_ASSERT(op->inFlightCount == 1);
        if (!op->hasError) {
            if (response.status == SSH_FX_OK)
                emit finished(op->jobId);
            else
                reportRequestError(op, errorMessage(response.errorString,
                    tr("Failed to close remote file.")));
        }
        removeTransferRequest(it);
        break;
    default:
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_STATUS packet.");
    }
}

void SftpChannelPrivate::handlePutStatus(const JobMap::Iterator &it,
    const SftpStatusResponse &response)
{
    SftpUploadFile::Ptr job = it.value().staticCast<SftpUploadFile>();
    switch (job->state) {
    case SftpUploadFile::OpenRequested: {
        bool emitError = false;
        if (job->parentJob) {
            if (!job->parentJob->hasError) {
                job->parentJob->setError();
                emitError = true;
            }
        } else {
            emitError = true;
        }

        if (emitError) {
            emit finished(job->jobId,
                errorMessage(response.errorString,
                    tr("Failed to open remote file for writing.")));
        }
        m_jobs.erase(it);
        break;
    }
    case SftpUploadFile::Open:
        if (job->hasError || (job->parentJob && job->parentJob->hasError)) {
            job->hasError = true;
            finishTransferRequest(it);
            return;
        }

        if (response.status == SSH_FX_OK) {
            sendWriteRequest(it);
        } else {
            if (job->parentJob)
                job->parentJob->setError();
            reportRequestError(job, errorMessage(response.errorString,
                tr("Failed to write remote file.")));
            finishTransferRequest(it);
        }
        break;
    case SftpUploadFile::CloseRequested:
        Q_ASSERT(job->inFlightCount == 1);
        if (job->hasError || (job->parentJob && job->parentJob->hasError)) {
            m_jobs.erase(it);
            return;
        }

        if (response.status == SSH_FX_OK) {
            if (job->parentJob) {
                job->parentJob->uploadsInProgress.removeOne(job);
                if (job->parentJob->mkdirsInProgress.isEmpty()
                    && job->parentJob->uploadsInProgress.isEmpty())
                    emit finished(job->parentJob->jobId);
            } else {
                emit finished(job->jobId);
            }
        } else {
            const QString error = errorMessage(response.errorString,
                tr("Failed to close remote file."));
            if (job->parentJob) {
                job->parentJob->setError();
                emit finished(job->parentJob->jobId, error);
            } else {
                emit finished(job->jobId, error);
            }
        }
        m_jobs.erase(it);
        break;
    default:
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_STATUS packet.");
    }
}

void SftpChannelPrivate::handleName()
{
    const SftpNameResponse &response = m_incomingPacket.asNameResponse();
    JobMap::Iterator it = lookupJob(response.requestId);
    switch (it.value()->type()) {
    case AbstractSftpOperation::ListDir: {
        SftpListDir::Ptr op = it.value().staticCast<SftpListDir>();
        if (op->state != SftpListDir::Open) {
            throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
                "Unexpected SSH_FXP_NAME packet.");
        }

        for (int i = 0; i < response.files.count(); ++i) {
            const SftpFile &file = response.files.at(i);
            emit dataAvailable(op->jobId, file.fileName);
        }
        sendData(m_outgoingPacket.generateReadDir(op->remoteHandle,
            op->jobId).rawData());
        break;
    }
    default:
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_NAME packet.");
    }
}

void SftpChannelPrivate::handleReadData()
{
    const SftpDataResponse &response = m_incomingPacket.asDataResponse();
    JobMap::Iterator it = lookupJob(response.requestId);
    if (it.value()->type() != AbstractSftpOperation::Download) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_DATA packet.");
    }

    SftpDownload::Ptr op = it.value().staticCast<SftpDownload>();
    if (op->hasError) {
        finishTransferRequest(it);
        return;
    }

    if (!op->localFile->seek(op->offsets[response.requestId])) {
        reportRequestError(op, op->localFile->errorString());
        finishTransferRequest(it);
        return;
    }

    if (op->localFile->write(response.data) != response.data.size()) {
        reportRequestError(op, op->localFile->errorString());
        finishTransferRequest(it);
        return;
    }

    if (op->offset >= op->fileSize && op->fileSize != 0)
        finishTransferRequest(it);
    else
        sendReadRequest(op, response.requestId);
}

void SftpChannelPrivate::handleAttrs()
{
    const SftpAttrsResponse &response = m_incomingPacket.asAttrsResponse();
    JobMap::Iterator it = lookupJob(response.requestId);
    AbstractSftpTransfer::Ptr transfer
        = it.value().dynamicCast<AbstractSftpTransfer>();
    if (!transfer || transfer->state != AbstractSftpTransfer::Open
        || !transfer->statRequested) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_ATTRS packet.");
    }
    Q_ASSERT(transfer->type() == AbstractSftpOperation::UploadFile
        || transfer->type() == AbstractSftpOperation::Download);

    if (transfer->type() == AbstractSftpOperation::Download) {
        SftpDownload::Ptr op = transfer.staticCast<SftpDownload>();
        if (response.attrs.sizePresent) {
            op->fileSize = response.attrs.size;
        } else {
            op->fileSize = 0;
            op->eofId = op->jobId;
        }
        op->statRequested = false;
        spawnReadRequests(op);
    } else {
        SftpUploadFile::Ptr op = transfer.staticCast<SftpUploadFile>();
        if (op->parentJob && op->parentJob->hasError) {
            op->hasError = true;
            sendTransferCloseHandle(op, op->jobId);
            return;
        }

        if (response.attrs.sizePresent) {
            op->offset = response.attrs.size;
            spawnWriteRequests(it);
        } else {
            if (op->parentJob)
                op->parentJob->setError();
            reportRequestError(op, tr("Cannot append to remote file: "
                "Server does not support the file size attribute."));
            sendTransferCloseHandle(op, op->jobId);
        }
    }
}

SftpChannelPrivate::JobMap::Iterator SftpChannelPrivate::lookupJob(SftpJobId id)
{
    JobMap::Iterator it = m_jobs.find(id);
    if (it == m_jobs.end()) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid request id in SFTP packet.");
    }
    return it;
}

void SftpChannelPrivate::closeHook()
{
    m_jobs.clear();
    m_incomingData.clear();
    m_incomingPacket.clear();
    emit closed();
}

void SftpChannelPrivate::handleOpenSuccessInternal()
{
#ifdef CREATOR_SSH_DEBUG
    qDebug("SFTP session started");
#endif
    m_sendFacility.sendSftpPacket(remoteChannel());
    m_sftpState = SubsystemRequested;
}

void SftpChannelPrivate::handleOpenFailureInternal()
{
    if (channelState() != SessionRequested) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_MSG_CHANNEL_OPEN_FAILURE packet.");
    }
    emit initializationFailed(tr("Server could not start session."));
}

void SftpChannelPrivate::sendReadRequest(const SftpDownload::Ptr &job,
    quint32 requestId)
{
    Q_ASSERT(job->eofId == SftpInvalidJob);
    sendData(m_outgoingPacket.generateReadFile(job->remoteHandle, job->offset,
        AbstractSftpPacket::MaxDataSize, requestId).rawData());
    job->offsets[requestId] = job->offset;
    job->offset += AbstractSftpPacket::MaxDataSize;
    if (job->offset >= job->fileSize)
        job->eofId = requestId;
}

void SftpChannelPrivate::reportRequestError(const AbstractSftpOperationWithHandle::Ptr &job,
    const QString &error)
{
    emit finished(job->jobId, error);
    job->hasError = true;
}

void SftpChannelPrivate::finishTransferRequest(const JobMap::Iterator &it)
{
    AbstractSftpTransfer::Ptr job = it.value().staticCast<AbstractSftpTransfer>();
    if (job->inFlightCount == 1)
        sendTransferCloseHandle(job, it.key());
    else
        removeTransferRequest(it);
}

void SftpChannelPrivate::sendTransferCloseHandle(const AbstractSftpTransfer::Ptr &job,
    quint32 requestId)
{
    sendData(m_outgoingPacket.generateCloseHandle(job->remoteHandle,
       requestId).rawData());
    job->state = SftpDownload::CloseRequested;
}

void SftpChannelPrivate::removeTransferRequest(const JobMap::Iterator &it)
{
    --it.value().staticCast<AbstractSftpTransfer>()->inFlightCount;
    m_jobs.erase(it);
}

void SftpChannelPrivate::sendWriteRequest(const JobMap::Iterator &it)
{
    SftpUploadFile::Ptr job = it.value().staticCast<SftpUploadFile>();
    QByteArray data = job->localFile->read(AbstractSftpPacket::MaxDataSize);
    if (job->localFile->error() != QFile::NoError) {
        if (job->parentJob)
            job->parentJob->setError();
        reportRequestError(job, tr("Error reading local file: %1")
            .arg(job->localFile->errorString()));
        finishTransferRequest(it);
    } else if (data.isEmpty()) {
        finishTransferRequest(it);
    } else {
        sendData(m_outgoingPacket.generateWriteFile(job->remoteHandle,
            job->offset, data, it.key()).rawData());
        job->offset += AbstractSftpPacket::MaxDataSize;
    }
}

void SftpChannelPrivate::spawnWriteRequests(const JobMap::Iterator &it)
{
    SftpUploadFile::Ptr op = it.value().staticCast<SftpUploadFile>();
    op->calculateInFlightCount(AbstractSftpPacket::MaxDataSize);
    sendWriteRequest(it);
    for (int i = 1; !op->hasError && i < op->inFlightCount; ++i)
        sendWriteRequest(m_jobs.insert(++m_nextJobId, op));
}

void SftpChannelPrivate::spawnReadRequests(const SftpDownload::Ptr &job)
{
    job->calculateInFlightCount(AbstractSftpPacket::MaxDataSize);
    sendReadRequest(job, job->jobId);
    for (int i = 1; i < job->inFlightCount; ++i) {
        const quint32 requestId = ++m_nextJobId;
        m_jobs.insert(requestId, job);
        sendReadRequest(job, requestId);
    }
}

} // namespace Internal
} // namespace Utils
