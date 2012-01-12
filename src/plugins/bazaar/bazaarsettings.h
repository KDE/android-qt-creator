/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
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

#ifndef BAZAARSETTINGS_H
#define BAZAARSETTINGS_H

#include <vcsbase/vcsbaseclientsettings.h>

namespace Bazaar {
namespace Internal {

class BazaarSettings : public VcsBase::VcsBaseClientSettings
{
public:
    static const QLatin1String diffIgnoreWhiteSpaceKey;
    static const QLatin1String diffIgnoreBlankLinesKey;
    static const QLatin1String logVerboseKey;
    static const QLatin1String logForwardKey;
    static const QLatin1String logIncludeMergesKey;
    static const QLatin1String logFormatKey;

    BazaarSettings();
    bool sameUserId(const BazaarSettings &other) const;
};

} // namespace Internal
} // namespace Bazaar

#endif // BAZAARSETTINGS_H
