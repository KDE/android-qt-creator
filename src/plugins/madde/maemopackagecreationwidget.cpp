/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "maemopackagecreationwidget.h"
#include "ui_maemopackagecreationwidget.h"

#include "debianmanager.h"
#include "maddedevice.h"
#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "rpmmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/profileinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <utils/qtcassert.h>

#include <QTimer>
#include <QFileDialog>
#include <QImageReader>
#include <QMessageBox>

using namespace ProjectExplorer;

namespace Madde {
namespace Internal {

// TODO: Split up into dedicated widgets for Debian and RPM steps.
MaemoPackageCreationWidget::MaemoPackageCreationWidget(AbstractMaemoPackageCreationStep *step)
    : ProjectExplorer::BuildStepConfigWidget(),
      m_step(step),
      m_ui(new Ui::MaemoPackageCreationWidget)
{
    m_ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QTimer::singleShot(0, this, SLOT(initGui()));
}

MaemoPackageCreationWidget::~MaemoPackageCreationWidget()
{
    delete m_ui;
}

void MaemoPackageCreationWidget::initGui()
{
    m_ui->shortDescriptionLineEdit->setMaxLength(60);
    updateVersionInfo();
    Core::Id deviceType = ProjectExplorer::DeviceTypeProfileInformation::deviceTypeId(m_step->target()->profile());
    if (MaddeDevice::isDebianBased(deviceType)) {
        const Utils::FileName path = DebianManager::debianDirectory(m_step->target());
        const QSize iconSize = MaddeDevice::packageManagerIconSize(deviceType);
        m_ui->packageManagerIconButton->setFixedSize(iconSize);
        m_ui->packageManagerIconButton->setToolTip(tr("Size should be %1x%2 pixels")
            .arg(iconSize.width()).arg(iconSize.height()));
        m_ui->editSpecFileButton->setVisible(false);
        updateDebianFileList(path);
        handleControlFileUpdate(path);
        DebianManager *dm = DebianManager::instance();
        connect(m_ui->packageManagerNameLineEdit, SIGNAL(editingFinished()),
            SLOT(setPackageManagerName()));
        connect(dm, SIGNAL(debianDirectoryChanged(Utils::FileName)),
            SLOT(updateDebianFileList(Utils::FileName)));
        connect(dm, SIGNAL(changelogChanged(Utils::FileName)),
            SLOT(updateVersionInfo()));
        connect(dm, SIGNAL(controlChanged(Utils::FileName)),
            SLOT(handleControlFileUpdate(Utils::FileName)));
    } else {
        const Utils::FileName path = RpmManager::specFile(m_step->target());
        m_ui->packageManagerNameLabel->hide();
        m_ui->packageManagerNameLineEdit->hide();
        m_ui->packageManagerIconLabel->hide();
        m_ui->packageManagerIconButton->hide();
        m_ui->editDebianFileLabel->hide();
        m_ui->debianFilesComboBox->hide();
        m_ui->editDebianFileButton->hide();

        // This is fragile; be careful when editing the UI file.
        m_ui->formLayout->removeItem(m_ui->formLayout->itemAt(4, QFormLayout::LabelRole));
        m_ui->formLayout->removeItem(m_ui->formLayout->itemAt(4, QFormLayout::FieldRole));
        m_ui->formLayout->removeItem(m_ui->formLayout->itemAt(5, QFormLayout::LabelRole));
        m_ui->formLayout->removeItem(m_ui->formLayout->itemAt(5, QFormLayout::FieldRole));
        m_ui->formLayout->removeItem(m_ui->formLayout->itemAt(6, QFormLayout::LabelRole));
        m_ui->formLayout->removeItem(m_ui->formLayout->itemAt(6, QFormLayout::FieldRole));
        handleSpecFileUpdate(path);
        connect(RpmManager::instance(), SIGNAL(specFileChanged(Utils::FileName)),
            SLOT(handleSpecFileUpdate(Utils::FileName)));
        connect(m_ui->editSpecFileButton, SIGNAL(clicked()),
            SLOT(editSpecFile()));
    }
    connect(m_step, SIGNAL(packageFilePathChanged()), this,
        SIGNAL(updateSummary()));
    connect(m_ui->packageNameLineEdit, SIGNAL(editingFinished()),
        SLOT(setPackageName()));
    connect(m_ui->shortDescriptionLineEdit, SIGNAL(editingFinished()),
        SLOT(setShortDescription()));
}

void MaemoPackageCreationWidget::updateDebianFileList(const Utils::FileName &debianDir)
{
    if (debianDir != DebianManager::debianDirectory(m_step->target()))
        return;

    m_ui->debianFilesComboBox->clear();
    const QStringList &debianFiles = DebianManager::debianFiles(debianDir);
    foreach (const QString &fileName, debianFiles) {
        if (fileName != QLatin1String("compat")
                && !fileName.endsWith(QLatin1Char('~')))
            m_ui->debianFilesComboBox->addItem(fileName);
    }
}

void MaemoPackageCreationWidget::updateVersionInfo()
{
    QString error;
    QString versionString = m_step->versionString(&error);
    if (versionString.isEmpty()) {
        QMessageBox::critical(this, tr("No Version Available."), error);
        versionString = AbstractMaemoPackageCreationStep::DefaultVersionNumber;
    }
    const QStringList list = versionString.split(QLatin1Char('.'),
        QString::SkipEmptyParts);
    const bool blocked = m_ui->major->signalsBlocked();
    m_ui->major->blockSignals(true);
    m_ui->minor->blockSignals(true);
    m_ui->patch->blockSignals(true);
    m_ui->major->setValue(list.value(0, QLatin1String("0")).toInt());
    m_ui->minor->setValue(list.value(1, QLatin1String("0")).toInt());
    m_ui->patch->setValue(list.value(2, QLatin1String("0")).toInt());
    m_ui->major->blockSignals(blocked);
    m_ui->minor->blockSignals(blocked);
    m_ui->patch->blockSignals(blocked);
    updateSummary();
}

void MaemoPackageCreationWidget::handleControlFileUpdate(const Utils::FileName &debianDir)
{
    if (debianDir != DebianManager::debianDirectory(m_step->target()))
        return;

    updatePackageName();
    updateShortDescription();
    updatePackageManagerName();
    updatePackageManagerIcon();
    updateSummary();
}

void MaemoPackageCreationWidget::handleSpecFileUpdate(const Utils::FileName &spec)
{
    if (spec != RpmManager::specFile(m_step->target()))
        return;

    updatePackageName();
    updateShortDescription();
    updateVersionInfo();
    updateSummary();
}

void MaemoPackageCreationWidget::updatePackageManagerIcon()
{
    const Utils::FileName path = DebianManager::debianDirectory(m_step->target());
    QString error;
    const QIcon &icon = DebianManager::packageManagerIcon(path, &error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, tr("Could not read icon"), error);
    } else {
        m_ui->packageManagerIconButton->setIcon(icon);
        m_ui->packageManagerIconButton->setIconSize(m_ui->packageManagerIconButton->size());
    }
}

void MaemoPackageCreationWidget::updatePackageName()
{
    const Utils::FileName path = DebianManager::debianDirectory(m_step->target());
    m_ui->packageNameLineEdit->setText(DebianManager::packageName(path));
}

void MaemoPackageCreationWidget::updatePackageManagerName()
{
    const Utils::FileName path = DebianManager::debianDirectory(m_step->target());
    Core::Id deviceType
            = ProjectExplorer::DeviceTypeProfileInformation::deviceTypeId(m_step->target()->profile());
    m_ui->packageManagerNameLineEdit->setText(DebianManager::packageManagerName(path, deviceType));
}

void MaemoPackageCreationWidget::updateShortDescription()
{
    const Utils::FileName path = DebianManager::debianDirectory(m_step->target());
    m_ui->shortDescriptionLineEdit->setText(DebianManager::shortDescription(path));
}

void MaemoPackageCreationWidget::setPackageManagerIcon()
{
    const Utils::FileName path = DebianManager::debianDirectory(m_step->target());
    Core::Id deviceType
            = ProjectExplorer::DeviceTypeProfileInformation::deviceTypeId(m_step->target()->profile());
    QString imageFilter = tr("Images") + QLatin1String("( ");
    const QList<QByteArray> &imageTypes = QImageReader::supportedImageFormats();
    foreach (const QByteArray &imageType, imageTypes)
        imageFilter += "*." + QString::fromAscii(imageType) + QLatin1Char(' ');
    imageFilter += QLatin1Char(')');
    const QSize iconSize = MaddeDevice::packageManagerIconSize(deviceType);
    const QString iconFileName = QFileDialog::getOpenFileName(this,
        tr("Choose Image (will be scaled to %1x%2 pixels if necessary)")
            .arg(iconSize.width()).arg(iconSize.height()), QString(), imageFilter);
    if (!iconFileName.isEmpty()) {
        QString error;
        if (!DebianManager::setPackageManagerIcon(path, deviceType,
                                                  Utils::FileName::fromString(iconFileName), &error))
            QMessageBox::critical(this, tr("Could Not Set New Icon"), error);
    }
}

void MaemoPackageCreationWidget::setPackageName()
{
    const Utils::FileName path = DebianManager::debianDirectory(m_step->target());
    if (!DebianManager::setPackageName(path, m_ui->packageNameLineEdit->text())) {
        QMessageBox::critical(this, tr("File Error"),
            tr("Could not set project name."));
    }
}

void MaemoPackageCreationWidget::setPackageManagerName()
{
    const Utils::FileName path = DebianManager::debianDirectory(m_step->target());
    Core::Id deviceType
            = ProjectExplorer::DeviceTypeProfileInformation::deviceTypeId(m_step->target()->profile());
    if (!DebianManager::setPackageManagerName(path, deviceType, m_ui->packageManagerNameLineEdit->text())) {
        QMessageBox::critical(this, tr("File Error"),
            tr("Could not set package name for project manager."));
    }
}

void MaemoPackageCreationWidget::setShortDescription()
{
    const Utils::FileName path = DebianManager::debianDirectory(m_step->target());
    if (!DebianManager::setShortDescription(path, m_ui->shortDescriptionLineEdit->text())) {
        QMessageBox::critical(this, tr("File Error"),
            tr("Could not set project description."));
    }
}

QString MaemoPackageCreationWidget::summaryText() const
{
    return tr("<b>Create Package:</b> ")
        + QDir::toNativeSeparators(m_step->packageFilePath());
}

QString MaemoPackageCreationWidget::displayName() const
{
    return m_step->displayName();
}

void MaemoPackageCreationWidget::versionInfoChanged()
{
    QString error;
    const bool success = m_step->setVersionString(m_ui->major->text()
        + QLatin1Char('.') + m_ui->minor->text() + QLatin1Char('.')
        + m_ui->patch->text(), &error);
    if (!success) {
        QMessageBox::critical(this, tr("Could Not Set Version Number"), error);
        updateVersionInfo();
    }
}

void MaemoPackageCreationWidget::editDebianFile()
{
    Utils::FileName path = DebianManager::debianDirectory(m_step->target());
    path.appendPath(m_ui->debianFilesComboBox->currentText());
    editFile(path.toString());
}

void MaemoPackageCreationWidget::editSpecFile()
{
    editFile(RpmManager::specFile(m_step->target()).toString());
}

void MaemoPackageCreationWidget::editFile(const QString &filePath)
{
    Core::EditorManager::openEditor(filePath, Core::Id(),
        Core::EditorManager::ModeSwitch);
}

} // namespace Internal
} // namespace Madde
