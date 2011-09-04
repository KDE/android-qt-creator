/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidpackagecreationwidget.h"
#include "androidpackagecreationstep.h"
#include "androidconfigurations.h"
#include "androidcreatekeystorecertificate.h"
#include "androidtarget.h"
#include "ui_androidpackagecreationwidget.h"

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qmakestep.h>
#include <qt4projectmanager/makestep.h>
#include <utils/qtcassert.h>

#include <QtCore/QTimer>
#include <QtCore/QProcess>

#include <QtGui/QFileDialog>
#include <QtGui/QImageReader>
#include <QtGui/QMessageBox>

namespace Android {
namespace Internal {

using namespace Qt4ProjectManager;

const QLatin1String emptyPerrmission("< type or choose a permission >");
const QLatin1String packageNameRegExp("^([a-z_]{1}[a-z0-9_]+(\\.[a-zA-Z_]{1}[a-zA-Z0-9_]*)*)$");

QString cleanPackageName(QString packageName)
{
    const QRegExp legalChars(QLatin1String("[a-zA-Z0-9_\\.]"));

    for (int i = 0; i < packageName.length(); ++i)
        if (!legalChars.exactMatch(packageName.mid(i, 1)))
            packageName[i] = QLatin1Char('_');

    return packageName;
}

bool checkPackageName(const QString & packageName)
{
    return QRegExp(packageNameRegExp).exactMatch(packageName);
}

///////////////////////////// CheckModel /////////////////////////////
CheckModel::CheckModel(QObject *parent):QAbstractListModel(parent)
{

}

void CheckModel::setAvailableItems(const QStringList &items)
{
    m_availableItems = items;
    reset();
}

void CheckModel::setCheckedItems(const QStringList &items)
{
    m_checkedItems=items;
    reset();
}

const QStringList &CheckModel::checkedItems()
{
    return m_checkedItems;
}

QVariant CheckModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    switch (role) {
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
    if (index1 < index2)
        qSwap(index1, index2);
    // HACK

    beginMoveRows(QModelIndex(), index1, index1, QModelIndex(), index2);
    const QString &item1 = m_availableItems[index1];
    const QString &item2 = m_availableItems[index2];
    m_availableItems.swap(index1, index2);
    index1 = m_checkedItems.indexOf(item1);
    index2 = m_checkedItems.indexOf(item2);
    if (index1 > -1 && index2 > -1)
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
PermissionsModel::PermissionsModel(QObject *parent):QAbstractListModel(parent)
{
}

void PermissionsModel::setPermissions(const QStringList &permissions)
{
    m_permissions = permissions;
    reset();
}
const QStringList &PermissionsModel::permissions()
{
    return m_permissions;
}

QModelIndex PermissionsModel::addPermission(const QString &permission)
{
    const int idx = m_permissions.count();
    beginInsertRows(QModelIndex(), idx, idx+1);
    m_permissions.push_back(permission);
    endInsertRows();
    return index(idx);
}

bool PermissionsModel::updatePermission(QModelIndex index, const QString &permission)
{
    if (!index.isValid())
        return false;
    if (m_permissions[index.row()] == permission)
        return false;
    m_permissions[index.row()] = permission;
    emit dataChanged(index, index);
    return true;
}

void PermissionsModel::removePermission(int index)
{
    if (index >= m_permissions.size())
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
    connect(m_step, SIGNAL(updateRequiredLibrariesModels()), SLOT(updateRequiredLibrariesModels()));

}

void AndroidPackageCreationWidget::init()
{
}

void AndroidPackageCreationWidget::initGui()
{
    updateAndroidProjectInfo();
    AndroidTarget * target = m_step->androidTarget();
    connect(target,
        SIGNAL(androidDirContentsChanged()),
        this, SLOT(updateAndroidProjectInfo()));
    m_ui->packageNameLineEdit->setValidator(new QRegExpValidator(QRegExp(packageNameRegExp), this));
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
    m_ui->KeystoreLocationLineEdit->setText(m_step->keystorePath());
}

void AndroidPackageCreationWidget::updateAndroidProjectInfo()
{
    AndroidTarget * target = m_step->androidTarget();
    m_ui->targetSDKComboBox->clear();
    QStringList targets=AndroidConfigurations::instance().sdkTargets();
    m_ui->targetSDKComboBox->addItems(targets);
    m_ui->targetSDKComboBox->setCurrentIndex(targets.indexOf(target->targetSDK()));
    m_ui->packageNameLineEdit->setText(target->packageName());
    m_ui->appNameLineEdit->setText(target->applicationName());
    if (!m_ui->appNameLineEdit->text().length()) {
        QString applicationName = target->project()->displayName();
        target->setPackageName(cleanPackageName(target->packageName()+"."+applicationName));
        m_ui->packageNameLineEdit->setText(target->packageName());
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
    if (m_ui->targetComboBox->currentIndex() == -1 && targets.count()) {
        m_ui->targetComboBox->setCurrentIndex(0);
        m_step->androidTarget()->setTargetApplication(m_ui->targetComboBox->currentText());
    }
    m_ui->hIconButton->setIcon(target->highDpiIcon());
    m_ui->mIconButton->setIcon(target->mediumDpiIcon());
    m_ui->lIconButton->setIcon(target->lowDpiIcon());
}

void AndroidPackageCreationWidget::setPackageName()
{
    const QString packageName= m_ui->packageNameLineEdit->text();
    if (!checkPackageName(packageName)) {
        QMessageBox::critical(this, tr("Invalid package name")
                              , tr("The package name '%1' is not valid.\nPlease choose a valid package name for your application (e.g. \"org.example.myapplication\").").arg(packageName));
        m_ui->packageNameLineEdit->selectAll();
        m_ui->packageNameLineEdit->setFocus();
        return;
    }
    m_step->androidTarget()->setPackageName(packageName);
}

void AndroidPackageCreationWidget::setApplicationName()
{
    m_step->androidTarget()->setApplicationName(m_ui->appNameLineEdit->text());
}

void AndroidPackageCreationWidget::setTargetSDK(const QString & target)
{
    m_step->androidTarget()->setTargetSDK(target);
    Qt4BuildConfiguration *bc = m_step->androidTarget()->activeBuildConfiguration();
    ProjectExplorer::BuildManager * bm = ProjectExplorer::ProjectExplorerPlugin::instance()->buildManager();
    QMakeStep *qs = bc->qmakeStep();

    if (!qs)
        return;

    qs->setForced(true);

    bm->buildList(bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
    bm->appendStep(qs);
    bc->setSubNodeBuild(0);
    bool use=bc->useSystemEnvironment();
    bc->setUseSystemEnvironment(!use);
    bc->setUseSystemEnvironment(use);
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

void AndroidPackageCreationWidget::updateRequiredLibrariesModels()
{
    m_qtLibsModel->setCheckedItems(m_step->androidTarget()->qtLibs());
    m_prebundledLibs->setCheckedItems(m_step->androidTarget()->prebundledLibs());
}

void AndroidPackageCreationWidget::readElfInfo()
{
    m_step->checkRequiredLibraries();
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

void AndroidPackageCreationWidget::setCertificates()
{
    QAbstractItemModel * certificates = m_step->keystoreCertificates();
    m_ui->signPackageCheckBox->setChecked(certificates);
    m_ui->certificatesAliasComboBox->setModel(certificates);
}

void AndroidPackageCreationWidget::on_signPackageCheckBox_toggled(bool checked)
{
    if (!checked)
        return;
    if (m_step->keystorePath().length())
        setCertificates();
}

void AndroidPackageCreationWidget::on_KeystoreCreatePushButton_clicked()
{
    AndroidCreateKeystoreCertificate d;
    if (d.exec()!=QDialog::Accepted)
        return;
    m_ui->KeystoreLocationLineEdit->setText(d.keystoreFilePath());
    m_step->setKeystorePath(d.keystoreFilePath());
    m_step->setKeystorePassword(d.keystorePassword());
    m_step->setCertificateAlias(d.certificateAlias());
    m_step->setCertificatePassword(d.certificatePassword());
    setCertificates();
}

void AndroidPackageCreationWidget::on_KeystoreLocationPushButton_clicked()
{
    QString keystorePath = m_step->keystorePath();
    if (!keystorePath.length())
        keystorePath=QDir::homePath();
    QString file = QFileDialog::getOpenFileName(this, tr("Select keystore file"), keystorePath, tr("Keystore files (*.keystore *.jks)"));
    if (!file.length())
        return;
    m_ui->KeystoreLocationLineEdit->setText(file);
    m_step->setKeystorePath(file);
    m_ui->signPackageCheckBox->setChecked(false);
}

void AndroidPackageCreationWidget::on_certificatesAliasComboBox_activated(const QString &alias)
{
    if (alias.length())
        m_step->setCertificateAlias(alias);
}

void AndroidPackageCreationWidget::on_certificatesAliasComboBox_currentIndexChanged(const QString &alias)
{
    if (alias.length())
        m_step->setCertificateAlias(alias);
}

void AndroidPackageCreationWidget::on_openPackageLocationCheckBox_toggled(bool checked)
{
    m_step->setOpenPackageLocation(checked);
}

} // namespace Internal
} // namespace Android



