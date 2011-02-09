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
#include <QtCore/QProcess>

#include <QtGui/QFileDialog>
#include <QtGui/QImageReader>
#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

const QLatin1String emptyPerrmission("< type or choose a permission >");

///////////////////////////// CheckModel /////////////////////////////
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

const QStringList & CheckModel::checkedItems()
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

void CheckModel::swap(int index1, int index2)
{
    // HACK prevent qt (4.7.1) crash
    if (index1<index2)
        qSwap(index1, index2);
    // HACK

    beginMoveRows(QModelIndex(), index1, index1, QModelIndex(), index2);
    const QString & item1 = m_availableItems[index1];
    const QString & item2 = m_availableItems[index2];
    m_availableItems.swap(index1, index2);
    index1 = m_checkedItems.indexOf(item1);
    index2 = m_checkedItems.indexOf(item2);
    if (index1>-1 && index2>-1)
        m_checkedItems.swap(index1, index2);
    endMoveRows();
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
///////////////////////////// CheckModel /////////////////////////////


///////////////////////////// PermissionsModel /////////////////////////////
PermissionsModel::PermissionsModel( QObject * parent ):QAbstractListModel(parent)
{
}

void PermissionsModel::setPermissions(const QStringList & permissions)
{
    m_permissions = permissions;
    reset();
}
const QStringList & PermissionsModel::permissions()
{
    return m_permissions;
}

QModelIndex PermissionsModel::addPermission(const QString & permission)
{
    const int idx= m_permissions.count();
    beginInsertRows(QModelIndex(), idx, idx+1);
    m_permissions.push_back(permission);
    endInsertRows();
    return index(idx);
}

bool PermissionsModel::updatePermission(QModelIndex index, const QString & permission)
{
    if (!index.isValid())
        return false;
    if (m_permissions[index.row()] == permission)
        return false;
    m_permissions[index.row()]= permission;
    emit dataChanged(index, index);
    return true;
}

void PermissionsModel::removePermission(int index)
{
    if (index>=m_permissions.size())
        return;
    beginRemoveRows(QModelIndex(), index, index+1);
    m_permissions.removeAt(index);
    endRemoveRows();
}

QVariant PermissionsModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !index.isValid())
        return QVariant();
    return m_permissions[index.row()];
}

int PermissionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_permissions.count();
}
///////////////////////////// PermissionsModel /////////////////////////////


///////////////////////////// AndroidPackageCreationWidget /////////////////////////////
AndroidPackageCreationWidget::AndroidPackageCreationWidget(AndroidPackageCreationStep *step)
    : ProjectExplorer::BuildStepConfigWidget(),
      m_step(step),
      m_ui(new Ui::AndroidPackageCreationWidget)
{
    m_qtLibsModel = new CheckModel(this);
    m_prebundledLibs = new CheckModel(this);
    m_permissionsModel = new PermissionsModel(this);

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
        SIGNAL(androidDirContentsChanged()),
        this, SLOT(updateAndroidProjectInfo()));

    connect(m_ui->packageNameLineEdit, SIGNAL(editingFinished()), SLOT(setPackageName()));
    connect(m_ui->appNameLineEdit, SIGNAL(editingFinished()), SLOT(setApplicationName()));
    connect(m_ui->versionCode, SIGNAL(editingFinished()), SLOT(setVersionCode()));
    connect(m_ui->versionNameLinedit, SIGNAL(editingFinished()), SLOT(setVersionName()));
    connect(m_ui->targetSDKComboBox, SIGNAL(activated(QString)), SLOT(setTargetSDK(QString)));
    connect(m_ui->permissionsListView, SIGNAL(activated(QModelIndex)), SLOT(permissionActivated(QModelIndex)));
    connect(m_ui->addPermissionButton, SIGNAL(clicked()), SLOT(addPermission()));
    connect(m_ui->removePermissionButton, SIGNAL(clicked()), SLOT(removePermission()));
    connect(m_ui->permissionsComboBox->lineEdit(), SIGNAL(editingFinished()), SLOT(updatePermission()));
    connect(m_ui->savePermissionsButton, SIGNAL(clicked()), SLOT(savePermissionsButton()));
    connect(m_ui->discardPermissionsButton, SIGNAL(clicked()), SLOT(discardPermissionsButton()));
    connect(m_ui->targetComboBox, SIGNAL(activated(QString)), SLOT(setTarget(QString)));
    connect(m_qtLibsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(setQtLibs(QModelIndex,QModelIndex)));
    connect(m_prebundledLibs, SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(setPrebundledLibs(QModelIndex,QModelIndex)));
    connect(m_ui->prebundledLibsListView, SIGNAL(activated(QModelIndex)), SLOT(prebundledLibSelected(QModelIndex)));
    connect(m_ui->upPushButton, SIGNAL(clicked()), SLOT(prebundledLibMoveUp()));
    connect(m_ui->downPushButton, SIGNAL(clicked()), SLOT(prebundledLibMoveDown()));
    connect(m_ui->readInfoPushButton, SIGNAL(clicked()), SLOT(readElfInfo()));

    connect(m_ui->hIconButton, SIGNAL(clicked()), SLOT(setHDPIIcon()));
    connect(m_ui->mIconButton, SIGNAL(clicked()), SLOT(setMDPIIcon()));
    connect(m_ui->lIconButton, SIGNAL(clicked()), SLOT(setLDPIIcon()));

    m_ui->qtLibsListView->setModel(m_qtLibsModel);
    m_ui->prebundledLibsListView->setModel(m_prebundledLibs);
    m_ui->permissionsListView->setModel(m_permissionsModel);
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
        QString applicationName = target->project()->displayName();
        if (applicationName.length())
            applicationName[0]=applicationName[0].toUpper();
        m_ui->appNameLineEdit->setText(applicationName);
        target->setApplicationName(applicationName);
    }
    m_ui->versionCode->setValue(target->versionCode());
    m_ui->versionNameLinedit->setText(target->versionName());

    m_qtLibsModel->setAvailableItems(target->availableQtLibs());
    m_qtLibsModel->setCheckedItems(target->qtLibs());
    m_prebundledLibs->setAvailableItems(target->availablePrebundledLibs());
    m_prebundledLibs->setCheckedItems(target->prebundledLibs());

    m_permissionsModel->setPermissions(target->permissions());
    m_ui->removePermissionButton->setEnabled(m_permissionsModel->permissions().size());

    targets=target->availableTargetApplications();
    m_ui->targetComboBox->clear();
    m_ui->targetComboBox->addItems(targets);
    m_ui->targetComboBox->setCurrentIndex(targets.indexOf(target->targetApplication()));
    if (-1 == m_ui->targetComboBox->currentIndex() && targets.count())
    {
        m_ui->targetComboBox->setCurrentIndex(0);
        m_step->androidTarget()->setTargetApplication(m_ui->targetComboBox->currentText());
    }
    m_ui->hIconButton->setIcon(target->highDpiIcon());
    m_ui->mIconButton->setIcon(target->mediumDpiIcon());
    m_ui->lIconButton->setIcon(target->lowDpiIcon());
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

