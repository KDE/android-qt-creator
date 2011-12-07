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

#ifndef AUTOTOOLSMANAGER_H
#define AUTOTOOLSMANAGER_H

#include <projectexplorer/iprojectmanager.h>

namespace AutotoolsProjectManager {
namespace Internal {

/**
 * @brief Implementation of ProjectExplorer::IProjectManager interface.
 *
 * An autotools project is identified by the MIME type text/x-makefile.
 * The project is represented by an instance of ProjectExplorer::Project,
 * which gets created by AutotoolsManager::openProject().
 */
class AutotoolsManager : public ProjectExplorer::IProjectManager
{
    Q_OBJECT

public:
    AutotoolsManager() {}

    ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString);
    QString mimeType() const;
};

} // namespace Internal
} // namespace AutotoolsProjectManager

#endif // AUTOTOOLSMANAGER_H
