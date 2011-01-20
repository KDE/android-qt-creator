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

#ifndef QMLPROJECTTARGET_H
#define QMLPROJECTTARGET_H

#include <projectexplorer/target.h>

#include <QtCore/QStringList>
#include <QtCore/QVariantMap>

namespace QmlProjectManager {
class QmlProject;
class QmlProjectRunConfiguration;

namespace Internal {

class QmlProjectTargetFactory;

class QmlProjectTarget : public ProjectExplorer::Target
{
    Q_OBJECT
    friend class QmlProjectTargetFactory;

public:
    explicit QmlProjectTarget(QmlProject *parent);
    ~QmlProjectTarget();

    ProjectExplorer::BuildConfigWidget *createConfigWidget();

    QmlProject *qmlProject() const;

    ProjectExplorer::IBuildConfigurationFactory *buildConfigurationFactory() const;
    ProjectExplorer::DeployConfigurationFactory *deployConfigurationFactory() const;

protected:
    bool fromMap(const QVariantMap &map);
};

class QmlProjectTargetFactory : public ProjectExplorer::ITargetFactory
{
    Q_OBJECT

public:
    explicit QmlProjectTargetFactory(QObject *parent = 0);
    ~QmlProjectTargetFactory();

    bool supportsTargetId(const QString &id) const;
    QStringList supportedTargetIds(ProjectExplorer::Project *parent) const;
    QString displayNameForId(const QString &id) const;

    bool canCreate(ProjectExplorer::Project *parent, const QString &id) const;
    QmlProjectTarget *create(ProjectExplorer::Project *parent, const QString &id);
    bool canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const;
    QmlProjectTarget *restore(ProjectExplorer::Project *parent, const QVariantMap &map);
};

} // namespace Internal
} // namespace QmlProjectManager

#endif // QMLPROJECTTARGET_H
