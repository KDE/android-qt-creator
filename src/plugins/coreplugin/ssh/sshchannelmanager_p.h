/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef SSHCHANNELLAYER_P_H
#define SSHCHANNELLAYER_P_H

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

namespace Core {

class SftpChannel;
class SshRemoteProcess;

namespace Internal {

class AbstractSshChannel;
class SshIncomingPacket;
class SshSendFacility;

class SshChannelManager : public QObject
{
    Q_OBJECT
public:
    SshChannelManager(SshSendFacility &sendFacility, QObject *parent);

    QSharedPointer<SshRemoteProcess> createRemoteProcess(const QByteArray &command);
    QSharedPointer<SftpChannel> createSftpChannel();
    void closeAllChannels();

    void handleChannelRequest(const SshIncomingPacket &packet);
    void handleChannelOpen(const SshIncomingPacket &packet);
    void handleChannelOpenFailure(const SshIncomingPacket &packet);
    void handleChannelOpenConfirmation(const SshIncomingPacket &packet);
    void handleChannelSuccess(const SshIncomingPacket &packet);
    void handleChannelFailure(const SshIncomingPacket &packet);
    void handleChannelWindowAdjust(const SshIncomingPacket &packet);
    void handleChannelData(const SshIncomingPacket &packet);
    void handleChannelExtendedData(const SshIncomingPacket &packet);
    void handleChannelEof(const SshIncomingPacket &packet);
    void handleChannelClose(const SshIncomingPacket &packet);

signals:
    void timeout();

private:
    typedef QHash<quint32, AbstractSshChannel *>::Iterator ChannelIterator;

    ChannelIterator lookupChannelAsIterator(quint32 channelId,
        bool allowNotFound = false);
    AbstractSshChannel *lookupChannel(quint32 channelId,
        bool allowNotFound = false);
    void removeChannel(ChannelIterator it);
    void insertChannel(AbstractSshChannel *priv,
        const QSharedPointer<QObject> &pub);

    SshSendFacility &m_sendFacility;
    QHash<quint32, AbstractSshChannel *> m_channels;
    QHash<AbstractSshChannel *, QSharedPointer<QObject> > m_sessions;
    quint32 m_nextLocalChannelId;
};

} // namespace Internal
} // namespace Core

#endif // SSHCHANNELLAYER_P_H
