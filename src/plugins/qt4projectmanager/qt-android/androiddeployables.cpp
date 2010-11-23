/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "androiddeployables.h"

#include "androidprofilesupdatedialog.h"

#include <profileevaluator.h>
#include <projectexplorer/buildstep.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>

#include <QtCore/QTimer>

namespace Qt4ProjectManager {
namespace Internal {

AndroidDeployables::AndroidDeployables(const ProjectExplorer::BuildStep *buildStep)
    : m_buildStep(buildStep), m_updateTimer(new QTimer(this))
{
    QTimer::singleShot(0, this, SLOT(init()));
    m_updateTimer->setInterval(1500);
    connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(createModels()));
}

AndroidDeployables::~AndroidDeployables() {}

void AndroidDeployables::init()
{
    Qt4Project *pro = qt4BuildConfiguration()->qt4Target()->qt4Project();
    connect(pro, SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)),
            m_updateTimer, SLOT(start()));

    // TODO do we want to disable the view

    createModels();
}

void AndroidDeployables::createModels()
{
    if (!qt4BuildConfiguration() || !qt4BuildConfiguration()->qt4Target()
        || qt4BuildConfiguration()->qt4Target()->project()->activeTarget()->id()
            != QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
        return;
    const Qt4ProFileNode *const rootNode
        = qt4BuildConfiguration()->qt4Target()->qt4Project()->rootProjectNode();
    if (!rootNode) // Happens on project creation by wizard.
        return;
    m_updateTimer->stop();
    disconnect(qt4BuildConfiguration()->qt4Target()->qt4Project(),
        SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)),
        m_updateTimer, SLOT(start()));
    beginResetModel();
    qDeleteAll(m_listModels);
    m_listModels.clear();
    createModels(rootNode);
    QList<AndroidDeployableListModel *> modelsWithoutTargetPath;
    foreach (AndroidDeployableListModel *const model, m_listModels) {
        if (!model->hasTargetPath()) {
            if (model->proFileUpdateSetting() == AndroidDeployableListModel::AskToUpdateProFile)
                modelsWithoutTargetPath << model;
        }
    }

    if (!modelsWithoutTargetPath.isEmpty()) {
        AndroidProFilesUpdateDialog dialog(modelsWithoutTargetPath);
        dialog.exec();
        const QList<AndroidProFilesUpdateDialog::UpdateSetting> &settings
            = dialog.getUpdateSettings();
        foreach (const AndroidProFilesUpdateDialog::UpdateSetting &setting, settings) {
            const AndroidDeployableListModel::ProFileUpdateSetting updateSetting
                = setting.second
                    ? AndroidDeployableListModel::UpdateProFile
                    : AndroidDeployableListModel::DontUpdateProFile;
            m_updateSettings.insert(setting.first->proFilePath(),
                updateSetting);
            setting.first->setProFileUpdateSetting(updateSetting);
        }
    }

    endResetModel();
    connect(qt4BuildConfiguration()->qt4Target()->qt4Project(),
            SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)),
            m_updateTimer, SLOT(start()));
}

void AndroidDeployables::createModels(const Qt4ProFileNode *proFileNode)
{
    switch (proFileNode->projectType()) {
    case ApplicationTemplate:
    case LibraryTemplate:
    case ScriptTemplate: {
        UpdateSettingsMap::ConstIterator it
            = m_updateSettings.find(proFileNode->path());
        const AndroidDeployableListModel::ProFileUpdateSetting updateSetting
            = it != m_updateSettings.end()
                  ? it.value() : AndroidDeployableListModel::AskToUpdateProFile;
        AndroidDeployableListModel *const newModel
            = new AndroidDeployableListModel(proFileNode, updateSetting, this);
        m_listModels << newModel;
        break;
    }
    case SubDirsTemplate: {
        const QList<ProjectExplorer::ProjectNode *> &subProjects
            = proFileNode->subProjectNodes();
        foreach (const ProjectExplorer::ProjectNode *subProject, subProjects) {
            const Qt4ProFileNode * const qt4SubProject
                = qobject_cast<const Qt4ProFileNode *>(subProject);
            if (qt4SubProject && !qt4SubProject->path()
                .endsWith(QLatin1String(".pri")))
                createModels(qt4SubProject);
        }
    }
    default:
        break;
    }
}

void AndroidDeployables::setUnmodified()
{
    foreach (AndroidDeployableListModel *model, m_listModels)
        model->setUnModified();
}

bool AndroidDeployables::isModified() const
{
    foreach (const AndroidDeployableListModel *model, m_listModels) {
        if (model->isModified())
            return true;
    }
    return false;
}

int AndroidDeployables::deployableCount() const
{
    int count = 0;
    foreach (const AndroidDeployableListModel *model, m_listModels)
        count += model->rowCount();
    return count;
}

AndroidDeployable AndroidDeployables::deployableAt(int i) const
{
    foreach (const AndroidDeployableListModel *model, m_listModels) {
        Q_ASSERT(i >= 0);
        if (i < model->rowCount())
            return model->deployableAt(i);
        i -= model->rowCount();
    }

    Q_ASSERT(!"Invalid deployable number");
    return AndroidDeployable(QString(), QString());
}

QString AndroidDeployables::remoteExecutableFilePath(const QString &localExecutableFilePath) const
{
    foreach (const AndroidDeployableListModel *model, m_listModels) {
        if (model->localExecutableFilePath() == localExecutableFilePath)
            return model->remoteExecutableFilePath();
    }
    return QString();
}

const Qt4BuildConfiguration *AndroidDeployables::qt4BuildConfiguration() const
{
    const Qt4BuildConfiguration * const bc
        = qobject_cast<Qt4BuildConfiguration *>(m_buildStep->target()->activeBuildConfiguration());
    return bc;
}

int AndroidDeployables::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : modelCount();
}

QVariant AndroidDeployables::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= modelCount()
            || index.column() != 0)
        return QVariant();
    const AndroidDeployableListModel *const model = m_listModels.at(index.row());
    if (role == Qt::ForegroundRole && !model->hasTargetPath()) {
        QBrush brush;
        brush.setColor(Qt::red);
        return brush;
    }
    if (role == Qt::DisplayRole)
        return QFileInfo(model->proFilePath()).fileName();
    return QVariant();
}

} // namespace Qt4ProjectManager
} // namespace Internal
