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

#ifndef PLUGINFILEFACTORY_H
#define PLUGINFILEFACTORY_H

#include <coreplugin/ifilefactory.h>

#include <QtCore/QStringList>

namespace ProjectExplorer {

class IProjectManager;
class ProjectExplorerPlugin;

namespace Internal {

class ProjectFileFactory : public Core::IFileFactory
{
    Q_OBJECT

    explicit ProjectFileFactory(ProjectExplorer::IProjectManager *manager);

public:
    virtual QStringList mimeTypes() const;
    bool canOpen(const QString &fileName);
    Core::Id id() const;
    QString displayName() const;

    Core::IFile *open(const QString &fileName);

    static QList<ProjectFileFactory*> createFactories(QString *filterString);

private:
    const QStringList m_mimeTypes;
    ProjectExplorer::IProjectManager *m_manager;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PLUGINFILEFACTORY