void AndroidPackageCreationWidget::setTarget(const QString & target)
{
    m_step->androidTarget()->setTargetApplication(target);
}

void AndroidPackageCreationWidget::setQtLibs(QModelIndex,QModelIndex)
{
    m_step->androidTarget()->setQtLibs(m_qtLibsModel->checkedItems());
}

void AndroidPackageCreationWidget::setPrebundledLibs(QModelIndex,QModelIndex)
{
    m_step->androidTarget()->setPrebundledLibs(m_prebundledLibs->checkedItems());
}

void AndroidPackageCreationWidget::prebundledLibSelected(const QModelIndex & index)
{
    m_ui->upPushButton->setEnabled(false);
    m_ui->downPushButton->setEnabled(false);
    if (!index.isValid())
        return;
    if (index.row()>0)
        m_ui->upPushButton->setEnabled(true);
    if (index.row()<m_prebundledLibs->rowCount(QModelIndex())-1)
        m_ui->downPushButton->setEnabled(true);
}

void AndroidPackageCreationWidget::prebundledLibMoveUp()
{
    const QModelIndex & index = m_ui->prebundledLibsListView->currentIndex();
    if (index.isValid())
        m_prebundledLibs->swap(index.row(), index.row()-1);
}

void AndroidPackageCreationWidget::prebundledLibMoveDown()
{
    const QModelIndex & index = m_ui->prebundledLibsListView->currentIndex();
    if (index.isValid())
        m_prebundledLibs->swap(index.row(), index.row()+1);
}

void AndroidPackageCreationWidget::setHDPIIcon()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Choose High DPI Icon"), QDir::homePath(), tr("png images (*.png)"));
    if (!file.length())
        return;
    m_step->androidTarget()->setHighDpiIcon(file);
    m_ui->hIconButton->setIcon(m_step->androidTarget()->highDpiIcon());
}

void AndroidPackageCreationWidget::setMDPIIcon()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Choose Medium DPI Icon"), QDir::homePath(), tr("png images (*.png)"));
    if (!file.length())
        return;
    m_step->androidTarget()->setMediumDpiIcon(file);
    m_ui->mIconButton->setIcon(m_step->androidTarget()->mediumDpiIcon());
}

void AndroidPackageCreationWidget::setLDPIIcon()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Choose Low DPI Icon"), QDir::homePath(), tr("png images (*.png)"));
    if (!file.length())
        return;
    m_step->androidTarget()->setLowDpiIcon(file);
    m_ui->lIconButton->setIcon(m_step->androidTarget()->lowDpiIcon());
}

