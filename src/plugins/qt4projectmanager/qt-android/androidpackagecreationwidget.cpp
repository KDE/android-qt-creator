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

#include "androidpackagecreationwidget.h"
#include "ui_androidpackagecreationwidget.h"

#include "androidpackagecreationstep.h"
#include "androidtemplatesmanager.h"
#include "androidtoolchain.h"
#include "androidconfigurations.h"

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <utils/qtcassert.h>

#include <QtCore/QTimer>
#include <QtGui/QFileDialog>
#include <QtGui/QImageReader>
#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

AndroidPackageCreationWidget::AndroidPackageCreationWidget(AndroidPackageCreationStep *step)
    : ProjectExplorer::BuildStepConfigWidget(),
      m_step(step),
      m_ui(new Ui::AndroidPackageCreationWidget)
{
    m_ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QTimer::singleShot(0, this, SLOT(initGui()));
}

void AndroidPackageCreationWidget::init()
{
}

void AndroidPackageCreationWidget::initGui()
{
    const ProjectExplorer::Project * const project
        = m_step->buildConfiguration()->target()->project();
    updateAndroidProjectInfo(project);
    connect(m_step, SIGNAL(packageFilePathChanged()), this,
        SIGNAL(updateSummary()));
    connect(m_step, SIGNAL(qtVersionChanged()), this,
        SLOT(handleToolchainChanged()));
    connect(AndroidTemplatesManager::instance(),
        SIGNAL(androidDirContentsChanged(const ProjectExplorer::Project*)),
        this, SLOT(updateAndroidProjectInfo(const ProjectExplorer::Project*)));
    connect(m_ui->packageNameLineEdit, SIGNAL(editingFinished()), SLOT(setPackageName()));
    connect(m_ui->appNameLineEdit, SIGNAL(editingFinished()), SLOT(setApplicationName()));
    connect(m_ui->versionCode, SIGNAL(editingFinished()), SLOT(setVersionCode()));
    connect(m_ui->versionNameLinedit, SIGNAL(editingFinished()), SLOT(setVersionName()));
    connect(m_ui->targetSDKComboBox, SIGNAL(activated(QString)), SLOT(setTargetSDK(QString)));

}

void AndroidPackageCreationWidget::updateAndroidProjectInfo(const ProjectExplorer::Project *project)
{
    const ProjectExplorer::Project * const ourProject
        = m_step->buildConfiguration()->target()->project();

    if (project != ourProject)
        return;
    m_ui->targetSDKComboBox->clear();
    m_ui->targetSDKComboBox->addItems(AndroidConfigurations::instance().sdkTargets());
    m_ui->targetSDKComboBox->setCurrentIndex(AndroidConfigurations::instance().sdkTargets().indexOf(AndroidTemplatesManager::instance()->targetSDK(project)));
    m_ui->packageNameLineEdit->setText(AndroidTemplatesManager::instance()->packageName(project));
    m_ui->appNameLineEdit->setText(AndroidTemplatesManager::instance()->applicationName(project));
    m_ui->versionCode->setValue(AndroidTemplatesManager::instance()->versionCode(project));
    m_ui->versionNameLinedit->setText(AndroidTemplatesManager::instance()->versionName(project));
//    QString error;
//    const QIcon &icon
//        = AndroidTemplatesManager::instance()->packageManagerIcon(project, &error);
//    if (!error.isEmpty()) {
//        QMessageBox::critical(this, tr("Could not read icon"), error);
//    } else {
//        m_ui->packageManagerIconButton->setIcon(icon);
//        m_ui->packageManagerIconButton->setIconSize(m_ui->packageManagerIconButton->size());
//    }
}

void AndroidPackageCreationWidget::setPackageName()
{
    AndroidTemplatesManager::instance()->setPackageName(m_step->buildConfiguration()->target()->project(),
                                                        m_ui->packageNameLineEdit->text());
}

void AndroidPackageCreationWidget::setApplicationName()
{
    AndroidTemplatesManager::instance()->setApplicationName(m_step->buildConfiguration()->target()->project(),
                                                        m_ui->appNameLineEdit->text());
}

void AndroidPackageCreationWidget::setTargetSDK(const QString & target)
{
    AndroidTemplatesManager::instance()->setTargetSDK(m_step->buildConfiguration()->target()->project(),
                                                        target);
}

void AndroidPackageCreationWidget::setVersionCode()
{
    AndroidTemplatesManager::instance()->setVersionCode(m_step->buildConfiguration()->target()->project(),
                                                        m_ui->versionCode->value());
}

void AndroidPackageCreationWidget::setVersionName()
{
    AndroidTemplatesManager::instance()->setVersionName(m_step->buildConfiguration()->target()->project(),
                                                        m_ui->versionNameLinedit->text());
}


void AndroidPackageCreationWidget::setPackageManagerIcon()
{
    QString imageFilter = tr("Images") + QLatin1String("( ");
    const QList<QByteArray> &imageTypes = QImageReader::supportedImageFormats();
    foreach (const QByteArray &imageType, imageTypes)
        imageFilter += "*." + QString::fromAscii(imageType) + QLatin1Char(' ');
    imageFilter += QLatin1Char(')');
    const QString iconFileName = QFileDialog::getOpenFileName(this,
        tr("Choose Image (will be scaled to 48x48 pixels if necessary)"),
        QString(), imageFilter);
    if (!iconFileName.isEmpty()) {
        QString error;
        if (!AndroidTemplatesManager::instance()->setPackageManagerIcon(m_step->
            buildConfiguration()->target()->project(), iconFileName))
            QMessageBox::critical(this, tr("Could Not Set New Icon"), error);
    }
}

void AndroidPackageCreationWidget::handleToolchainChanged()
{
    if (!m_step->androidToolChain())
        return;
#warning FIXME Android
//    m_ui->skipCheckBox
//        ->setVisible(m_step->androidToolChain()->allowsPackagingDisabling());
//    m_ui->skipCheckBox->setChecked(!m_step->isPackagingEnabled());
    emit updateSummary();
}

QString AndroidPackageCreationWidget::summaryText() const
{
    const QString constantString = tr("<b>Create Package:</b> ");
    const QString dynamicString = m_step->isPackagingEnabled()
        ? QDir::toNativeSeparators(m_step->packageFilePath())
        : tr("(Packaging disabled)");
    return constantString + dynamicString;
}

QString AndroidPackageCreationWidget::displayName() const
{
    return m_step->displayName();
}

//void AndroidPackageCreationWidget::handleSkipButtonToggled(bool checked)
//{
//    m_ui->major->setEnabled(!checked);
//    m_ui->minor->setEnabled(!checked);
//    m_ui->debianFilesComboBox->setEnabled(!checked);
//    m_ui->editDebianFileButton->setEnabled(!checked);
//    m_step->setPackagingEnabled(!checked);
//    emit updateSummary();
//}



//void AndroidPackageCreationWidget::editDebianFile()
//{
////    const QString debianFilePath = AndroidTemplatesManager::instance()
////        ->debianDirPath(m_step->buildConfiguration()->target()->project())
////        + QLatin1Char('/') + m_ui->debianFilesComboBox->currentText();
////    Core::EditorManager::instance()->openEditor(debianFilePath,
////                                                QString(),
////                                                Core::EditorManager::ModeSwitch);
//}

} // namespace Internal
} // namespace Qt4ProjectManager
