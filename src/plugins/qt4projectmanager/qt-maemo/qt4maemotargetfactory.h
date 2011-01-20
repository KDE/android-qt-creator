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

#ifndef QT4MAEMOTARGETFACTORY_H
#define QT4MAEMOTARGETFACTORY_H

#include "qt4target.h"

namespace Qt4ProjectManager {
namespace Internal {

class Qt4MaemoTargetFactory : public Qt4BaseTargetFactory
{
    Q_OBJECT
public:
    Qt4MaemoTargetFactory(QObject *parent = 0);
    ~Qt4MaemoTargetFactory();

    QStringList supportedTargetIds(ProjectExplorer::Project *parent) const;
    QString displayNameForId(const QString &id) const;

    bool canCreate(ProjectExplorer::Project *parent, const QString &id) const;
    bool canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const;
    Qt4ProjectManager::Qt4BaseTarget *restore(ProjectExplorer::Project *parent, const QVariantMap &map);
    QString defaultShadowBuildDirectory(const QString &projectLocation, const QString &id);

    virtual bool supportsTargetId(const QString &id) const;

    QList<BuildConfigurationInfo> availableBuildConfigurations(const QString &proFilePath);
    Qt4BaseTarget *create(ProjectExplorer::Project *parent, const QString &id);
    Qt4BaseTarget *create(ProjectExplorer::Project *parent, const QString &id, QList<BuildConfigurationInfo> infos);

private:
    QList<BuildConfigurationInfo> availableBuildConfigurations(const QString &proFilePath,
        const QString &id);
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4MAEMOTARGETFACTORY_H
