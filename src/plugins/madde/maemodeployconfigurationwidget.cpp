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
#include "maemodeployconfigurationwidget.h"
#include "ui_maemodeployconfigurationwidget.h"

#include "maemoglobal.h"
#include "qt4maemodeployconfiguration.h"
#include "qt4maemotarget.h"

#include <qt4projectmanager/qt4nodes.h>
#include <remotelinux/deployablefile.h>
#include <remotelinux/deployablefilesperprofile.h>
#include <remotelinux/deploymentinfo.h>
#include <remotelinux/deploymentsettingsassistant.h>
#include <remotelinux/remotelinuxdeployconfigurationwidget.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QPixmap>
#include <QtGui/QVBoxLayout>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {

MaemoDeployConfigurationWidget::MaemoDeployConfigurationWidget(QWidget *parent)
    : DeployConfigurationWidget(parent),
      ui(new Ui::MaemoDeployConfigurationWidget),
      m_remoteLinuxWidget(new RemoteLinuxDeployConfigurationWidget)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_remoteLinuxWidget);
    QWidget * const subWidget = new QWidget;
    ui->setupUi(subWidget);
    mainLayout->addWidget(subWidget);
    mainLayout->addStretch(1);

    connect(m_remoteLinuxWidget,
        SIGNAL(currentModelChanged(const RemoteLinux::DeployableFilesPerProFile *)),
        SLOT(handleCurrentModelChanged(const RemoteLinux::DeployableFilesPerProFile *)));
    handleCurrentModelChanged(m_remoteLinuxWidget->currentModel());
}

MaemoDeployConfigurationWidget::~MaemoDeployConfigurationWidget()
{
    delete ui;
}

void MaemoDeployConfigurationWidget::init(DeployConfiguration *dc)
{
    m_remoteLinuxWidget->init(dc);
    connect(ui->addDesktopFileButton, SIGNAL(clicked()), SLOT(addDesktopFile()));
    connect(ui->addIconButton, SIGNAL(clicked()), SLOT(addIcon()));
    connect(deployConfiguration()->deploymentInfo().data(), SIGNAL(modelAboutToBeReset()),
        SLOT(handleDeploymentInfoToBeReset()));
}

Qt4MaemoDeployConfiguration *MaemoDeployConfigurationWidget::deployConfiguration() const
{
    return qobject_cast<Qt4MaemoDeployConfiguration *>(m_remoteLinuxWidget->deployConfiguration());
}

void MaemoDeployConfigurationWidget::handleDeploymentInfoToBeReset()
{
    ui->addDesktopFileButton->setEnabled(false);
    ui->addIconButton->setEnabled(false);
}

void MaemoDeployConfigurationWidget::handleCurrentModelChanged(const DeployableFilesPerProFile *proFileInfo)
{
    ui->addDesktopFileButton->setEnabled(canAddDesktopFile(proFileInfo));
    ui->addIconButton->setEnabled(canAddIcon(proFileInfo));
}

void MaemoDeployConfigurationWidget::addDesktopFile()
{
    DeployableFilesPerProFile * const proFileInfo = m_remoteLinuxWidget->currentModel();
    QTC_ASSERT(canAddDesktopFile(proFileInfo), return);

    const QString desktopFilePath = QFileInfo(proFileInfo->proFilePath()).path()
        + QLatin1Char('/') + proFileInfo->projectName() + QLatin1String(".desktop");
    if (!QFile::exists(desktopFilePath)) {
        const QString desktopTemplate = QLatin1String("[Desktop Entry]\nEncoding=UTF-8\n"
            "Version=1.0\nType=Application\nTerminal=false\nName=%1\nExec=%2\n"
            "Icon=%1\nX-Window-Icon=\nX-HildonDesk-ShowInToolbar=true\n"
            "X-Osso-Type=application/x-executable\n");
        Utils::FileSaver saver(desktopFilePath);
        saver.write(desktopTemplate.arg(proFileInfo->projectName(),
            proFileInfo->remoteExecutableFilePath()).toUtf8());
        if (!saver.finalize(this))
            return;
    }

    DeployableFile d;
    d.remoteDir = QLatin1String("/usr/share/applications");
    if (qobject_cast<Qt4Maemo5Target *>(deployConfiguration()->target()))
        d.remoteDir += QLatin1String("/hildon");
    d.localFilePath = desktopFilePath;
    if (!deployConfiguration()->deploymentSettingsAssistant()->addDeployableToProFile(proFileInfo,
        QLatin1String("desktopfile"), d)) {
        QMessageBox::critical(this, tr("Project File Update Failed"),
            tr("Could not update the project file."));
    } else {
        ui->addDesktopFileButton->setEnabled(false);
    }
}

