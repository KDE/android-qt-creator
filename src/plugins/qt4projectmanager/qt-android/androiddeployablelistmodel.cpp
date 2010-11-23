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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "androiddeployablelistmodel.h"

#include "androidtoolchain.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4target.h>

#include <utils/qtcassert.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QBrush>

namespace Qt4ProjectManager {
namespace Internal {

AndroidDeployableListModel::AndroidDeployableListModel(const Qt4ProFileNode *proFileNode,
    ProFileUpdateSetting updateSetting, QObject *parent)
    : QAbstractTableModel(parent),
      m_projectType(proFileNode->projectType()),
      m_proFilePath(proFileNode->path()),
      m_projectName(proFileNode->displayName()),
      m_targetInfo(proFileNode->targetInformation()),
      m_installsList(proFileNode->installsList()),
      m_config(proFileNode->variableValue(ConfigVar)),
      m_modified(false),
      m_proFileUpdateSetting(updateSetting),
      m_hasTargetPath(false)
{
    buildModel();
}

AndroidDeployableListModel::~AndroidDeployableListModel() {}

bool AndroidDeployableListModel::buildModel()
{
    m_deployables.clear();

    m_hasTargetPath = !m_installsList.targetPath.isEmpty();
    if (!m_hasTargetPath && m_proFileUpdateSetting == UpdateProFile) {
        const QString remoteDirSuffix
            = QLatin1String(m_projectType == LibraryTemplate
                ? "/lib" : "/bin");
        const QString remoteDirAndroid5
            = QLatin1String("/opt/usr") + remoteDirSuffix;
        const QString remoteDirAndroid6
            = QLatin1String("/usr/local") + remoteDirSuffix;
        m_deployables.prepend(AndroidDeployable(localExecutableFilePath(),
            remoteDirAndroid5));
        QFile projectFile(m_proFilePath);
        if (!projectFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            qWarning("Error updating .pro file.");
            return false;
        }
        QString proFileTemplate = QLatin1String("\nunix:!symbian {\n"
            "    maemo5 {\n        target.path = maemo5path\n    } else {\n"
            "        target.path = maemo6path\n    }\n"
            "    INSTALLS += target\n}");
        proFileTemplate.replace(QLatin1String("maemo5path"), remoteDirAndroid5);
        proFileTemplate.replace(QLatin1String("maemo6path"), remoteDirAndroid6);
        if (!projectFile.write(proFileTemplate.toLocal8Bit())) {
            qWarning("Error updating .pro file.");
            return false;
        }
    } else {
        m_deployables.prepend(AndroidDeployable(localExecutableFilePath(),
            m_installsList.targetPath));
    }
    foreach (const InstallsItem &elem, m_installsList.items) {
        foreach (const QString &file, elem.files)
            m_deployables << AndroidDeployable(file, elem.path);
    }

    m_modified = true;
    return true;
}

AndroidDeployable AndroidDeployableListModel::deployableAt(int row) const
{
    Q_ASSERT(row >= 0 && row < rowCount());
    return m_deployables.at(row);
}

int AndroidDeployableListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_deployables.count();
}

int AndroidDeployableListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant AndroidDeployableListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();

    if (isEditable(index)) {
        if (role == Qt::DisplayRole)
            return tr("<no target path set>");
        if (role == Qt::ForegroundRole) {
            QBrush brush;
            brush.setColor("red");
            return brush;
        }
    }

    const AndroidDeployable &d = deployableAt(index.row());
    if (index.column() == 0 && role == Qt::DisplayRole)
        return QDir::toNativeSeparators(d.localFilePath);
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return d.remoteDir;
    return QVariant();
}

Qt::ItemFlags AndroidDeployableListModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags parentFlags = QAbstractTableModel::flags(index);
    if (isEditable(index))
        return parentFlags | Qt::ItemIsEditable;
    return parentFlags;
}

bool AndroidDeployableListModel::setData(const QModelIndex &index,
                                   const QVariant &value, int role)
{
    if (!isEditable(index) || role != Qt::EditRole)
        return false;
    const QString &remoteDir = value.toString();
    if (!addLinesToProFile(QStringList()
             << QString::fromLocal8Bit("target.path = %1").arg(remoteDir)
             << QLatin1String("INSTALLS += target")))
        return false;
    m_deployables.first().remoteDir = remoteDir;
    emit dataChanged(index, index);
    return true;
}

QVariant AndroidDeployableListModel::headerData(int section,
             Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    return section == 0 ? tr("Local File Path") : tr("Remote Directory");
}

QString AndroidDeployableListModel::localExecutableFilePath() const
{
    if (!m_targetInfo.valid)
        return QString();

    const bool isLib = m_projectType == LibraryTemplate;
    bool isStatic = false; // Nonsense init for stupid compilers.
    QString fileName;
    if (isLib) {
        fileName += QLatin1String("lib");
        isStatic = m_config.contains(QLatin1String("static"))
            || m_config.contains(QLatin1String("staticlib"));
    }
    fileName += m_targetInfo.target;
    if (isLib)
        fileName += QLatin1String(isStatic ? ".a" : ".so");
    return QDir::cleanPath(m_targetInfo.workingDir + '/' + fileName);
}

