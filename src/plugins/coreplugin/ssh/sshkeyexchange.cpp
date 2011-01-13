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

#include "sshkeyexchange_p.h"

#include "sshbotanconversions_p.h"
#include "sshcapabilities_p.h"
#include "sshsendfacility_p.h"
#include "sshexception_p.h"
#include "sshincomingpacket_p.h"

#include <botan/botan.h>
#include <botan/dsa.h>
#include <botan/look_pk.h>
#include <botan/pubkey.h>
#include <botan/rsa.h>

#include <string>

using namespace Botan;

namespace Core {
namespace Internal {

namespace {

    // For debugging
    void printNameList(const char *listName, const SshNameList &list)
    {
#ifdef CREATOR_SSH_DEBUG
        qDebug("%s:", listName);
        foreach (const QByteArray &name, list.names)
            qDebug("%s", name.constData());
#else
        Q_UNUSED(listName);
        Q_UNUSED(list);
#endif
    }
} // anonymous namespace

SshKeyExchange::SshKeyExchange(SshSendFacility &sendFacility)
    : m_sendFacility(sendFacility)
{
}

SshKeyExchange::~SshKeyExchange() {}

void SshKeyExchange::sendKexInitPacket(const QByteArray &serverId)
{
    m_serverId = serverId;
    const AbstractSshPacket::Payload &payload
        = m_sendFacility.sendKeyExchangeInitPacket();
    m_clientKexInitPayload = QByteArray(payload.data, payload.size);
}

bool SshKeyExchange::sendDhInitPacket(const SshIncomingPacket &serverKexInit)
{
#ifdef CREATOR_SSH_DEBUG
    qDebug("server requests key exchange");
#endif
    serverKexInit.printRawBytes();
    SshKeyExchangeInit kexInitParams
            = serverKexInit.extractKeyExchangeInitData();

    printNameList("Key Algorithms", kexInitParams.keyAlgorithms);
    printNameList("Server Host Key Algorithms", kexInitParams.serverHostKeyAlgorithms);
    printNameList("Encryption algorithms client to server", kexInitParams.encryptionAlgorithmsClientToServer);
    printNameList("Encryption algorithms server to client", kexInitParams.encryptionAlgorithmsServerToClient);
    printNameList("MAC algorithms client to server", kexInitParams.macAlgorithmsClientToServer);
    printNameList("MAC algorithms server to client", kexInitParams.macAlgorithmsServerToClient);
    printNameList("Compression algorithms client to server", kexInitParams.compressionAlgorithmsClientToServer);
    printNameList("Compression algorithms client to server", kexInitParams.compressionAlgorithmsClientToServer);
    printNameList("Languages client to server", kexInitParams.languagesClientToServer);
    printNameList("Languages server to client", kexInitParams.languagesServerToClient);
#ifdef CREATOR_SSH_DEBUG
    qDebug("First packet follows: %d", kexInitParams.firstKexPacketFollows);
#endif

    const QByteArray &keyAlgo
        = SshCapabilities::findBestMatch(SshCapabilities::KeyExchangeMethods,
              kexInitParams.keyAlgorithms.names);
    m_serverHostKeyAlgo
        = SshCapabilities::findBestMatch(SshCapabilities::PublicKeyAlgorithms,
              kexInitParams.serverHostKeyAlgorithms.names);
    m_encryptionAlgo
        = SshCapabilities::findBestMatch(SshCapabilities::EncryptionAlgorithms,
              kexInitParams.encryptionAlgorithmsClientToServer.names);
    m_decryptionAlgo
        = SshCapabilities::findBestMatch(SshCapabilities::EncryptionAlgorithms,
              kexInitParams.encryptionAlgorithmsServerToClient.names);
    m_c2sHMacAlgo
        = SshCapabilities::findBestMatch(SshCapabilities::MacAlgorithms,
              kexInitParams.macAlgorithmsClientToServer.names);
    m_s2cHMacAlgo
        = SshCapabilities::findBestMatch(SshCapabilities::MacAlgorithms,
              kexInitParams.macAlgorithmsServerToClient.names);
    SshCapabilities::findBestMatch(SshCapabilities::CompressionAlgorithms,
        kexInitParams.compressionAlgorithmsClientToServer.names);
    SshCapabilities::findBestMatch(SshCapabilities::CompressionAlgorithms,
        kexInitParams.compressionAlgorithmsServerToClient.names);

    AutoSeeded_RNG rng;
    m_dhKey.reset(new DH_PrivateKey(rng,
        DL_Group(botanKeyExchangeAlgoName(keyAlgo))));

    const AbstractSshPacket::Payload &payload = serverKexInit.payLoad();
    m_serverKexInitPayload = QByteArray(payload.data, payload.size);
    m_sendFacility.sendKeyDhInitPacket(m_dhKey->get_y());
    return kexInitParams.firstKexPacketFollows;
}

void SshKeyExchange::sendNewKeysPacket(const SshIncomingPacket &dhReply,
    const QByteArray &clientId)
{
    const SshKeyExchangeReply &reply
        = dhReply.extractKeyExchangeReply(m_serverHostKeyAlgo);
    if (reply.f <= 0 || reply.f >= m_dhKey->group_p()) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_KEY_EXCHANGE_FAILED,
            "Server sent invalid f.");
    }

