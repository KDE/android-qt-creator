/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qt4desktoptarget.h"
#include "qt4project.h"
#include "qt4runconfiguration.h"
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <QtGui/QApplication>
#include <QtGui/QStyle>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

Qt4DesktopTarget::Qt4DesktopTarget(Qt4Project *parent, const QString &id) :
    Qt4BaseTarget(parent, id),
    m_buildConfigurationFactory(new Qt4BuildConfigurationFactory(this)),
    m_deployConfigurationFactory(new ProjectExplorer::DeployConfigurationFactory(this))
{
    setDisplayName(defaultDisplayName());
    setIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));
}

Qt4DesktopTarget::~Qt4DesktopTarget()
{
}

QString Qt4DesktopTarget::defaultDisplayName()
{
    return QApplication::translate("Qt4ProjectManager::Qt4Target", "Desktop", "Qt4 Desktop target display name");
}

Qt4BuildConfigurationFactory *Qt4DesktopTarget::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

ProjectExplorer::DeployConfigurationFactory *Qt4DesktopTarget::deployConfigurationFactory() const
{
    return m_deployConfigurationFactory;
}

void Qt4DesktopTarget::createApplicationProFiles()
{
    removeUnconfiguredCustomExectutableRunConfigurations();

    // We use the list twice
    QList<Qt4ProFileNode *> profiles = qt4Project()->applicationProFiles();
    QSet<QString> paths;
    foreach (Qt4ProFileNode *pro, profiles)
        paths << pro->path();

    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
        if (Qt4RunConfiguration *qt4rc = qobject_cast<Qt4RunConfiguration *>(rc)) {
            paths.remove(qt4rc->proFilePath());
        }

    // Only add new runconfigurations if there are none.
    foreach (const QString &path, paths)
        addRunConfiguration(new Qt4RunConfiguration(this, path));

    // Oh still none? Add a custom executable runconfiguration
    if (runConfigurations().isEmpty()) {
        addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(this));
    }
}
