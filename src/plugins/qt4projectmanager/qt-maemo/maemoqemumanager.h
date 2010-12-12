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

#ifndef QEMURUNTIMEMANAGER_H
#define QEMURUNTIMEMANAGER_H

#include "maemoconstants.h"
#include "maemoqemuruntime.h"

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QProcess>

#include <QtGui/QIcon>

QT_FORWARD_DECLARE_CLASS(QAction);
QT_FORWARD_DECLARE_CLASS(QFileSystemWatcher)
QT_FORWARD_DECLARE_CLASS(QStringList);

namespace ProjectExplorer {
    class BuildConfiguration;
    class Project;
    class RunConfiguration;
    class Target;
}

namespace Qt4ProjectManager {
    class QtVersion;
    namespace Internal {
    class MaemoRunConfiguration;


class MaemoQemuManager : public QObject
{
    Q_OBJECT

public:
    static MaemoQemuManager& instance(QObject *parent = 0);

    bool runtimeForQtVersion(int uniqueId, MaemoQemuRuntime *rt) const;

signals:
    void qemuProcessStatus(QemuStatus, const QString &error = QString());

private slots:
    void qtVersionsChanged(const QList<int> &uniqueIds);

    void projectAdded(ProjectExplorer::Project *project);
    void projectRemoved(ProjectExplorer::Project *project);
    void projectChanged(ProjectExplorer::Project *project);

    void targetAdded(ProjectExplorer::Target *target);
    void targetRemoved(ProjectExplorer::Target *target);
    void targetChanged(ProjectExplorer::Target *target);

    void runConfigurationAdded(ProjectExplorer::RunConfiguration *rc);
    void runConfigurationRemoved(ProjectExplorer::RunConfiguration *rc);
    void runConfigurationChanged(ProjectExplorer::RunConfiguration *rc);

    void buildConfigurationAdded(ProjectExplorer::BuildConfiguration *bc);
    void buildConfigurationRemoved(ProjectExplorer::BuildConfiguration *bc);
    void buildConfigurationChanged(ProjectExplorer::BuildConfiguration *bc);

    void environmentChanged();  // needed to check for qt version
    void deviceConfigurationChanged(ProjectExplorer::Target *target);

    void startRuntime();
    void terminateRuntime();

    void qemuProcessFinished();
    void qemuProcessError(QProcess::ProcessError error);
    void qemuStatusChanged(QemuStatus status, const QString &error);
    void qemuOutput();

    void runtimeRootChanged(const QString &directory);
    void runtimeFolderChanged(const QString &directory);

private:
    MaemoQemuManager(QObject *parent);
    ~MaemoQemuManager();

    bool sessionHasMaemoTarget() const;

    void updateStarterIcon(bool running);
    void toggleStarterButton(ProjectExplorer::Target *target);
    bool targetUsesMatchingRuntimeConfig(ProjectExplorer::Target *target,
        QtVersion **qtVersion = 0);

    void notify(const QList<int> uniqueIds);
    void toggleDeviceConnections(MaemoRunConfiguration *mrc, bool connect);

private:
    QAction *m_qemuAction;
    QProcess *m_qemuProcess;

    int m_runningQtId;
    bool m_userTerminated;
    QIcon m_qemuStarterIcon;
    QMap<int, MaemoQemuRuntime> m_runtimes;
    static MaemoQemuManager *m_instance;
    QFileSystemWatcher *m_runtimeRootWatcher;
    QFileSystemWatcher *m_runtimeFolderWatcher;
};

}   // namespace Internal
}   // namespace Qt4ProjectManager

#endif  // QEMURUNTIMEMANAGER_H
