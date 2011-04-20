/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
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

#include "classviewsymbolinformation.h"
#include "classviewutils.h"

#include <QtCore/QPair>
#include <QtCore/QHash>

namespace ClassView {
namespace Internal {

SymbolInformation::SymbolInformation() :
    m_iconType(INT_MIN),
    m_hash(0)
{
}

SymbolInformation::SymbolInformation(const QString &valueName, const QString &valueType,
                                     int valueIconType) :
    m_iconType(valueIconType),
    m_name(valueName),
    m_type(valueType)
{
    // calculate hash
    m_hash = qHash(qMakePair(m_iconType, qMakePair(m_name, m_type)));
}

int SymbolInformation::iconTypeSortOrder() const
{
    return Utils::iconTypeSortOrder(m_iconType);
}

bool SymbolInformation::operator<(const SymbolInformation &other) const
{
    // comparsion is not a critical for speed
    if (iconType() != other.iconType()) {
        int l = iconTypeSortOrder();
        int r = other.iconTypeSortOrder();
        if (l < r)
            return true;
        if (l > r)
            return false;
    }

    int cmp = name().compare(other.name(), Qt::CaseInsensitive);
    if (cmp < 0)
        return true;
    if (cmp > 0)
        return false;
    return type().compare(other.type(), Qt::CaseInsensitive) < 0;
}

} // namespace Internal
} // namespace ClassView
