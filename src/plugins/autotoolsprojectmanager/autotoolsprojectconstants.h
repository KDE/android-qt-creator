/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010-2011 Openismus GmbH.
**   Authors: Peter Penz (ppenz@openismus.com)
**            Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#ifndef AUTOTOOLSPROJECTCONSTANTS_H
#define AUTOTOOLSPROJECTCONSTANTS_H

namespace AutotoolsProjectManager {
/**
 * Collects project constants, that are shared between several classes.
 */
namespace Constants {
    const char MAKEFILE_MIMETYPE[] = "text/x-makefile";

//BuildConfiguration
    const char AUTOTOOLS_BC_ID[] = "AutotoolsProjectManager.AutotoolsBuildConfiguration";
    const char BUILD_DIRECTORY_KEY[] = "AutotoolsProjectManager.AutotoolsBuildConfiguration.BuildDirectory";

//Target
    const char DEFAULT_AUTOTOOLS_TARGET_ID[] = "AutotoolsProjectManager.DefaultAutotoolsTarget";

//Project
    const char AUTOTOOLS_PROJECT_ID[] = "AutotoolsProjectManager.AutotoolsProject";
    const char PROJECT_CONTEXT[] = "AutotoolsProject.ProjectContext";
} // namespace Constants
} // namespace AutotoolsProjectManager

#endif // AUTOTOOLSPROJECTCONSTANTS_H