void MaemoDeployConfigurationWidget::addIcon()
{
    DeployableFilesPerProFile * const proFileInfo = m_remoteLinuxWidget->currentModel();
    const int iconDim = MaemoGlobal::applicationIconSize(deployConfiguration()->supportedOsType());
    const QString origFilePath = QFileDialog::getOpenFileName(this,
        tr("Choose Icon (will be scaled to %1x%1 pixels, if necessary)").arg(iconDim),
        proFileInfo->projectDir(), QLatin1String("(*.png)"));
    if (origFilePath.isEmpty())
        return;
    QPixmap pixmap(origFilePath);
    if (pixmap.isNull()) {
        QMessageBox::critical(this, tr("Invalid Icon"),
            tr("Unable to read image"));
        return;
    }
    const QSize iconSize(iconDim, iconDim);
    if (pixmap.size() != iconSize)
        pixmap = pixmap.scaled(iconSize);
    const QString newFileName = proFileInfo->projectName() + QLatin1Char('.')
        + QFileInfo(origFilePath).suffix();
    const QString newFilePath = proFileInfo->projectDir() + QLatin1Char('/') + newFileName;
    if (!pixmap.save(newFilePath)) {
        QMessageBox::critical(this, tr("Failed to Save Icon"),
            tr("Could not save icon to '%1'.").arg(newFilePath));
        return;
    }

    if (!deployConfiguration()->deploymentSettingsAssistant()->addDeployableToProFile(proFileInfo,
        QLatin1String("icon"), DeployableFile(newFilePath, remoteIconDir()))) {
        QMessageBox::critical(this, tr("Project File Update Failed"),
            tr("Could not update the project file."));
    } else {
        ui->addIconButton->setEnabled(false);
    }
}

bool MaemoDeployConfigurationWidget::canAddDesktopFile(const DeployableFilesPerProFile *proFileInfo) const
{
    return proFileInfo && proFileInfo->isApplicationProject()
        && deployConfiguration()->localDesktopFilePath(proFileInfo).isEmpty();
}

bool MaemoDeployConfigurationWidget::canAddIcon(const DeployableFilesPerProFile *proFileInfo) const
{
    return proFileInfo && proFileInfo->isApplicationProject()
        && remoteIconFilePath(proFileInfo).isEmpty();
}

QString MaemoDeployConfigurationWidget::remoteIconFilePath(const DeployableFilesPerProFile *proFileInfo) const
{
    QTC_ASSERT(proFileInfo->projectType() == ApplicationTemplate, return  QString());

    const QStringList imageTypes = QStringList() << QLatin1String("jpg") << QLatin1String("png")
        << QLatin1String("svg");
    for (int i = 0; i < proFileInfo->rowCount(); ++i) {
        const DeployableFile &d = proFileInfo->deployableAt(i);
        const QString extension = QFileInfo(d.localFilePath).suffix();
        if (d.remoteDir.startsWith(remoteIconDir()) && imageTypes.contains(extension))
            return d.remoteDir + QLatin1Char('/') + QFileInfo(d.localFilePath).fileName();
    }
    return QString();
}

QString MaemoDeployConfigurationWidget::remoteIconDir() const
{
    return QString::fromLocal8Bit("/usr/share/icons/hicolor/%1x%1/apps")
        .arg(MaemoGlobal::applicationIconSize(deployConfiguration()->supportedOsType()));
}

} // namespace Internal
} // namespace Madde