void AndroidPackageCreationWidget::permissionActivated(QModelIndex index)
{
    m_ui->permissionsComboBox->setCurrentIndex(
                m_ui->permissionsComboBox->findText(m_permissionsModel->data(index, Qt::DisplayRole).toString()));
    m_ui->permissionsComboBox->lineEdit()->setText(m_permissionsModel->data(index, Qt::DisplayRole).toString());
}

void AndroidPackageCreationWidget::addPermission()
{
    setEnabledSaveDiscardButtons(true);
    m_ui->permissionsListView->setCurrentIndex(m_permissionsModel->addPermission(emptyPerrmission));
    m_ui->permissionsComboBox->lineEdit()->setText(emptyPerrmission);
    m_ui->permissionsComboBox->setFocus();
    m_ui->removePermissionButton->setEnabled(m_permissionsModel->permissions().size());
}

void AndroidPackageCreationWidget::updatePermission()
{
    if (m_permissionsModel->updatePermission(m_ui->permissionsListView->currentIndex()
                                         , m_ui->permissionsComboBox->lineEdit()->text()))
        setEnabledSaveDiscardButtons(true);
}

void AndroidPackageCreationWidget::removePermission()
{
    setEnabledSaveDiscardButtons(true);
    if (m_ui->permissionsListView->currentIndex().isValid())
        m_permissionsModel->removePermission(m_ui->permissionsListView->currentIndex().row());
    m_ui->removePermissionButton->setEnabled(m_permissionsModel->permissions().size());
}

void AndroidPackageCreationWidget::savePermissionsButton()
{
    setEnabledSaveDiscardButtons(false);
    m_step->androidTarget()->setPermissions(m_permissionsModel->permissions());
}

void AndroidPackageCreationWidget::discardPermissionsButton()
{
    setEnabledSaveDiscardButtons(false);
    m_permissionsModel->setPermissions(m_step->androidTarget()->permissions());
    m_ui->permissionsComboBox->setCurrentIndex(-1);
    m_ui->removePermissionButton->setEnabled(m_permissionsModel->permissions().size());
}

void AndroidPackageCreationWidget::readElfInfo()
{
    QProcess readelfProc;
    QString appPath=m_step->androidTarget()->targetApplicationPath();
    if (!QFile::exists(appPath))
    {
        QMessageBox::critical(this, tr("Can't find read elf information"),
                              tr("Can't find '%1'.\n"
                                 "Please make sure your appication "
                                 " built successfully and is selected in Appplication tab ('Run option') ").arg(appPath) );
        return;
    }
    readelfProc.start(QString("%1 -d -W %2")
                      .arg(AndroidConfigurations::instance().readelfPath())
                      .arg(appPath));
    if (!readelfProc.waitForFinished(-1))
        return;
    QStringList libs;
    QList<QByteArray> lines=readelfProc.readAll().trimmed().split('\n');
    foreach(QByteArray line, lines)
    {
        if (line.contains("(NEEDED)") && line.contains("Shared library:") )
        {
            const int pos=line.lastIndexOf('[')+1;
            libs<<line.mid(pos,line.length()-pos-1);
        }
    }
    libs.sort();
    QStringList checkedLibs = m_qtLibsModel->checkedItems();
    foreach(const QString & qtLib, m_step->androidTarget()->availableQtLibs())
    {
        if (libs.contains("lib"+qtLib+".so") && !checkedLibs.contains(qtLib))
            checkedLibs<<qtLib;
    }
    checkedLibs.sort();
    m_step->androidTarget()->setQtLibs(checkedLibs);
    m_qtLibsModel->setCheckedItems(checkedLibs);

    checkedLibs = m_prebundledLibs->checkedItems();
    foreach(const QString & qtLib, m_step->androidTarget()->availableQtLibs())
    {
        if (libs.contains(qtLib) && !checkedLibs.contains(qtLib))
            checkedLibs<<qtLib;
    }
    m_step->androidTarget()->setPrebundledLibs(checkedLibs);
    m_prebundledLibs->setCheckedItems(checkedLibs);
}

void AndroidPackageCreationWidget::setEnabledSaveDiscardButtons(bool enabled)
{
    if (!enabled)
        m_ui->permissionsListView->setFocus();
    m_ui->savePermissionsButton->setEnabled(enabled);
    m_ui->discardPermissionsButton->setEnabled(enabled);
}

QString AndroidPackageCreationWidget::summaryText() const
{
    return tr("<b>Package configurations</b>");
}

QString AndroidPackageCreationWidget::displayName() const
{
    return m_step->displayName();
}

} // namespace Internal
} // namespace Qt4ProjectManager