QString AndroidDeployableListModel::remoteExecutableFilePath() const
{
    return m_hasTargetPath
        ? deployableAt(0).remoteDir + '/'
              + QFileInfo(localExecutableFilePath()).fileName()
        : QString();
}

QString AndroidDeployableListModel::projectDir() const
{
    return QFileInfo(m_proFilePath).dir().path();
}

void AndroidDeployableListModel::setProFileUpdateSetting(ProFileUpdateSetting updateSetting)
{
    m_proFileUpdateSetting = updateSetting;
    if (updateSetting == UpdateProFile)
        buildModel();
}

bool AndroidDeployableListModel::isEditable(const QModelIndex &index) const
{
    return index.row() == 0 && index.column() == 1
        && m_deployables.first().remoteDir.isEmpty();
}

bool AndroidDeployableListModel::canAddDesktopFile() const
{
    if (m_projectType == LibraryTemplate)
        return false;
    foreach (const AndroidDeployable &d, m_deployables) {
        if (QFileInfo(d.localFilePath).fileName() == m_projectName + QLatin1String(".desktop"))
            return false;
    }
    return true;
}

bool AndroidDeployableListModel::addDesktopFile(QString &error)
{
    if (!canAddDesktopFile())
        return true;
    const QString desktopFilePath = QFileInfo(m_proFilePath).path()
        + QLatin1Char('/') + m_projectName + QLatin1String(".desktop");
    QFile desktopFile(desktopFilePath);
    const bool existsAlready = desktopFile.exists();
    if (!desktopFile.open(QIODevice::ReadWrite)) {
        error = tr("Failed to open '%1': %2")
            .arg(desktopFilePath, desktopFile.errorString());
        return false;
    }

    const QByteArray desktopTemplate("[Desktop Entry]\nEncoding=UTF-8\n"
        "Version=1.0\nType=Application\nTerminal=false\nName=%1\nExec=%2\n"
        "Icon=%1\nX-Window-Icon=\nX-HildonDesk-ShowInToolbar=true\n"
        "X-Osso-Type=application/x-executable\n");
    const QString contents = existsAlready
        ? QString::fromUtf8(desktopFile.readAll())
        : QString::fromLocal8Bit(desktopTemplate)
              .arg(m_projectName, remoteExecutableFilePath());
    desktopFile.resize(0);
    const QByteArray &contentsAsByteArray = contents.toUtf8();
    if (desktopFile.write(contentsAsByteArray) != contentsAsByteArray.count()
            || !desktopFile.flush()) {
        error = tr("Could not write '%1': %2")
            .arg(desktopFilePath, desktopFile.errorString());
            return false;
    }

    const AndroidToolChain *const tc = androidToolchain();
    QTC_ASSERT(tc, return false);
    QString remoteDir = QLatin1String("/usr/share/applications");
    if (tc->version() == AndroidToolChain::android_8)
        remoteDir += QLatin1String("/hildon");
    const QLatin1String filesLine("desktopfile.files = $${TARGET}.desktop");
    const QString pathLine = QLatin1String("desktopfile.path = ") + remoteDir;
    const QLatin1String installsLine("INSTALLS += desktopfile");
    if (!addLinesToProFile(QStringList() << filesLine << pathLine
            << installsLine)) {
        error = tr("Error writing project file.");
        return false;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_deployables << AndroidDeployable(desktopFilePath, remoteDir);
    endInsertRows();
    return true;
}

bool AndroidDeployableListModel::addLinesToProFile(const QStringList &lines)
{
    QFile projectFile(m_proFilePath);
    if (!projectFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning("Error opening .pro file for writing.");
        return false;
    }
    QString proFileScope;
    const AndroidToolChain *const tc = androidToolchain();
    QTC_ASSERT(tc, return false);
    if (tc->version() == AndroidToolChain::android_8)
        proFileScope = QLatin1String("android5");
    else
        proFileScope = QLatin1String("unix:!symbian:!maemo5");
    const QLatin1String separator("\n    ");
    const QString proFileString = QString(QLatin1Char('\n') + proFileScope
        + QLatin1String(" {") + separator + lines.join(separator)
        + QLatin1String("\n}\n"));
    const QByteArray &proFileByteArray = proFileString.toLocal8Bit();
    if (projectFile.write(proFileByteArray) != proFileByteArray.count()
            || !projectFile.flush()) {
        qWarning("Error updating .pro file.");
        return false;
    }
    return true;
}

const AndroidToolChain *AndroidDeployableListModel::androidToolchain() const
{
    const ProjectExplorer::Project *const activeProject
        = ProjectExplorer::ProjectExplorerPlugin::instance()->session()->startupProject();
    QTC_ASSERT(activeProject, return false);
    const Qt4Target *const activeTarget
        = qobject_cast<Qt4Target *>(activeProject->activeTarget());
    QTC_ASSERT(activeTarget, return false);
    const Qt4BuildConfiguration *const bc
        = activeTarget->activeBuildConfiguration();
    QTC_ASSERT(bc, return false);
    const AndroidToolChain *const tc
        = dynamic_cast<AndroidToolChain *>(bc->toolChain());
    QTC_ASSERT(tc, return false);
    return tc;
}

} // namespace Qt4ProjectManager
} // namespace Internal
