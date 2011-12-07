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

#ifndef VCSBASE_CONSTANTS_H
#define VCSBASE_CONSTANTS_H

#include <QtCore/QtGlobal>

namespace VCSBase {
namespace Constants {

const char VCS_SETTINGS_CATEGORY[] = "V.Version Control";
const char VCS_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("VCSBase", "Version Control");
const char SETTINGS_CATEGORY_VCS_ICON[] = ":/core/images/category_vcs.png";
const char VCS_COMMON_SETTINGS_ID[] = "A.Common";
const char VCS_COMMON_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("VCSBase", "Common");

// Ids for sort order (wizards and preferences)
const char VCS_ID_BAZAAR[] = "B.Bazaar";
const char VCS_ID_GIT[] = "G.Git";
const char VCS_ID_MERCURIAL[] = "H.Mercurial";
const char VCS_ID_SUBVERSION[] = "J.Subversion";
const char VCS_ID_PERFORCE[] = "P.Perforce";
const char VCS_ID_CVS[] = "Z.CVS";

namespace Internal {
    enum { debug = 0 };
} // namespace Internal

} // namespace Constants
} // namespace VCSBase

#endif // VCSBASE_CONSTANTS_H
