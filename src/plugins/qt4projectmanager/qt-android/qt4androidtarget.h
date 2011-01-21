/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef ANDROIDTEMPLATESCREATOR_H
#define ANDROIDTEMPLATESCREATOR_H

#include "qt4target.h"

#include <QtCore/QFile>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtGui/QIcon>
#include <QDomDocument>

QT_FORWARD_DECLARE_CLASS(QFileSystemWatcher);

namespace ProjectExplorer {
class Project;
class ProjectNode;
class Target;
}

namespace Qt4ProjectManager {

class Qt4Project;
class Qt4Target;

namespace Internal {
class Qt4ProFileNode;
class Qt4AndroidDeployConfigurationFactory;
class Qt4AndroidTargetFactory;

class Qt4AndroidTarget  : public Qt4BaseTarget
{
    friend class Qt4AndroidTargetFactory;
    Q_OBJECT
public:
    explicit Qt4AndroidTarget(Qt4Project *parent, const QString &id);
    virtual ~Qt4AndroidTarget();

    Internal::Qt4BuildConfigurationFactory *buildConfigurationFactory() const;
    ProjectExplorer::DeployConfigurationFactory *deployConfigurationFactory() const;
    QString defaultBuildDirectory() const;
    void createApplicationProFiles();

    static QString defaultDisplayName();


    QString packageName();
    bool setPackageName(const QString & name);

    QString intentName();
    QString activityName();

    QString applicationName();
    bool setApplicationName(const QString & name);

    QString targetSDK();
    bool setTargetSDK(const QString & target);

    int versionCode();
    bool setVersionCode(int version);

    QString versionName();
    bool setVersionName(const QString &version);

    QStringList permissions();
    bool setPermissions(const QStringList &permissions);

    QStringList availableQtLibs();
    QStringList qtLibs();
    bool setQtLibs(const QStringList & qtLibs);

    QStringList availablePrebundledLibs();
    QStringList prebundledLibs();
    bool setPrebundledLibs(const QStringList & qtLibs);


    QIcon packageManagerIcon();
    bool setPackageManagerIcon(const QString &iconFilePath);

    QString androidDirPath();
    QString androidManifestPath();
    QString androidDefaultPropertiesPath();


public slots:
    bool createAndroidTemplatesIfNecessary(bool forceJava=false);
    void updateProject(const QString &targetSDK);

signals:
    void androidDirContentsChanged();
    void changeLogChanged();
    void controlChanged();

private slots:
    void handleTargetAdded(ProjectExplorer::Target *target);
    void handleTargetToBeRemoved(ProjectExplorer::Target *target);

private:
    void raiseError(const QString &reason);
    bool openAndroidManifest(QDomDocument & doc);
    bool saveAndroidManifest(QDomDocument & doc);


    QFileSystemWatcher * const m_androidFilesWatcher;

    Qt4BuildConfigurationFactory *m_buildConfigurationFactory;
    Qt4AndroidDeployConfigurationFactory *m_deployConfigurationFactory;

};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDTEMPLATESCREATOR_H
