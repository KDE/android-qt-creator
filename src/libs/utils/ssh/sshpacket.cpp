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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "sshpacket_p.h"

#include "sshcapabilities_p.h"
#include "sshcryptofacility_p.h"
#include "sshexception_p.h"
#include "sshpacketparser_p.h"

#include <QtCore/QDebug>

#include <cctype>

namespace Utils {
namespace Internal {

const quint32 AbstractSshPacket::PaddingLengthOffset = 4;
const quint32 AbstractSshPacket::PayloadOffset = PaddingLengthOffset + 1;
const quint32 AbstractSshPacket::TypeOffset = PayloadOffset;
const quint32 AbstractSshPacket::MinPaddingLength = 4;

namespace {

    void printByteArray(const QByteArray &data)
    {
#ifdef CREATOR_SSH_DEBUG
        for (int i = 0; i < data.count(); ++i)
            qDebug() << std::hex << (static_cast<unsigned int>(data[i]) & 0xff) << " ";
#else
        Q_UNUSED(data);
#endif
    }
} // anonymous namespace


AbstractSshPacket::AbstractSshPacket() : m_length(0) { }
AbstractSshPacket::~AbstractSshPacket() {}

bool AbstractSshPacket::isComplete() const
{
    if (currentDataSize() < minPacketSize())
        return false;
    Q_ASSERT(4 + length() + macLength() >= currentDataSize());
    return 4 + length() + macLength() == currentDataSize();
}

void AbstractSshPacket::clear()
{
    m_data.clear();
    m_length = 0;
}

SshPacketType AbstractSshPacket::type() const
{
    Q_ASSERT(isComplete());
    return static_cast<SshPacketType>(m_data.at(TypeOffset));
}

AbstractSshPacket::Payload AbstractSshPacket::payLoad() const
{
    Payload p;
    p.data = m_data.constData() + PayloadOffset;
    p.size = length() - paddingLength() - 1;
    return p;
}

void AbstractSshPacket::printRawBytes() const
{
    printByteArray(m_data);
}

QByteArray AbstractSshPacket::encodeString(const QByteArray &string)
{
    QByteArray data;
    data.resize(4);
    data += string;
    setLengthField(data);
    return data;
}

QByteArray AbstractSshPacket::encodeMpInt(const Botan::BigInt &number)
{
    if (number.is_zero())
        return QByteArray(4, 0);

    int stringLength = number.bytes();
    const bool positiveAndMsbSet = number.sign() == Botan::BigInt::Positive
                                   && (number.byte_at(stringLength - 1) & 0x80);
    if (positiveAndMsbSet)
        ++stringLength;
    QByteArray data;
    data.resize(4 + stringLength);
    int pos = 4;
    if (positiveAndMsbSet)
        data[pos++] = '\0';
    number.binary_encode(reinterpret_cast<Botan::byte *>(data.data()) + pos);
    setLengthField(data);
    return data;
}

int AbstractSshPacket::paddingLength() const
{
    return m_data[PaddingLengthOffset];
}

quint32 AbstractSshPacket::length() const
{
    //Q_ASSERT(currentDataSize() >= minPacketSize());
    if (m_length == 0)
        calculateLength();
    return m_length;
}

void AbstractSshPacket::calculateLength() const
{
    m_length = SshPacketParser::asUint32(m_data, static_cast<quint32>(0));
}

QByteArray AbstractSshPacket::generateMac(const SshAbstractCryptoFacility &crypt,
    quint32 seqNr) const
{
    const quint32 seqNrBe = qToBigEndian(seqNr);
    QByteArray data(reinterpret_cast<const char *>(&seqNrBe), sizeof seqNrBe);
    data += QByteArray(m_data.constData(), length() + 4);
    return crypt.generateMac(data, data.size());
}

quint32 AbstractSshPacket::minPacketSize() const
{
    return qMax<quint32>(cipherBlockSize(), 16) + macLength();
}

void AbstractSshPacket::setLengthField(QByteArray &data)
{
    const quint32 length = qToBigEndian(data.size() - 4);
    data.replace(0, 4, reinterpret_cast<const char *>(&length), 4);
}

} // namespace Internal
} // namespace Utils
