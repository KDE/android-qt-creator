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

#include "sftppacket_p.h"

#include "sshpacketparser_p.h"

namespace Utils {
namespace Internal {

const quint32 AbstractSftpPacket::MaxDataSize = 32000;
const quint32 AbstractSftpPacket::MaxPacketSize = 34000;
const int AbstractSftpPacket::TypeOffset = 4;
const int AbstractSftpPacket::RequestIdOffset = TypeOffset + 1;
const int AbstractSftpPacket::PayloadOffset = RequestIdOffset + 4;

AbstractSftpPacket::AbstractSftpPacket()
{
}

quint32 AbstractSftpPacket::requestId() const
{
    return SshPacketParser::asUint32(m_data, RequestIdOffset);
}

} // namespace Internal
} // namespace Utils
