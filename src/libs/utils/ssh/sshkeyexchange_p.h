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

#ifndef SSHKEYEXCHANGE_P_H
#define SSHKEYEXCHANGE_P_H

#include <botan/dh.h>

#include <QtCore/QByteArray>
#include <QtCore/QScopedPointer>

namespace Botan { class HashFunction; }

namespace Utils {
namespace Internal {

class SshSendFacility;
class SshIncomingPacket;

class SshKeyExchange
{
public:
    SshKeyExchange(SshSendFacility &sendFacility);
    ~SshKeyExchange();

    void sendKexInitPacket(const QByteArray &serverId);

    // Returns true <=> the server sends a guessed package.
    bool sendDhInitPacket(const SshIncomingPacket &serverKexInit);

    void sendNewKeysPacket(const SshIncomingPacket &dhReply,
        const QByteArray &clientId);

    QByteArray k() const { return m_k; }
    QByteArray h() const { return m_h; }
    Botan::HashFunction *hash() const { return m_hash.data(); }
    QByteArray encryptionAlgo() const { return m_encryptionAlgo; }
    QByteArray decryptionAlgo() const { return m_decryptionAlgo; }
    QByteArray hMacAlgoClientToServer() const { return m_c2sHMacAlgo; }
    QByteArray hMacAlgoServerToClient() const { return m_s2cHMacAlgo; }

private:
    QByteArray m_serverId;
    QByteArray m_clientKexInitPayload;
    QByteArray m_serverKexInitPayload;
    QScopedPointer<Botan::DH_PrivateKey> m_dhKey;
    QByteArray m_k;
    QByteArray m_h;
    QByteArray m_serverHostKeyAlgo;
    QByteArray m_encryptionAlgo;
    QByteArray m_decryptionAlgo;
    QByteArray m_c2sHMacAlgo;
    QByteArray m_s2cHMacAlgo;
    QScopedPointer<Botan::HashFunction> m_hash;
    SshSendFacility &m_sendFacility;
};

} // namespace Internal
} // namespace Utils

#endif // SSHKEYEXCHANGE_P_H
