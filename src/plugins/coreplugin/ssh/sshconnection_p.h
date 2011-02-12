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

#ifndef SSHCONNECTION_P_H
#define SSHCONNECTION_P_H

#include "sshconnection.h"
#include "sshexception_p.h"
#include "sshincomingpacket_p.h"
#include "sshremoteprocess.h"
#include "sshsendfacility_p.h"

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QScopedPointer>
#include <QtCore/QTimer>

QT_BEGIN_NAMESPACE
class QTcpSocket;
QT_END_NAMESPACE

namespace Botan { class Exception; }

namespace Core {
class SftpChannel;

namespace Internal {
class SshChannelManager;

// NOTE: When you add stuff here, don't forget to update m_packetHandlers.
enum SshStateInternal {
    SocketUnconnected, // initial and after disconnect
    SocketConnecting, // After connectToHost()
    SocketConnected, // After socket's connected() signal
    KeyExchangeStarted, // After server's KEXINIT message
    KeyExchangeSuccess, // After server's DH_REPLY message
    UserAuthServiceRequested,
    UserAuthRequested,

    ConnectionEstablished // After service has been started
    // ...
};

class SshConnectionPrivate : public QObject
{
    Q_OBJECT
    friend class Core::SshConnection;
public:
    SshConnectionPrivate(SshConnection *conn);
    ~SshConnectionPrivate();

    void connectToHost(const SshConnectionParameters &serverInfo);
    void closeConnection(SshErrorCode sshError, SshError userError,
        const QByteArray &serverErrorString, const QString &userErrorString);
    QSharedPointer<SshRemoteProcess> createRemoteProcess(const QByteArray &command);
    QSharedPointer<SftpChannel> createSftpChannel();
    SshStateInternal state() const { return m_state; }
    SshError error() const { return m_error; }
    QString errorString() const { return m_errorString; }

signals:
    void connected();
    void disconnected();
    void dataAvailable(const QString &message);
    void error(Core::SshError);

private:
    Q_SLOT void handleSocketConnected();
    Q_SLOT void handleIncomingData();
    Q_SLOT void handleSocketError();
    Q_SLOT void handleSocketDisconnected();
    Q_SLOT void handleTimeout();
    Q_SLOT void sendKeepAlivePacket();

    void handleServerId();
    void handlePackets();
    void handleCurrentPacket();
    void handleKeyExchangeInitPacket();
    void handleKeyExchangeReplyPacket();
    void handleNewKeysPacket();
    void handleServiceAcceptPacket();
    void handlePasswordExpiredPacket();
    void handleUserAuthSuccessPacket();
    void handleUserAuthFailurePacket();
    void handleUserAuthBannerPacket();
    void handleGlobalRequest();
    void handleDebugPacket();
    void handleUnimplementedPacket();
    void handleChannelRequest();
    void handleChannelOpen();
    void handleChannelOpenFailure();
    void handleChannelOpenConfirmation();
    void handleChannelSuccess();
    void handleChannelFailure();
    void handleChannelWindowAdjust();
    void handleChannelData();
    void handleChannelExtendedData();
    void handleChannelEof();
    void handleChannelClose();
    void handleDisconnect();
    bool canUseSocket() const;

    void sendData(const QByteArray &data);

    typedef void (SshConnectionPrivate::*PacketHandler)();
    typedef QList<SshStateInternal> StateList;
    void setupPacketHandlers();
    void setupPacketHandler(SshPacketType type, const StateList &states,
        PacketHandler handler);

    typedef QPair<StateList, PacketHandler> HandlerInStates;
    QHash<SshPacketType, HandlerInStates> m_packetHandlers;

    static const quint64 InvalidSeqNr;

    QTcpSocket *m_socket;
    SshStateInternal m_state;
    SshIncomingPacket m_incomingPacket;
    SshSendFacility m_sendFacility;
    SshChannelManager * const m_channelManager;
    SshConnectionParameters m_connParams;
    QByteArray m_incomingData;
    SshError m_error;
    QString m_errorString;
    QScopedPointer<SshKeyExchange> m_keyExchange;
    QTimer m_timeoutTimer;
    QTimer m_keepAliveTimer;
    bool m_ignoreNextPacket;
    SshConnection *m_conn;
    quint64 m_lastInvalidMsgSeqNr;
};

} // namespace Internal
} // namespace Core

#endif // SSHCONNECTION_P_H
