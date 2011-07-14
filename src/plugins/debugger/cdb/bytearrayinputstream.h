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

#ifndef BYTEARRAYINPUTSTREAM_H
#define BYTEARRAYINPUTSTREAM_H

#include <QtCore/QByteArray>
#include <QtCore/QString>

namespace Debugger {
namespace Internal {

class ByteArrayInputStream
{
    Q_DISABLE_COPY(ByteArrayInputStream)
public:
    typedef void (ModifierFunc)(ByteArrayInputStream &s);

    explicit ByteArrayInputStream(QByteArray &ba);

    ByteArrayInputStream &operator<<(char a)              { m_target.append(a); return *this; }
    ByteArrayInputStream &operator<<(const QByteArray &a) { m_target.append(a); return *this; }
    ByteArrayInputStream &operator<<(const char *a)       { m_target.append(a); return *this; }
    ByteArrayInputStream &operator<<(const QString &a)    { m_target.append(a.toLatin1()); return *this; }

    ByteArrayInputStream &operator<<(int i) { appendInt(i); return *this; }
    ByteArrayInputStream &operator<<(unsigned i) { appendInt(i); return *this; }
    ByteArrayInputStream &operator<<(quint64 i) { appendInt(i); return *this; }
    ByteArrayInputStream &operator<<(qint64 i) { appendInt(i); return *this; }

    // Stream a modifier by invoking it
    ByteArrayInputStream &operator<<(ModifierFunc mf) { mf(*this); return *this; }

    void setHexPrefix(bool hp) { m_hexPrefix = hp; }
    bool hexPrefix() const     { return  m_hexPrefix; }
    void setIntegerBase(int b) { m_integerBase = b; }
    int integerBase() const    { return m_integerBase; }
    // Append a separator if required (target does not end with it)
    void appendSeparator(char c = ' ');

private:
    template <class IntType> void appendInt(IntType i);

    QByteArray &m_target;
    int m_integerBase;
    bool m_hexPrefix;
    int m_width;
};

template <class IntType>
void ByteArrayInputStream::appendInt(IntType i)
{
    const bool hexPrefix = m_integerBase == 16 && m_hexPrefix;
    if (hexPrefix)
        m_target.append("0x");
    const QByteArray n = QByteArray::number(i, m_integerBase);
    if (m_width > 0) {
        int pad = m_width - n.size();
        if (hexPrefix)
            pad -= 2;
        if (pad > 0)
            m_target.append(QByteArray(pad, '0'));
    }
    m_target.append(n);
}

// Streamable modifiers for ByteArrayInputStream
void hexPrefixOn(ByteArrayInputStream &bs);
void hexPrefixOff(ByteArrayInputStream &bs);
void hex(ByteArrayInputStream &bs);
void dec(ByteArrayInputStream &bs);
void blankSeparator(ByteArrayInputStream &bs);

// Bytearray parse helpers
QByteArray trimFront(QByteArray in);
QByteArray trimBack(QByteArray in);
QByteArray simplify(const QByteArray &inIn);

} // namespace Internal
} // namespace Debugger

#endif // BYTEARRAYINPUTSTREAM_H
