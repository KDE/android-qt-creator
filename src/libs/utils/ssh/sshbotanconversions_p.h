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

#ifndef BYTEARRAYCONVERSIONS_P_H
#define BYTEARRAYCONVERSIONS_P_H

#include "sshcapabilities_p.h"

#include <botan/rng.h>
#include <botan/secmem.h>

namespace Utils {
namespace Internal {

inline const Botan::byte *convertByteArray(const QByteArray &a)
{
    return reinterpret_cast<const Botan::byte *>(a.constData());
}

inline Botan::byte *convertByteArray(QByteArray &a)
{
    return reinterpret_cast<Botan::byte *>(a.data());
}

inline QByteArray convertByteArray(const Botan::SecureVector<Botan::byte> &v)
{
    return QByteArray(reinterpret_cast<const char *>(v.begin()), v.size());
}

inline const char *botanKeyExchangeAlgoName(const QByteArray &rfcAlgoName)
{
    Q_ASSERT(rfcAlgoName == SshCapabilities::DiffieHellmanGroup1Sha1
        || rfcAlgoName == SshCapabilities::DiffieHellmanGroup14Sha1);
    return rfcAlgoName == SshCapabilities::DiffieHellmanGroup1Sha1
        ? "modp/ietf/1024" : "modp/ietf/2048";
}

inline const char *botanCryptAlgoName(const QByteArray &rfcAlgoName)
{
    Q_ASSERT(rfcAlgoName == SshCapabilities::CryptAlgo3Des
        || rfcAlgoName == SshCapabilities::CryptAlgoAes128);
    return rfcAlgoName == SshCapabilities::CryptAlgo3Des
        ? "TripleDES" : "AES-128";
}

inline const char *botanEmsaAlgoName(const QByteArray &rfcAlgoName)
{
    Q_ASSERT(rfcAlgoName == SshCapabilities::PubKeyDss
        || rfcAlgoName == SshCapabilities::PubKeyRsa);
    return rfcAlgoName == SshCapabilities::PubKeyDss
        ? "EMSA1(SHA-1)" : "EMSA3(SHA-1)";
}

inline const char *botanSha1Name() { return "SHA-1"; }

inline const char *botanHMacAlgoName(const QByteArray &rfcAlgoName)
{
    Q_ASSERT(rfcAlgoName == SshCapabilities::HMacSha1);
    return botanSha1Name();
}

inline quint32 botanHMacKeyLen(const QByteArray &rfcAlgoName)
{
    Q_ASSERT(rfcAlgoName == SshCapabilities::HMacSha1);
    return 20;
}

} // namespace Internal
} // namespace Utils

#endif // BYTEARRAYCONVERSIONS_P_H
