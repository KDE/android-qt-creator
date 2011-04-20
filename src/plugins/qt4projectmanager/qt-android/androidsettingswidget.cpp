/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidsettingswidget.h"

#include "ui_androidsettingswidget.h"

#include "androidconfigurations.h"

#include "androidconstants.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QIntValidator>
#include <QtCore/QModelIndex>


namespace Qt4ProjectManager {
namespace Internal {

void AVDModel::setAvdList(QVector<AndroidDevice> list)
{
    m_list= list;
    reset();
}

QString AVDModel::avdName(const QModelIndex & index)
{
    return m_list[index.row()].serialNumber;
}

QVariant AVDModel::data( const QModelIndex & index, int role) const
{
    if (role != Qt::DisplayRole || !index.isValid())
        return QVariant();
    switch (index.column())
    {
        case 0:
            return m_list[index.row()].serialNumber;
        case 1:
            return QString("android-%1").arg(m_list[index.row()].sdk);
    }
    return QVariant();
}

QVariant AVDModel::headerData ( int section, Qt::Orientation orientation, int role ) const
{
    if (orientation == Qt::Horizontal &&  role == Qt::DisplayRole)
    {
        switch (section)
        {
            case 0:
                return tr("AVD Name");
            case 1:
                return tr("AVD Target");
        }
    }
    return  QAbstractItemModel::headerData(section, orientation, role );
}

int AVDModel::rowCount ( const QModelIndex & /*parent*/) const
{
    return m_list.size();
}

int AVDModel::columnCount ( const QModelIndex & /*parent*/) const
{
    return 2;
}


AndroidSettingsWidget::AndroidSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_ui(new Ui_AndroidSettingsWidget),
      m_androidConfig(AndroidConfigurations::instance().config()),
      m_saveSettingsRequested(false)
{
    initGui();
}

AndroidSettingsWidget::~AndroidSettingsWidget()
{
    if (m_saveSettingsRequested)
        AndroidConfigurations::instance().setConfig(m_androidConfig);
    delete m_ui;
}

QString AndroidSettingsWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << m_ui->SDKLocationLabel->text()
        << ' ' << m_ui->SDKLocationLineEdit->text()
        << ' ' << m_ui->NDKLocationLabel->text()
        << ' ' << m_ui->NDKLocationLineEdit->text()
        << ' ' << m_ui->NDKToolchainVersionLabel->text()
        << ' ' << m_ui->AntLocationLabel->text()
        << ' ' << m_ui->AntLocationLineEdit->text()
        << ' ' << m_ui->GdbLocationLabel->text()
        << ' ' << m_ui->GdbLocationLineEdit->text()
        << ' ' << m_ui->AVDManagerLabel->text()
        << ' ' << m_ui->DataPartitionSizeLable->text()
        << ' ' << m_ui->DataPartitionSizeSpinBox->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

void AndroidSettingsWidget::initGui()
{
    m_ui->setupUi(this);
    m_ui->toolchainVersionComboBox->clear();
    if (checkSDK(m_androidConfig.SDKLocation))
        m_ui->SDKLocationLineEdit->setText(m_androidConfig.SDKLocation);
    else
        m_androidConfig.SDKLocation="";
    if (checkNDK(m_androidConfig.NDKLocation))
        m_ui->NDKLocationLineEdit->setText(m_androidConfig.NDKLocation);
    else
        m_androidConfig.NDKLocation="";
    m_ui->AntLocationLineEdit->setText(m_androidConfig.AntLocation);
    m_ui->GdbLocationLineEdit->setText(m_androidConfig.GdbLocation);
    m_ui->DataPartitionSizeSpinBox->setValue(m_androidConfig.PartitionSize);
    m_ui->AVDTableView->setModel(&m_AVDModel);
    m_AVDModel.setAvdList(AndroidConfigurations::instance().androidVirtualDevices());
    m_ui->AVDTableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    m_ui->AVDTableView->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
}

void AndroidSettingsWidget::saveSettings(bool saveNow)
{
    // We must defer this step because of a stupid bug on MacOS. See QTCREATORBUG-1675.
    if (saveNow)
    {
        AndroidConfigurations::instance().setConfig(m_androidConfig);
        m_saveSettingsRequested = false;
    }
    else
        m_saveSettingsRequested = true;
}


bool AndroidSettingsWidget::checkSDK(const QString & location)
{
    if (!location.length())
        return false;
    if ( !QFile::exists(location+QLatin1String("/platform-tools/adb" ANDROID_EXE_SUFFIX)) || 
        (!QFile::exists(location+QLatin1String("/tools/android" ANDROID_EXE_SUFFIX)) && !QFile::exists(location+QLatin1String("/tools/android" ANDROID_BAT_SUFFIX))) || 
         !QFile::exists(location+QLatin1String("/tools/emulator" ANDROID_EXE_SUFFIX)) )
    {
        QMessageBox::critical(this, tr("Android SDK Folder"), tr("\"%1\" doesn't seem to be an Android SDK top folder").arg(location));
        return false;
    }
    return true;
}

bool AndroidSettingsWidget::checkNDK(const QString & location)
{
    m_ui->toolchainVersionComboBox->setEnabled(false);
    m_ui->GdbLocationLineEdit->setEnabled(false);
    m_ui->GdbLocationPushButton->setEnabled(false);
    if (!location.length())
        return false;
    if (!QFile::exists(location+QLatin1String("/platforms")) || !QFile::exists(location+QLatin1String("/toolchains")) || !QFile::exists(location+QLatin1String("/sources/cxx-stl")) )
    {
        QMessageBox::critical(this, tr("Android SDK Folder"), tr("\"%1\" doesn't seem to be an Android NDK top folder").arg(location));
        return false;
    }
    m_ui->toolchainVersionComboBox->setEnabled(true);
    m_ui->GdbLocationLineEdit->setEnabled(true);
    m_ui->GdbLocationPushButton->setEnabled(true);
    fillToolchainVersions();
    return true;

}

void AndroidSettingsWidget::SDKLocationEditingFinished()
{
    QString location=m_ui->SDKLocationLineEdit->text();
    if (!checkSDK(location))
    {
        m_ui->AVDManagerFrame->setEnabled(false);
        return;
    }
    m_androidConfig.SDKLocation = location;
    saveSettings(true);
    m_AVDModel.setAvdList(AndroidConfigurations::instance().androidVirtualDevices());
    m_ui->AVDManagerFrame->setEnabled(true);
}

void AndroidSettingsWidget::NDKLocationEditingFinished()
{
    QString location=m_ui->NDKLocationLineEdit->text();
    if (!checkNDK(location))
        return;
    m_androidConfig.NDKLocation = location;
    saveSettings(true);
}

void AndroidSettingsWidget::fillToolchainVersions()
{
    m_ui->toolchainVersionComboBox->clear();
    QStringList toolchainVersions=AndroidConfigurations::instance().ndkToolchainVersions();
    QString toolchain=m_androidConfig.NDKToolchainVersion;
    foreach(QString item, toolchainVersions)
        m_ui->toolchainVersionComboBox->addItem(item);
    m_ui->toolchainVersionComboBox->setCurrentIndex(toolchainVersions.indexOf(toolchain));
}

void AndroidSettingsWidget::toolchainVersionIndexChanged(QString version)
{
    m_androidConfig.NDKToolchainVersion=version;
    saveSettings(true);
}


void AndroidSettingsWidget::AntLocationEditingFinished()
{
    QString location=m_ui->AntLocationLineEdit->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.AntLocation = location;
}

void AndroidSettingsWidget::GdbLocationEditingFinished()
{
    QString location=m_ui->GdbLocationLineEdit->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.GdbLocation = location;
}

void AndroidSettingsWidget::browseSDKLocation()
{
    QString dir=QFileDialog::getExistingDirectory(this, tr("Select Android SDK folder"));
    if (!checkSDK(dir))
        return;
    m_ui->SDKLocationLineEdit->setText(dir);
    SDKLocationEditingFinished();
}

void AndroidSettingsWidget::browseNDKLocation()
{
    QString dir=QFileDialog::getExistingDirectory(this, tr("Select Android NDK folder"));
    if (!checkNDK(dir))
        return;
    m_ui->NDKLocationLineEdit->setText(dir);
    NDKLocationEditingFinished();
}

void AndroidSettingsWidget::browseAntLocation()
{
    QString dir=QDir::homePath();
#ifdef Q_OS_LINUX
    dir=QLatin1String("/usr/bin/ant");
    QLatin1String antApp("ant");
#elif defined(Q_OS_WIN)
    QLatin1String antApp("ant.bat");
#endif
    QString file=QFileDialog::getOpenFileName(this, tr("Select ant file"),dir,antApp);
    if (!file.length())
        return;
    m_ui->AntLocationLineEdit->setText(file);
    AntLocationEditingFinished();
}


void AndroidSettingsWidget::browseGdbLocation()
{
    QString gdbPath=AndroidConfigurations::instance().gdbPath();
    QString file=QFileDialog::getOpenFileName(this, tr("Select gdb file"),gdbPath);
    if (!file.length())
        return;
    m_ui->GdbLocationLineEdit->setText(file);
    GdbLocationEditingFinished();
}

void AndroidSettingsWidget::addAVD()
{
    AndroidConfigurations::instance().createAVD();
    m_AVDModel.setAvdList(AndroidConfigurations::instance().androidVirtualDevices());
}

void AndroidSettingsWidget::removeAVD()
{
    AndroidConfigurations::instance().removeAVD(m_AVDModel.avdName(m_ui->AVDTableView->currentIndex()));
    m_AVDModel.setAvdList(AndroidConfigurations::instance().androidVirtualDevices());
}

void AndroidSettingsWidget::startAVD()
{
    AndroidConfigurations::instance().startAVD(-1, m_AVDModel.avdName(m_ui->AVDTableView->currentIndex()));
}

void AndroidSettingsWidget::avdActivated(QModelIndex index)
{
    m_ui->AVDRemovePushButton->setEnabled(index.isValid());
    m_ui->AVDStartPushButton->setEnabled(index.isValid());
}

void AndroidSettingsWidget::DataPartitionSizeEditingFinished()
{
    m_androidConfig.PartitionSize=m_ui->DataPartitionSizeSpinBox->value();
}

} // namespace Internal
} // namespace Qt4ProjectManager
