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

#include "sshcryptofacility_p.h"

#include "sshbotanconversions_p.h"
#include "sshcapabilities_p.h"
#include "sshexception_p.h"
#include "sshkeyexchange_p.h"
#include "sshkeypasswordretriever_p.h"
#include "sshpacket_p.h"

#include <botan/ber_dec.h>
#include <botan/botan.h>
#include <botan/cbc.h>
#include <botan/dsa.h>
#include <botan/hash.h>
#include <botan/hmac.h>
#include <botan/look_pk.h>
#include <botan/pipe.h>
#include <botan/pkcs8.h>
#include <botan/pubkey.h>
#include <botan/rsa.h>

#include <QtCore/QDebug>
#include <QtCore/QList>

#include <string>

using namespace Botan;

namespace Utils {
namespace Internal {

SshAbstractCryptoFacility::SshAbstractCryptoFacility()
    : m_cipherBlockSize(0), m_macLength(0)
{
}

SshAbstractCryptoFacility::~SshAbstractCryptoFacility() {}

void SshAbstractCryptoFacility::clearKeys()
{
    m_cipherBlockSize = 0;
    m_macLength = 0;
    m_sessionId.clear();
    m_pipe.reset(0);
    m_hMac.reset(0);
}

void SshAbstractCryptoFacility::recreateKeys(const SshKeyExchange &kex)
{
    checkInvariant();

    if (m_sessionId.isEmpty())
        m_sessionId = kex.h();
    Algorithm_Factory &af = global_state().algorithm_factory();
    const std::string &cryptAlgo = botanCryptAlgoName(cryptAlgoName(kex));
    BlockCipher * const cipher = af.prototype_block_cipher(cryptAlgo)->clone();

    m_cipherBlockSize = cipher->BLOCK_SIZE;
    const QByteArray ivData = generateHash(kex, ivChar(), m_cipherBlockSize);
    const InitializationVector iv(convertByteArray(ivData), m_cipherBlockSize);

    const quint32 keySize = max_keylength_of(cryptAlgo);
    const QByteArray cryptKeyData = generateHash(kex, keyChar(), keySize);
    SymmetricKey cryptKey(convertByteArray(cryptKeyData), keySize);

    BlockCipherMode * const cipherMode
        = makeCipherMode(cipher, new Null_Padding, iv, cryptKey);
    m_pipe.reset(new Pipe(cipherMode));

    m_macLength = botanHMacKeyLen(hMacAlgoName(kex));
    const QByteArray hMacKeyData = generateHash(kex, macChar(), macLength());
    SymmetricKey hMacKey(convertByteArray(hMacKeyData), macLength());
    const HashFunction * const hMacProto
        = af.prototype_hash_function(botanHMacAlgoName(hMacAlgoName(kex)));
    m_hMac.reset(new HMAC(hMacProto->clone()));
    m_hMac->set_key(hMacKey);
}

void SshAbstractCryptoFacility::convert(QByteArray &data, quint32 offset,
    quint32 dataSize) const
{
    Q_ASSERT(offset + dataSize <= static_cast<quint32>(data.size()));
    checkInvariant();

    // Session id empty => No key exchange has happened yet.
    if (dataSize == 0 || m_sessionId.isEmpty())
        return;

    if (dataSize % cipherBlockSize() != 0) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid packet size");
    }
    m_pipe->process_msg(reinterpret_cast<const byte *>(data.constData()) + offset,
        dataSize);
    quint32 bytesRead = m_pipe->read(reinterpret_cast<byte *>(data.data()) + offset,
        dataSize, m_pipe->message_count() - 1); // Can't use Pipe::LAST_MESSAGE because of a VC bug.
    Q_ASSERT(bytesRead == dataSize);
}

QByteArray SshAbstractCryptoFacility::generateMac(const QByteArray &data,
    quint32 dataSize) const
{
    return m_sessionId.isEmpty()
        ? QByteArray()
        : convertByteArray(m_hMac->process(reinterpret_cast<const byte *>(data.constData()),
              dataSize));
}

QByteArray SshAbstractCryptoFacility::generateHash(const SshKeyExchange &kex,
    char c, quint32 length)
{
    const QByteArray &k = kex.k();
    const QByteArray &h = kex.h();
    QByteArray data(k);
    data.append(h).append(c).append(m_sessionId);
    SecureVector<byte> key
        = kex.hash()->process(convertByteArray(data), data.size());
    while (key.size() < length) {
        SecureVector<byte> tmpKey;
        tmpKey.append(convertByteArray(k), k.size());
        tmpKey.append(convertByteArray(h), h.size());
        tmpKey.append(key);
        key.append(kex.hash()->process(tmpKey));
    }
    return QByteArray(reinterpret_cast<const char *>(key.begin()), length);
}

void SshAbstractCryptoFacility::checkInvariant() const
{
    Q_ASSERT(m_sessionId.isEmpty() == !m_pipe);
}


const QByteArray SshEncryptionFacility::PrivKeyFileStartLineRsa("-----BEGIN RSA PRIVATE KEY-----");
const QByteArray SshEncryptionFacility::PrivKeyFileStartLineDsa("-----BEGIN DSA PRIVATE KEY-----");
const QByteArray SshEncryptionFacility::PrivKeyFileEndLineRsa("-----END RSA PRIVATE KEY-----");
const QByteArray SshEncryptionFacility::PrivKeyFileEndLineDsa("-----END DSA PRIVATE KEY-----");

QByteArray SshEncryptionFacility::cryptAlgoName(const SshKeyExchange &kex) const
{
    return kex.encryptionAlgo();
}

QByteArray SshEncryptionFacility::hMacAlgoName(const SshKeyExchange &kex) const
{
    return kex.hMacAlgoClientToServer();
}

BlockCipherMode *SshEncryptionFacility::makeCipherMode(BlockCipher *cipher,
    BlockCipherModePaddingMethod *paddingMethod, const InitializationVector &iv,
    const SymmetricKey &key)
{
    return new CBC_Encryption(cipher, paddingMethod, key, iv);
}

void SshEncryptionFacility::encrypt(QByteArray &data) const
{
    convert(data, 0, data.size());
}

void SshEncryptionFacility::createAuthenticationKey(const QByteArray &privKeyFileContents)
{
    if (privKeyFileContents == m_cachedPrivKeyContents)
        return;

#ifdef CREATOR_SSH_DEBUG
    qDebug("%s: Key not cached, reading", Q_FUNC_INFO);
#endif
    QList<BigInt> pubKeyParams;
    QList<BigInt> allKeyParams;
    try {
        createAuthenticationKeyFromPKCS8(privKeyFileContents, pubKeyParams,
            allKeyParams);
    } catch (Botan::Exception &) {
        createAuthenticationKeyFromOpenSSL(privKeyFileContents, pubKeyParams,
            allKeyParams);
    }

    foreach (const BigInt &b, allKeyParams) {
        if (b.is_zero()) {
            throw SshClientException(SshKeyFileError,
                SSH_TR("Decoding of private key file failed."));
        }
    }

    m_authPubKeyBlob = AbstractSshPacket::encodeString(m_authKeyAlgoName);
    foreach (const BigInt &b, pubKeyParams)
        m_authPubKeyBlob += AbstractSshPacket::encodeMpInt(b);
    m_cachedPrivKeyContents = privKeyFileContents;
}

void SshEncryptionFacility::createAuthenticationKeyFromPKCS8(const QByteArray &privKeyFileContents,
    QList<BigInt> &pubKeyParams, QList<BigInt> &allKeyParams)
{
    Pipe pipe;
    pipe.process_msg(convertByteArray(privKeyFileContents),
        privKeyFileContents.size());
    Private_Key * const key = PKCS8::load_key(pipe, m_rng, SshKeyPasswordRetriever());
    if (DSA_PrivateKey * const dsaKey = dynamic_cast<DSA_PrivateKey *>(key)) {
        m_authKeyAlgoName = SshCapabilities::PubKeyDss;
        m_authKey.reset(dsaKey);
        pubKeyParams << dsaKey->group_p() << dsaKey->group_q()
            << dsaKey->group_g() << dsaKey->get_y();
        allKeyParams << pubKeyParams << dsaKey->get_x();
    } else if (RSA_PrivateKey * const rsaKey = dynamic_cast<RSA_PrivateKey *>(key)) {
        m_authKeyAlgoName = SshCapabilities::PubKeyRsa;
        m_authKey.reset(rsaKey);
        pubKeyParams << rsaKey->get_e() << rsaKey->get_n();
        allKeyParams << pubKeyParams << rsaKey->get_p() << rsaKey->get_q()
            << rsaKey->get_d();
    } else {
        throw Botan::Exception();
    }
}

void SshEncryptionFacility::createAuthenticationKeyFromOpenSSL(const QByteArray &privKeyFileContents,
    QList<BigInt> &pubKeyParams, QList<BigInt> &allKeyParams)
{
    bool syntaxOk = true;
    QList<QByteArray> lines = privKeyFileContents.split('\n');
    while (lines.last().isEmpty())
        lines.removeLast();
    if (lines.count() < 3) {
        syntaxOk = false;
    } else if (lines.first() == PrivKeyFileStartLineRsa) {
        if (lines.last() != PrivKeyFileEndLineRsa)
            syntaxOk =false;
        else
            m_authKeyAlgoName = SshCapabilities::PubKeyRsa;
    } else if (lines.first() == PrivKeyFileStartLineDsa) {
        if (lines.last() != PrivKeyFileEndLineDsa)
            syntaxOk = false;
        else
            m_authKeyAlgoName = SshCapabilities::PubKeyDss;
    } else {
        syntaxOk = false;
    }
    if (!syntaxOk) {
        throw SshClientException(SshKeyFileError,
            SSH_TR("Private key file has unexpected format."));
    }

    QByteArray privateKeyBlob;
    for (int i = 1; i < lines.size() - 1; ++i)
        privateKeyBlob += lines.at(i);
    privateKeyBlob = QByteArray::fromBase64(privateKeyBlob);

    BER_Decoder decoder(convertByteArray(privateKeyBlob),
                        privateKeyBlob.size());
    BER_Decoder sequence = decoder.start_cons(SEQUENCE);
    quint32 version;
    sequence.decode (version);
    if (version != 0) {
        throw SshClientException(SshKeyFileError,
            SSH_TR("Private key encoding has version %1, expected 0.")
            .arg(version));
    }

    if (m_authKeyAlgoName == SshCapabilities::PubKeyDss) {
        BigInt p, q, g, y, x;
        sequence.decode (p).decode (q).decode (g).decode (y).decode (x);
        DSA_PrivateKey * const dsaKey
                = new DSA_PrivateKey(m_rng, DL_Group(p, q, g), x);
        m_authKey.reset(dsaKey);
        pubKeyParams << p << q << g << y;
        allKeyParams << pubKeyParams << x;
    } else {
        BigInt p, q, e, d, n;
        sequence.decode (n).decode (e).decode (d).decode (p).decode (q);
        RSA_PrivateKey * const rsaKey
                = new RSA_PrivateKey (m_rng, p, q, e, d, n);
        m_authKey.reset(rsaKey);
        pubKeyParams << e << n;
        allKeyParams << pubKeyParams << p << q << d;
    }

    sequence.discard_remaining();
    sequence.verify_end();
}

QByteArray SshEncryptionFacility::authenticationAlgorithmName() const
{
    Q_ASSERT(m_authKey);
    return m_authKeyAlgoName;
}

QByteArray SshEncryptionFacility::authenticationKeySignature(const QByteArray &data) const
{
    Q_ASSERT(m_authKey);

    QScopedPointer<PK_Signer> signer(get_pk_signer (*m_authKey,
        botanEmsaAlgoName(m_authKeyAlgoName)));
    QByteArray dataToSign = AbstractSshPacket::encodeString(sessionId()) + data;
    QByteArray signature
        = convertByteArray(signer->sign_message(convertByteArray(dataToSign),
              dataToSign.size(), m_rng));
    return AbstractSshPacket::encodeString(m_authKeyAlgoName)
        + AbstractSshPacket::encodeString(signature);
}

QByteArray SshEncryptionFacility::getRandomNumbers(int count) const
{
    QByteArray data;
    data.resize(count);
    m_rng.randomize(convertByteArray(data), count);
    return data;
}

SshEncryptionFacility::~SshEncryptionFacility() {}


QByteArray SshDecryptionFacility::cryptAlgoName(const SshKeyExchange &kex) const
{
    return kex.decryptionAlgo();
}

QByteArray SshDecryptionFacility::hMacAlgoName(const SshKeyExchange &kex) const
{
    return kex.hMacAlgoServerToClient();
}

BlockCipherMode *SshDecryptionFacility::makeCipherMode(BlockCipher *cipher,
    BlockCipherModePaddingMethod *paddingMethod, const InitializationVector &iv,
    const SymmetricKey &key)
{
    return new CBC_Decryption(cipher, paddingMethod, key, iv);
}

void SshDecryptionFacility::decrypt(QByteArray &data, quint32 offset,
    quint32 dataSize) const
{
    convert(data, offset, dataSize);
#ifdef CREATOR_SSH_DEBUG
    qDebug("Decrypted data:");
    const char * const start = data.constData() + offset;
    const char * const end = start + dataSize;
    for (const char *c = start; c < end; ++c)
        qDebug() << "'" << *c << "' (0x" << (static_cast<int>(*c) & 0xff) << ")";
#endif
}

} // namespace Internal
} // namespace Utils
