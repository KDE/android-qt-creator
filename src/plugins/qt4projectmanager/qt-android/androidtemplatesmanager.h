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

class AndroidTemplatesManager : public QObject
{
    Q_OBJECT

public:
    static AndroidTemplatesManager *instance(QObject *parent = 0);
    QString packageName(const ProjectExplorer::Project *project);
    bool setPackageName(const ProjectExplorer::Project *project, const QString & name);

    QString applicationName(const ProjectExplorer::Project *project);
    bool setApplicationName(const ProjectExplorer::Project *project, const QString & name);

    QString targetSDK(const ProjectExplorer::Project *project);
    bool setTargetSDK(const ProjectExplorer::Project *project, const QString & target);

    int versionCode(const ProjectExplorer::Project *project);
    bool setVersionCode(const ProjectExplorer::Project *project, int version);

    QString versionName(const ProjectExplorer::Project *project);
    bool setVersionName(const ProjectExplorer::Project *project, const QString &version);

    QIcon packageManagerIcon(const ProjectExplorer::Project *project);
    bool setPackageManagerIcon(const ProjectExplorer::Project *project, const QString &iconFilePath);

    QString androidDirPath(const ProjectExplorer::Project *project);
    QString androidManifestPath(const ProjectExplorer::Project *project);

public slots:
    bool createAndroidTemplatesIfNecessary(const ProjectExplorer::Project *project);
    void updateProject(const ProjectExplorer::Project *project, const QString &targetSDK);

signals:
    void androidDirContentsChanged(const ProjectExplorer::Project *project);
    void changeLogChanged(const ProjectExplorer::Project *project);
    void controlChanged(const ProjectExplorer::Project *project);

private slots:
    void handleActiveProjectChanged(ProjectExplorer::Project *project);
    bool handleTarget(ProjectExplorer::Target *target);
    void handleAndroidDirContentsChanged();
    void handleProjectToBeRemoved(ProjectExplorer::Project *project);
    void handleProFileUpdated();

private:
    explicit AndroidTemplatesManager(QObject *parent);
    void raiseError(const QString &reason);
    bool openAndroidManifest(const ProjectExplorer::Project *project, QDomDocument & doc);
    bool saveAndroidManifest(const ProjectExplorer::Project *project, QDomDocument & doc);

    bool updateDesktopFiles(const Qt4Target *target);
    bool updateDesktopFile(const Qt4Target *target,
        Qt4ProFileNode *proFileNode);
    ProjectExplorer::Project *findProject(const QFileSystemWatcher *fsWatcher) const;
    void findLine(const QByteArray &string, QByteArray &document,
        int &lineEndPos, int &valuePos);
    bool adaptRulesFile(const ProjectExplorer::Project *project);
    bool adaptControlFile(const ProjectExplorer::Project *project);
    void adaptControlFileField(QByteArray &document, const QByteArray &fieldName,
        const QByteArray &newFieldValue);
    QSharedPointer<QFile> openFile(const QString &filePath,
        QIODevice::OpenMode mode, QString *error) const;

    static AndroidTemplatesManager *m_instance;

    typedef QMap<ProjectExplorer::Project *, QFileSystemWatcher *> AndroidProjectMap;
    AndroidProjectMap m_androidProjects;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDTEMPLATESCREATOR_H
