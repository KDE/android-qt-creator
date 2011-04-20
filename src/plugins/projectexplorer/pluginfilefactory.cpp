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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "pluginfilefactory.h"
#include "projectexplorer.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "iprojectmanager.h"
#include "session.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/messagemanager.h>

#include <QtCore/QDebug>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

ProjectFileFactory::ProjectFileFactory(IProjectManager *manager)
  : m_mimeTypes(manager->mimeType()),
    m_manager(manager)
{
}

QStringList ProjectFileFactory::mimeTypes() const
{
    return m_mimeTypes;
}

QString ProjectFileFactory::id() const
{
    return QLatin1String(Constants::FILE_FACTORY_ID);
}

QString ProjectFileFactory::displayName() const
{
    return tr("Project File Factory", "ProjectExplorer::ProjectFileFactory display name.");
}

Core::IFile *ProjectFileFactory::open(const QString &fileName)
{
    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
    pe->openProject(fileName);
    return 0;
}

QList<ProjectFileFactory *> ProjectFileFactory::createFactories(QString *filterString)
{
    // Register factories for all project managers
    QList<Internal::ProjectFileFactory*> rc;
    QList<IProjectManager*> projectManagers =
        ExtensionSystem::PluginManager::instance()->getObjects<IProjectManager>();

    QList<Core::MimeGlobPattern> allGlobPatterns;

    const QString filterSeparator = QLatin1String(";;");
    filterString->clear();
    foreach (IProjectManager *manager, projectManagers) {
        rc.push_back(new ProjectFileFactory(manager));
        if (!filterString->isEmpty())
            *filterString += filterSeparator;
        const QString mimeType = manager->mimeType();
        Core::MimeType mime = Core::ICore::instance()->mimeDatabase()->findByType(mimeType);
        const QString pFilterString = mime.filterString();
        allGlobPatterns.append(mime.globPatterns());
        *filterString += pFilterString;
    }
    QString allProjectFilter = Core::MimeType::formatFilterString(tr("All Projects"), allGlobPatterns);
    allProjectFilter.append(filterSeparator);
    filterString->prepend(allProjectFilter);
    return rc;
}
