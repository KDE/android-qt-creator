/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "sshchannel_p.h"

#include "sshincomingpacket_p.h"
#include "sshsendfacility_p.h"

#include <botan/exceptn.h>

#include <QtCore/QTimer>

namespace Core {
namespace Internal {

namespace {
    const quint32 MinMaxPacketSize = 32768;
    const quint32 MaxPacketSize = 16 * 1024 * 1024;
    const quint32 InitialWindowSize = MaxPacketSize;
    const quint32 NoChannel = 0xffffffffu;
} // anonymous namespace

AbstractSshChannel::AbstractSshChannel(quint32 channelId,
    SshSendFacility &sendFacility)
    : m_sendFacility(sendFacility), m_timeoutTimer(new QTimer(this)),
      m_localChannel(channelId), m_remoteChannel(NoChannel),
      m_localWindowSize(InitialWindowSize), m_remoteWindowSize(0),
      m_state(Inactive)
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, SIGNAL(timeout()), this, SIGNAL(timeout()));
}

AbstractSshChannel::~AbstractSshChannel()
{

}

void AbstractSshChannel::setChannelState(ChannelState state)
{
    m_state = state;
    if (state == Closed)
        closeHook();
}

void AbstractSshChannel::requestSessionStart()
{
    // Note: We are just being paranoid here about the Botan exceptions,
    // which are extremely unlikely to happen, because if there was a problem
    // with our cryptography stuff, it would have hit us before, on
    // establishing the connection.
    try {
        m_sendFacility.sendSessionPacket(m_localChannel, InitialWindowSize,
            MaxPacketSize);
        setChannelState(SessionRequested);
        m_timeoutTimer->start(ReplyTimeout);
    }  catch (Botan::Exception &e) {
        m_errorString = QString::fromAscii(e.what());
        closeChannel();
    }
}

void AbstractSshChannel::sendData(const QByteArray &data)
{
    try {
        m_sendBuffer += data;
        flushSendBuffer();
    }  catch (Botan::Exception &e) {
        m_errorString = QString::fromAscii(e.what());
        closeChannel();
    }
}

void AbstractSshChannel::handleWindowAdjust(quint32 bytesToAdd)
{
    checkChannelActive();

    const quint64 newValue = m_remoteWindowSize + bytesToAdd;
    if (newValue > 0xffffffffu) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Illegal window size requested.");
    }

    m_remoteWindowSize = newValue;
    flushSendBuffer();
}

void AbstractSshChannel::flushSendBuffer()
{
    const quint32 bytesToSend
        = qMin<quint32>(m_remoteWindowSize, m_sendBuffer.size());
    if (bytesToSend > 0) {
        const QByteArray &data = m_sendBuffer.left(bytesToSend);
        m_sendFacility.sendChannelDataPacket(m_remoteChannel, data);
        m_sendBuffer.remove(0, bytesToSend);
        m_remoteWindowSize -= bytesToSend;
    }
}

void AbstractSshChannel::handleOpenSuccess(quint32 remoteChannelId,
    quint32 remoteWindowSize, quint32 remoteMaxPacketSize)
{
    if (m_state != SessionRequested) {
       throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
           "Invalid SSH_MSG_CHANNEL_OPEN_CONFIRMATION packet.");
   }
    m_timeoutTimer->stop();

   if (remoteMaxPacketSize < MinMaxPacketSize) {
       throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
           "Maximum packet size too low.");
   }

#ifdef CREATOR_SSH_DEBUG
   qDebug("Channel opened. remote channel id: %u, remote window size: %u, "
       "remote max packet size: %u",
       remoteChannelId, remoteWindowSize, remoteMaxPacketSize);
#endif
   m_remoteChannel = remoteChannelId;
   m_remoteWindowSize = remoteWindowSize;
   m_remoteMaxPacketSize = remoteMaxPacketSize;
   setChannelState(SessionEstablished);
   handleOpenSuccessInternal();
}

void AbstractSshChannel::handleOpenFailure(const QString &reason)
{
    if (m_state != SessionRequested) {
       throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
           "Invalid SSH_MSG_CHANNEL_OPEN_FAILURE packet.");
   }
    m_timeoutTimer->stop();

#ifdef CREATOR_SSH_DEBUG
   qDebug("Channel open request failed for channel %u", m_localChannel);
#endif
   m_errorString = reason;
   handleOpenFailureInternal();
}

void AbstractSshChannel::handleChannelEof()
{
    if (m_state == Inactive || m_state == Closed) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_MSG_CHANNEL_EOF message.");
    }
    m_localWindowSize = 0;
}

void AbstractSshChannel::handleChannelClose()
{
#ifdef CREATOR_SSH_DEBUG
    qDebug("Receiving CLOSE for channel %u", m_localChannel);
#endif
    if (channelState() == Inactive || channelState() == Closed) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_MSG_CHANNEL_CLOSE message.");
    }
    closeChannel();
    setChannelState(Closed);
}

void AbstractSshChannel::handleChannelData(const QByteArray &data)
{
    const int bytesToDeliver = handleChannelOrExtendedChannelData(data);
    handleChannelDataInternal(bytesToDeliver == data.size()
        ? data : data.left(bytesToDeliver));
}

void AbstractSshChannel::handleChannelExtendedData(quint32 type, const QByteArray &data)
{
    const int bytesToDeliver = handleChannelOrExtendedChannelData(data);
    handleChannelExtendedDataInternal(type, bytesToDeliver == data.size()
        ? data : data.left(bytesToDeliver));
}

void AbstractSshChannel::handleChannelRequest(const SshIncomingPacket &packet)
{
    checkChannelActive();
    const QByteArray &requestType = packet.extractChannelRequestType();
    if (requestType == SshIncomingPacket::ExitStatusType)
        handleExitStatus(packet.extractChannelExitStatus());
    else if (requestType == SshIncomingPacket::ExitSignalType)
        handleExitSignal(packet.extractChannelExitSignal());
    else if (requestType != "eow@openssh.com") // Suppress warning for this one, as it's sent all the time.
        qWarning("Ignoring unknown request type '%s'", requestType.data());
}

int AbstractSshChannel::handleChannelOrExtendedChannelData(const QByteArray &data)
{
    checkChannelActive();

    const int bytesToDeliver = qMin<quint32>(data.size(), maxDataSize());
    if (bytesToDeliver != data.size())
        qWarning("Misbehaving server does not respect local window, clipping.");

    m_localWindowSize -= bytesToDeliver;
    if (m_localWindowSize < MaxPacketSize) {
        m_localWindowSize += MaxPacketSize;
        m_sendFacility.sendWindowAdjustPacket(m_remoteChannel,
            MaxPacketSize);
    }
    return bytesToDeliver;
}

void AbstractSshChannel::closeChannel()
{
    if (m_state == CloseRequested) {
        m_timeoutTimer->stop();
    } else if (m_state != Closed) {
        if (m_state == Inactive) {
            setChannelState(Closed);
        } else {
            setChannelState(CloseRequested);
            m_sendFacility.sendChannelEofPacket(m_remoteChannel);
            m_sendFacility.sendChannelClosePacket(m_remoteChannel);
        }
    }
}

void AbstractSshChannel::checkChannelActive()
{
    if (channelState() == Inactive || channelState() == Closed)
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Channel not open.");
}

quint32 AbstractSshChannel::maxDataSize() const
{
    return qMin(m_localWindowSize, MaxPacketSize);
}

} // namespace Internal
} // namespace Core
