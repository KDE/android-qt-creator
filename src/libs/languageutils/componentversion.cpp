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

#include "componentversion.h"

#include <QtCore/QString>

using namespace LanguageUtils;

const int ComponentVersion::NoVersion = -1;

ComponentVersion::ComponentVersion()
    : _major(NoVersion), _minor(NoVersion)
{
}

ComponentVersion::ComponentVersion(int major, int minor)
    : _major(major), _minor(minor)
{
}

ComponentVersion::ComponentVersion(const QString &versionString)
    : _major(NoVersion), _minor(NoVersion)
{
    int dotIdx = versionString.indexOf(QLatin1Char('.'));
    if (dotIdx == -1)
        return;
    bool ok = false;
    int maybeMajor = versionString.left(dotIdx).toInt(&ok);
    if (!ok)
        return;
    int maybeMinor = versionString.mid(dotIdx + 1).toInt(&ok);
    if (!ok)
        return;
    _major = maybeMajor;
    _minor = maybeMinor;
}

ComponentVersion::~ComponentVersion()
{
}

bool ComponentVersion::isValid() const
{
    return _major >= 0 && _minor >= 0;
}

QString ComponentVersion::toString() const
{
    return QString("%1.%2").arg(QString::number(_major),
                                QString::number(_minor));
}

namespace LanguageUtils {

bool operator<(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return lhs.majorVersion() < rhs.majorVersion()
            || (lhs.majorVersion() == rhs.majorVersion() && lhs.minorVersion() < rhs.minorVersion());
}

bool operator<=(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return lhs.majorVersion() < rhs.majorVersion()
            || (lhs.majorVersion() == rhs.majorVersion() && lhs.minorVersion() <= rhs.minorVersion());
}

bool operator==(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return lhs.majorVersion() == rhs.majorVersion() && lhs.minorVersion() == rhs.minorVersion();
}

bool operator!=(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return !(lhs == rhs);
}

}
