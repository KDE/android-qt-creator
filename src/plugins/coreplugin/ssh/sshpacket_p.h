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

#ifndef SSHPACKET_P_H
#define SSHPACKET_P_H

#include "sshexception_p.h"

#include <QtCore/QtEndian>
#include <QtCore/QByteArray>
#include <QtCore/QList>

#include <botan/bigint.h>

namespace Core {
namespace Internal {

enum SshPacketType {
    SSH_MSG_DISCONNECT = 1,
    SSH_MSG_IGNORE = 2,
    SSH_MSG_UNIMPLEMENTED = 3,
    SSH_MSG_DEBUG = 4,
    SSH_MSG_SERVICE_REQUEST = 5,
    SSH_MSG_SERVICE_ACCEPT = 6,

    SSH_MSG_KEXINIT = 20,
    SSH_MSG_NEWKEYS = 21,
    SSH_MSG_KEXDH_INIT = 30,
    SSH_MSG_KEXDH_REPLY = 31,

    SSH_MSG_USERAUTH_REQUEST = 50,
    SSH_MSG_USERAUTH_FAILURE = 51,
    SSH_MSG_USERAUTH_SUCCESS = 52,
    SSH_MSG_USERAUTH_BANNER = 53,
    SSH_MSG_USERAUTH_PK_OK = 60,
    SSH_MSG_USERAUTH_PASSWD_CHANGEREQ = 60,

    SSH_MSG_GLOBAL_REQUEST = 80,
    SSH_MSG_REQUEST_SUCCESS = 81,
    SSH_MSG_REQUEST_FAILURE = 82,

    SSH_MSG_CHANNEL_OPEN = 90,
    SSH_MSG_CHANNEL_OPEN_CONFIRMATION = 91,
    SSH_MSG_CHANNEL_OPEN_FAILURE = 92,
    SSH_MSG_CHANNEL_WINDOW_ADJUST = 93,
    SSH_MSG_CHANNEL_DATA = 94,
    SSH_MSG_CHANNEL_EXTENDED_DATA = 95,
    SSH_MSG_CHANNEL_EOF = 96,
    SSH_MSG_CHANNEL_CLOSE = 97,
    SSH_MSG_CHANNEL_REQUEST = 98,
    SSH_MSG_CHANNEL_SUCCESS = 99,
    SSH_MSG_CHANNEL_FAILURE = 100
};

enum SshExtendedDataType { SSH_EXTENDED_DATA_STDERR = 1 };

class SshAbstractCryptoFacility;

class AbstractSshPacket
{
public:
    virtual ~AbstractSshPacket();

    void clear();
    bool isComplete() const;
    SshPacketType type() const;

    static QByteArray encodeString(const QByteArray &string);
    static QByteArray encodeMpInt(const Botan::BigInt &number);
    template<typename T> static QByteArray encodeInt(T value)
    {
        const T valMsb = qToBigEndian(value);
        return QByteArray(reinterpret_cast<const char *>(&valMsb), sizeof valMsb);
    }

    static void setLengthField(QByteArray &data);

    void printRawBytes() const; // For Debugging.

    const QByteArray &rawData() const { return m_data; }

    struct Payload { const char *data; quint32 size; };
    Payload payLoad() const;

protected:
    AbstractSshPacket();

    virtual quint32 cipherBlockSize() const=0;
    virtual quint32 macLength() const=0;
    virtual void calculateLength() const;

    quint32 length() const;
    int paddingLength() const;
    quint32 minPacketSize() const;
    quint32 currentDataSize() const { return m_data.size(); }
    QByteArray generateMac(const SshAbstractCryptoFacility &crypt,
        quint32 seqNr) const;

    static const quint32 PaddingLengthOffset;
    static const quint32 PayloadOffset;
    static const quint32 TypeOffset;
    static const quint32 MinPaddingLength;

    mutable QByteArray m_data;
    mutable quint32 m_length;
};

} // namespace Internal
} // namespace Core

#endif // SSHPACKET_P_H
