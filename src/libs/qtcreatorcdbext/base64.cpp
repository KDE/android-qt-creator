/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "base64.h"

#include <sstream>

template <class OStream>
static void base64EncodeTriple(OStream &str, const unsigned char triple[3], size_t length = 3)
{
    static const char base64Encoding[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    // Convert 3 bytes of triplet into 4 bytes, write out 64 bit-wise using a character mapping,
    // see description of base64.
    unsigned tripleValue = triple[0] * 256;
    tripleValue += triple[1];
    tripleValue *= 256;
    tripleValue += triple[2];

    char result[4]= {0, 0, 0, 0};
    for (int i = 3; i >= 0 ; i--) {
        result[i] = base64Encoding[tripleValue % 64];
        tripleValue /= 64;
    }

    // Write out quad and pad if it is a padding triple.
    const size_t writeLength = length + 1;
    size_t i = 0;
    for ( ; i < writeLength; i++)
        str << result[i];
    for ( ; i < 4; i++)
        str << '=';
}

template <class OStream>
void base64EncodeHelper(OStream &str, const unsigned char *source, size_t sourcelen)
{
    if (!sourcelen) {
        str << "====";
        return;
    }
    /* Encode triples */
    const unsigned char *sourceEnd = source + sourcelen;
    for (const unsigned char *triple = source; triple < sourceEnd; ) {
        const unsigned char *nextTriple = triple + 3;
        if (nextTriple <= sourceEnd) { // Encode full triple, including very last one
            base64EncodeTriple(str, triple);
            triple = nextTriple;
        } else { // Past end and some padding required.
            unsigned char paddingTriple[3] = {0, 0, 0};
            std::copy(triple, sourceEnd, paddingTriple);
            base64EncodeTriple(str, paddingTriple, sourceEnd - triple);
            break;
        }
    }
}

void base64Encode(std::ostream &str, const unsigned char *source, size_t sourcelen)
{
    base64EncodeHelper(str, source, sourcelen);
}

std::string base64EncodeToString(const unsigned char *source, size_t sourcelen)
{
    std::ostringstream str;
    base64Encode(str, source, sourcelen);
    return str.str();
}

void base64EncodeW(std::wostream &str, const unsigned char *source, size_t sourcelen)
{
    base64EncodeHelper(str, source, sourcelen);
}

std::wstring base64EncodeToWString(const unsigned char *source, size_t sourcelen)
{
    std::wostringstream str;
    base64EncodeW(str, source, sourcelen);
    return str.str();
}
