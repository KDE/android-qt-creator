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

#include "sshoutgoingpacket_p.h"

#include "sshcapabilities_p.h"
#include "sshcryptofacility_p.h"

#include <QtCore/QtEndian>

namespace Core {
namespace Internal {

SshOutgoingPacket::SshOutgoingPacket(const SshEncryptionFacility &encrypter,
    const quint32 &seqNr) : m_encrypter(encrypter), m_seqNr(seqNr)
{
}

quint32 SshOutgoingPacket::cipherBlockSize() const
{
    return qMax(m_encrypter.cipherBlockSize(), 4U);
}

quint32 SshOutgoingPacket::macLength() const
{
    return m_encrypter.macLength();
}

void SshOutgoingPacket::generateKeyExchangeInitPacket()
{
    const QByteArray &supportedkeyExchangeMethods
        = encodeNameList(SshCapabilities::KeyExchangeMethods);
    const QByteArray &supportedPublicKeyAlgorithms
        = encodeNameList(SshCapabilities::PublicKeyAlgorithms);
    const QByteArray &supportedEncryptionAlgorithms
        = encodeNameList(SshCapabilities::EncryptionAlgorithms);
    const QByteArray &supportedMacAlgorithms
        = encodeNameList(SshCapabilities::MacAlgorithms);
    const QByteArray &supportedCompressionAlgorithms
        = encodeNameList(SshCapabilities::CompressionAlgorithms);
    const QByteArray &supportedLanguages = encodeNameList(QList<QByteArray>());

    init(SSH_MSG_KEXINIT);
    m_data += m_encrypter.getRandomNumbers(16);
    m_data.append(supportedkeyExchangeMethods);
    m_data.append(supportedPublicKeyAlgorithms);
    m_data.append(supportedEncryptionAlgorithms)
        .append(supportedEncryptionAlgorithms);
    m_data.append(supportedMacAlgorithms).append(supportedMacAlgorithms);
    m_data.append(supportedCompressionAlgorithms)
        .append(supportedCompressionAlgorithms);
    m_data.append(supportedLanguages).append(supportedLanguages);
    appendBool(false); // No guessed packet.
    m_data.append(QByteArray(4, 0)); // Reserved.
    finalize();
}

void SshOutgoingPacket::generateKeyDhInitPacket(const Botan::BigInt &e)
{
    init(SSH_MSG_KEXDH_INIT).appendMpInt(e).finalize();
}

void SshOutgoingPacket::generateNewKeysPacket()
{
    init(SSH_MSG_NEWKEYS).finalize();
}

void SshOutgoingPacket::generateUserAuthServiceRequestPacket()
{
    generateServiceRequest("ssh-userauth");
}

void SshOutgoingPacket::generateServiceRequest(const QByteArray &service)
{
    init(SSH_MSG_SERVICE_REQUEST).appendString(service).finalize();
}

void SshOutgoingPacket::generateUserAuthByPwdRequestPacket(const QByteArray &user,
    const QByteArray &service, const QByteArray &pwd)
{
    init(SSH_MSG_USERAUTH_REQUEST).appendString(user).appendString(service)
        .appendString("password").appendBool(false).appendString(pwd)
        .finalize();
}

void SshOutgoingPacket::generateUserAuthByKeyRequestPacket(const QByteArray &user,
    const QByteArray &service)
{
    init(SSH_MSG_USERAUTH_REQUEST).appendString(user).appendString(service)
        .appendString("publickey").appendBool(true)
        .appendString(m_encrypter.authenticationAlgorithmName())
        .appendString(m_encrypter.authenticationPublicKey());
    const QByteArray &dataToSign = m_data.mid(PayloadOffset);
    appendString(m_encrypter.authenticationKeySignature(dataToSign));
    finalize();
}

void SshOutgoingPacket::generateRequestFailurePacket()
{
    init(SSH_MSG_REQUEST_FAILURE).finalize();
}

void SshOutgoingPacket::generateSessionPacket(quint32 channelId,
    quint32 windowSize, quint32 maxPacketSize)
{
    init(SSH_MSG_CHANNEL_OPEN).appendString("session").appendInt(channelId)
        .appendInt(windowSize).appendInt(maxPacketSize).finalize();
}

void SshOutgoingPacket::generateEnvPacket(quint32 remoteChannel,
    const QByteArray &var, const QByteArray &value)
{
    init(SSH_MSG_CHANNEL_REQUEST).appendInt(remoteChannel).appendString("env")
        .appendBool(false).appendString(var).appendString(value);
}

void SshOutgoingPacket::generateExecPacket(quint32 remoteChannel,
    const QByteArray &command)
{
    init(SSH_MSG_CHANNEL_REQUEST).appendInt(remoteChannel).appendString("exec")
        .appendBool(true).appendString(command).finalize();
}

void SshOutgoingPacket::generateSftpPacket(quint32 remoteChannel)
{
    init(SSH_MSG_CHANNEL_REQUEST).appendInt(remoteChannel)
        .appendString("subsystem").appendBool(true).appendString("sftp")
        .finalize();
}

void SshOutgoingPacket::generateWindowAdjustPacket(quint32 remoteChannel,
    quint32 bytesToAdd)
{
    init(SSH_MSG_CHANNEL_WINDOW_ADJUST).appendInt(remoteChannel)
        .appendInt(bytesToAdd).finalize();
}

void SshOutgoingPacket::generateChannelDataPacket(quint32 remoteChannel,
    const QByteArray &data)
{
    init(SSH_MSG_CHANNEL_DATA).appendInt(remoteChannel).appendString(data)
        .finalize();
}

void SshOutgoingPacket::generateChannelSignalPacket(quint32 remoteChannel,
    const QByteArray &signalName)
{
    init(SSH_MSG_CHANNEL_REQUEST).appendInt(remoteChannel)
        .appendString("signal").appendBool(false).appendString(signalName)
        .finalize();
}

void SshOutgoingPacket::generateChannelEofPacket(quint32 remoteChannel)
{
    init(SSH_MSG_CHANNEL_EOF).appendInt(remoteChannel).finalize();
}

void SshOutgoingPacket::generateChannelClosePacket(quint32 remoteChannel)
{
    init(SSH_MSG_CHANNEL_CLOSE).appendInt(remoteChannel).finalize();
}

void SshOutgoingPacket::generateDisconnectPacket(SshErrorCode reason,
    const QByteArray &reasonString)
{
    init(SSH_MSG_DISCONNECT).appendInt(reason).appendString(reasonString)
        .appendString(QByteArray()).finalize();
}

void SshOutgoingPacket::generateMsgUnimplementedPacket(quint32 serverSeqNr)
{
    init(SSH_MSG_UNIMPLEMENTED).appendInt(serverSeqNr).finalize();
}

SshOutgoingPacket &SshOutgoingPacket::appendInt(quint32 val)
{
    m_data.append(encodeInt(val));
    return *this;
}

SshOutgoingPacket &SshOutgoingPacket::appendMpInt(const Botan::BigInt &number)
{
    m_data.append(encodeMpInt(number));
    return *this;
}

SshOutgoingPacket &SshOutgoingPacket::appendBool(bool b)
{
    m_data += static_cast<char>(b);
    return *this;
}

SshOutgoingPacket &SshOutgoingPacket::appendString(const QByteArray &string)
{
    m_data.append(encodeString(string));
    return *this;
}

SshOutgoingPacket &SshOutgoingPacket::init(SshPacketType type)
{
    m_data.resize(TypeOffset + 1);
    m_data[TypeOffset] = type;
    return *this;
}

SshOutgoingPacket &SshOutgoingPacket::setPadding()
{
    m_data += m_encrypter.getRandomNumbers(MinPaddingLength);
    int padLength = MinPaddingLength;
    const int divisor = sizeDivisor();
    const int mod = m_data.size() % divisor;
    padLength += divisor - mod;
    m_data += m_encrypter.getRandomNumbers(padLength - MinPaddingLength);
    m_data[PaddingLengthOffset] = padLength;
    return *this;
}

SshOutgoingPacket &SshOutgoingPacket::encrypt()
{
    const QByteArray &mac
        = generateMac(m_encrypter, m_seqNr);
    m_encrypter.encrypt(m_data);
    m_data += mac;
    return *this;
}

void SshOutgoingPacket::finalize()
{
    setPadding();
    setLengthField(m_data);
    m_length = m_data.size() - 4;
#ifdef CREATOR_SSH_DEBUG
    qDebug("Encrypting packet of type %u", m_data.at(TypeOffset));
#endif
    encrypt();
    Q_ASSERT(isComplete());
}

int SshOutgoingPacket::sizeDivisor() const
{
    return qMax(cipherBlockSize(), 8U);
}

QByteArray SshOutgoingPacket::encodeNameList(const QList<QByteArray> &list)
{
    QByteArray data;
    data.resize(4);
    for (int i = 0; i < list.count(); ++i) {
        if (i > 0)
            data.append(',');
        data.append(list.at(i));
    }
    AbstractSshPacket::setLengthField(data);
    return data;
}

} // namespace Internal
} // namespace Core
