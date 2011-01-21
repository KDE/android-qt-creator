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
#include "androidpackagecreationstep.h"
#include "androidtoolchain.h"
#include "androidconfigurations.h"
#include "qt4androidtarget.h"
#include "ui_androidpackagecreationwidget.h"

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


CheckModel::CheckModel(QObject * parent ):QAbstractListModel ( parent )
{

}

void CheckModel::setAvailableItems(const QStringList & items)
{
    m_availableItems = items;
    reset();
}

void CheckModel::setCheckedItems(const QStringList & items)
{
    m_checkedItems=items;
    reset();
}

QStringList CheckModel::checkedItems()
{
    return m_checkedItems;
}

QVariant CheckModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    switch(role)
    {
    case Qt::CheckStateRole:
        return m_checkedItems.contains(m_availableItems.at(index.row()))?Qt::Checked:Qt::Unchecked;
    case Qt::DisplayRole:
        return m_availableItems.at(index.row());
    }
    return QVariant();
}

bool CheckModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole || !index.isValid())
        return false;
    if (value.toInt() == Qt::Checked)
        m_checkedItems.append(m_availableItems.at(index.row()));
    else
        m_checkedItems.removeAll(m_availableItems.at(index.row()));
    emit dataChanged(index, index);
    return true;
}

int CheckModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_availableItems.count();
}

Qt::ItemFlags CheckModel::flags(const QModelIndex &/*index*/) const
{
    return Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled;
}

AndroidPackageCreationWidget::AndroidPackageCreationWidget(AndroidPackageCreationStep *step)
    : ProjectExplorer::BuildStepConfigWidget(),
      m_step(step),
      m_ui(new Ui::AndroidPackageCreationWidget)
{
    m_qtLibsModel = new CheckModel(this);
    m_prebundledLibs = new CheckModel(this);
    m_ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QTimer::singleShot(0, this, SLOT(initGui()));
}

void AndroidPackageCreationWidget::init()
{
}

void AndroidPackageCreationWidget::initGui()
{
    updateAndroidProjectInfo();
    Qt4AndroidTarget * target = m_step->androidTarget();
    connect(target,
        SIGNAL(androidDirContentsChanged(ProjectExplorer::Project*)),
        this, SLOT(updateAndroidProjectInfo(ProjectExplorer::Project*)));

    connect(m_ui->packageNameLineEdit, SIGNAL(editingFinished()), SLOT(setPackageName()));
    connect(m_ui->appNameLineEdit, SIGNAL(editingFinished()), SLOT(setApplicationName()));
    connect(m_ui->versionCode, SIGNAL(editingFinished()), SLOT(setVersionCode()));
    connect(m_ui->versionNameLinedit, SIGNAL(editingFinished()), SLOT(setVersionName()));
    connect(m_ui->targetSDKComboBox, SIGNAL(activated(QString)), SLOT(setTargetSDK(QString)));
    m_qtLibsModel->setAvailableItems(target->availableQtLibs());
    m_prebundledLibs->setAvailableItems(target->availablePrebundledLibs());
    m_ui->qtLibsListView->setModel(m_qtLibsModel);
    m_ui->prebundledLibsListView->setModel(m_prebundledLibs);
}

void AndroidPackageCreationWidget::updateAndroidProjectInfo()
{
    Qt4AndroidTarget * target = m_step->androidTarget();
    m_ui->targetSDKComboBox->clear();
    QStringList targets=AndroidConfigurations::instance().sdkTargets();
    m_ui->targetSDKComboBox->addItems(targets);
    m_ui->targetSDKComboBox->setCurrentIndex(targets.indexOf(target->targetSDK()));
    m_ui->packageNameLineEdit->setText(target->packageName());
    m_ui->appNameLineEdit->setText(target->applicationName());
    if (!m_ui->appNameLineEdit->text().length())
    {
        m_ui->appNameLineEdit->setText(target->project()->displayName());
        target->setApplicationName(target->project()->displayName());
    }
    m_ui->versionCode->setValue(target->versionCode());
    m_ui->versionNameLinedit->setText(target->versionName());
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
    m_step->androidTarget()->setPackageName(m_ui->packageNameLineEdit->text());
}

void AndroidPackageCreationWidget::setApplicationName()
{
    m_step->androidTarget()->setApplicationName(m_ui->appNameLineEdit->text());
}

void AndroidPackageCreationWidget::setTargetSDK(const QString & target)
{
    m_step->androidTarget()->setTargetSDK(target);
}

void AndroidPackageCreationWidget::setVersionCode()
{
    m_step->androidTarget()->setVersionCode(m_ui->versionCode->value());
}

void AndroidPackageCreationWidget::setVersionName()
{
    m_step->androidTarget()->setVersionName(m_ui->versionNameLinedit->text());
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
        if (!m_step->androidTarget()->setPackageManagerIcon(iconFileName))
            QMessageBox::critical(this, tr("Could Not Set New Icon"), error);
    }
}

void AndroidPackageCreationWidget::handleToolchainChanged()
{
//    if (!m_step->androidToolChain())
//        return;
#warning FIXME Android
//    m_ui->skipCheckBox
//        ->setVisible(m_step->androidToolChain()->allowsPackagingDisabling());
//    m_ui->skipCheckBox->setChecked(!m_step->isPackagingEnabled());
    emit updateSummary();
}

QString AndroidPackageCreationWidget::summaryText() const
{
    return tr("<b>Package configurations</b>");
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