    QByteArray concatenatedData = AbstractSshPacket::encodeString(clientId);
    concatenatedData += AbstractSshPacket::encodeString(m_serverId);
    concatenatedData += AbstractSshPacket::encodeString(m_clientKexInitPayload);
    concatenatedData += AbstractSshPacket::encodeString(m_serverKexInitPayload);
    concatenatedData += reply.k_s;
    concatenatedData += AbstractSshPacket::encodeMpInt(m_dhKey->get_y());
    concatenatedData += AbstractSshPacket::encodeMpInt(reply.f);
    SymmetricKey k = m_dhKey->derive_key(reply.f);
    m_k = AbstractSshPacket::encodeMpInt(BigInt(k.begin(), k.length()));
    concatenatedData += m_k;

    m_hash.reset(get_hash(botanSha1Name()));
    const SecureVector<byte> &hashResult
        = m_hash->process(convertByteArray(concatenatedData),
                        concatenatedData.size());
    m_h = convertByteArray(hashResult);

    QScopedPointer<Public_Key> sigKey;
    QScopedPointer<PK_Verifier> verifier;
    if (m_serverHostKeyAlgo == SshCapabilities::PubKeyDss) {
        const DL_Group group(reply.parameters.at(0), reply.parameters.at(1),
            reply.parameters.at(2));
        DSA_PublicKey * const dsaKey
            = new DSA_PublicKey(group, reply.parameters.at(3));
        sigKey.reset(dsaKey);
        verifier.reset(get_pk_verifier(*dsaKey,
            botanEmsaAlgoName(SshCapabilities::PubKeyDss)));
    } else if (m_serverHostKeyAlgo == SshCapabilities::PubKeyRsa) {
        RSA_PublicKey * const rsaKey
            = new RSA_PublicKey(reply.parameters.at(1), reply.parameters.at(0));
        sigKey.reset(rsaKey);
        verifier.reset(get_pk_verifier(*rsaKey,
            botanEmsaAlgoName(SshCapabilities::PubKeyRsa)));
    } else {
        Q_ASSERT(!"Impossible: Neither DSS nor RSA!");
    }
    const byte * const botanH = convertByteArray(m_h);
    const Botan::byte * const botanSig
        = convertByteArray(reply.signatureBlob);
    if (!verifier->verify_message(botanH, m_h.size(), botanSig,
        reply.signatureBlob.size())) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_KEY_EXCHANGE_FAILED,
            "Invalid signature in SSH_MSG_KEXDH_REPLY packet.");
    }

    m_sendFacility.sendNewKeysPacket();
}

} // namespace Internal
} // namespace Core
