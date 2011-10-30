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
#include <QtCore/QProcess>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QIntValidator>
#include <QtCore/QModelIndex>


namespace Android {
namespace Internal {

void AVDModel::setAvdList(QVector<AndroidDevice> list)
{
    m_list = list;
    reset();
}

QString AVDModel::avdName(const QModelIndex &index)
{
    return m_list[index.row()].serialNumber;
}

QVariant AVDModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !index.isValid())
        return QVariant();
    switch (index.column()) {
        case 0:
            return m_list[index.row()].serialNumber;
        case 1:
            return QString("API %1").arg(m_list[index.row()].sdk);
        case 2:
            return m_list[index.row()].cpuABI;
    }
    return QVariant();
}

QVariant AVDModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal &&  role == Qt::DisplayRole) {
        switch (section) {
            case 0:
                return tr("AVD Name");
            case 1:
                return tr("AVD Target");
            case 2:
                return tr("CPU/ABI");
        }
    }
    return  QAbstractItemModel::headerData(section, orientation, role );
}

int AVDModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_list.size();
}

int AVDModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
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
        << ' ' << m_ui->GdbserverLocationLabel->text()
        << ' ' << m_ui->GdbserverLocationLineEdit->text()
        << ' ' << m_ui->GdbLocationLabelx86->text()
        << ' ' << m_ui->GdbLocationLineEditx86->text()
        << ' ' << m_ui->GdbserverLocationLabelx86->text()
        << ' ' << m_ui->GdbserverLocationLineEditx86->text()
        << ' ' << m_ui->OpenJDKLocationLabel->text()
        << ' ' << m_ui->OpenJDKLocationLineEdit->text()
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
    m_ui->GdbLocationLineEdit->setText(m_androidConfig.ArmGdbLocation);
    m_ui->GdbserverLocationLineEdit->setText(m_androidConfig.ArmGdbserverLocation);
    m_ui->GdbLocationLineEditx86->setText(m_androidConfig.X86GdbLocation);
    m_ui->GdbserverLocationLineEditx86->setText(m_androidConfig.X86GdbserverLocation);
    m_ui->OpenJDKLocationLineEdit->setText(m_androidConfig.OpenJDKLocation);
    m_ui->DataPartitionSizeSpinBox->setValue(m_androidConfig.PartitionSize);
    m_ui->AVDTableView->setModel(&m_AVDModel);
    m_AVDModel.setAvdList(AndroidConfigurations::instance().androidVirtualDevices());
    m_ui->AVDTableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    m_ui->AVDTableView->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
}

void AndroidSettingsWidget::saveSettings(bool saveNow)
{
    // We must defer this step because of a stupid bug on MacOS. See QTCREATORBUG-1675.
    if (saveNow) {
        AndroidConfigurations::instance().setConfig(m_androidConfig);
        m_saveSettingsRequested = false;
    } else {
        m_saveSettingsRequested = true;
    }
}


bool AndroidSettingsWidget::checkSDK(const QString & location)
{
    if (!location.length())
        return false;
    if (!QFile::exists(location+QLatin1String("/platform-tools/adb" ANDROID_EXE_SUFFIX))
            || (!QFile::exists(location+QLatin1String("/tools/android" ANDROID_EXE_SUFFIX))
                && !QFile::exists(location+QLatin1String("/tools/android" ANDROID_BAT_SUFFIX)))
            || !QFile::exists(location+QLatin1String("/tools/emulator" ANDROID_EXE_SUFFIX)))
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
    m_ui->GdbserverLocationLineEdit->setEnabled(false);
    m_ui->GdbserverLocationPushButton->setEnabled(false);
    if (!location.length())
        return false;
    if (!QFile::exists(location+QLatin1String("/platforms"))
            || !QFile::exists(location+QLatin1String("/toolchains"))
            || !QFile::exists(location+QLatin1String("/sources/cxx-stl")))
    {
        QMessageBox::critical(this, tr("Android SDK Folder"), tr("\"%1\" doesn't seem to be an Android NDK top folder").arg(location));
        return false;
    }
    m_ui->toolchainVersionComboBox->setEnabled(true);
    m_ui->GdbLocationLineEdit->setEnabled(true);
    m_ui->GdbLocationPushButton->setEnabled(true);
    m_ui->GdbserverLocationLineEdit->setEnabled(true);
    m_ui->GdbserverLocationPushButton->setEnabled(true);
    fillToolchainVersions();
    return true;

}

void AndroidSettingsWidget::SDKLocationEditingFinished()
{
    QString location = m_ui->SDKLocationLineEdit->text();
    if (!checkSDK(location)) {
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
    QString location = m_ui->NDKLocationLineEdit->text();
    if (!checkNDK(location))
        return;
    m_androidConfig.NDKLocation = location;
    saveSettings(true);
}

void AndroidSettingsWidget::fillToolchainVersions()
{
    m_ui->toolchainVersionComboBox->clear();
    QStringList toolchainVersions = AndroidConfigurations::instance().ndkToolchainVersions();
    QString toolchain = m_androidConfig.NDKToolchainVersion;
    foreach (const QString &item, toolchainVersions)
        m_ui->toolchainVersionComboBox->addItem(item);
    if (!toolchain.isEmpty())
        m_ui->toolchainVersionComboBox->setCurrentIndex(toolchainVersions.indexOf(toolchain));
    else
        m_ui->toolchainVersionComboBox->setCurrentIndex(0);
}

void AndroidSettingsWidget::toolchainVersionIndexChanged(QString version)
{
    m_androidConfig.NDKToolchainVersion = version;
    saveSettings(true);
}


void AndroidSettingsWidget::AntLocationEditingFinished()
{
    QString location = m_ui->AntLocationLineEdit->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.AntLocation = location;
}

void AndroidSettingsWidget::GdbLocationEditingFinished()
{
    QString location = m_ui->GdbLocationLineEdit->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.ArmGdbLocation = location;
}

void AndroidSettingsWidget::GdbserverLocationEditingFinished()
{
    QString location = m_ui->GdbserverLocationLineEdit->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.ArmGdbserverLocation = location;
}

void AndroidSettingsWidget::GdbLocationX86EditingFinished()
{
    QString location = m_ui->GdbLocationLineEditx86->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.X86GdbLocation = location;
}

void AndroidSettingsWidget::GdbserverLocationX86EditingFinished()
{
    QString location = m_ui->GdbserverLocationLineEditx86->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.X86GdbserverLocation = location;
}

void AndroidSettingsWidget::OpenJDKLocationEditingFinished()
{
    QString location = m_ui->OpenJDKLocationLineEdit->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.OpenJDKLocation = location;
}

void AndroidSettingsWidget::browseSDKLocation()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Android SDK folder"));
    if (!checkSDK(dir))
        return;
    m_ui->SDKLocationLineEdit->setText(dir);
    SDKLocationEditingFinished();
}

void AndroidSettingsWidget::browseNDKLocation()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Android NDK folder"));
    if (!checkNDK(dir))
        return;
    m_ui->NDKLocationLineEdit->setText(dir);
    NDKLocationEditingFinished();
}

void AndroidSettingsWidget::browseAntLocation()
{
    QString dir = QDir::homePath();
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    dir = QLatin1String("/usr/bin/ant");
    QLatin1String antApp("ant");
#elif defined(Q_OS_WIN)
    QLatin1String antApp("ant.bat");
#elif defined(Q_OS_DARWIN)
    dir = QLatin1String("/opt/local/bin/ant");
    QLatin1String antApp("ant");
#endif
    QString file = QFileDialog::getOpenFileName(this, tr("Select ant script"),dir,antApp);
    if (!file.length())
        return;
    m_ui->AntLocationLineEdit->setText(file);
    AntLocationEditingFinished();
}

void AndroidSettingsWidget::browseGdbLocation()
{
    QString gdbPath = AndroidConfigurations::instance().gdbPath(ProjectExplorer::Abi::ArmArchitecture);
    QString file = QFileDialog::getOpenFileName(this, tr("Select gdb executable"),gdbPath);
    if (!file.length())
        return;
    m_ui->GdbLocationLineEdit->setText(file);
    GdbLocationEditingFinished();
}

void AndroidSettingsWidget::browseGdbserverLocation()
{
    QString gdbserverPath = AndroidConfigurations::instance().gdbServerPath(ProjectExplorer::Abi::ArmArchitecture);
    QString file = QFileDialog::getOpenFileName(this, tr("Select gdbserver android executable"),gdbserverPath);
    if (!file.length())
        return;
    m_ui->GdbserverLocationLineEdit->setText(file);
    GdbserverLocationEditingFinished();
}

void AndroidSettingsWidget::browseGdbLocationX86()
{
    QString gdbPath = AndroidConfigurations::instance().gdbPath(ProjectExplorer::Abi::X86Architecture);
    QString file = QFileDialog::getOpenFileName(this, tr("Select gdb executable"),gdbPath);
    if (!file.length())
        return;
    m_ui->GdbLocationLineEditx86->setText(file);
    GdbLocationX86EditingFinished();
}

void AndroidSettingsWidget::browseGdbserverLocationX86()
{
    QString gdbserverPath = AndroidConfigurations::instance().gdbServerPath(ProjectExplorer::Abi::X86Architecture);
    QString file = QFileDialog::getOpenFileName(this, tr("Select gdbserver android executable"),gdbserverPath);
    if (!file.length())
        return;
    m_ui->GdbserverLocationLineEditx86->setText(file);
    GdbserverLocationX86EditingFinished();
}

void AndroidSettingsWidget::browseOpenJDKLocation()
{
    QString openJDKPath = AndroidConfigurations::instance().openJDKPath();
    QString file = QFileDialog::getOpenFileName(this, tr("Select OpenJDK path"),openJDKPath);
    if (!file.length())
        return;
    m_ui->OpenJDKLocationLineEdit->setText(file);
    OpenJDKLocationEditingFinished();
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
    int tempApiLevel=-1;
    AndroidConfigurations::instance().startAVD(tempApiLevel, m_AVDModel.avdName(m_ui->AVDTableView->currentIndex()));
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

void AndroidSettingsWidget::manageAVD()
{
    QProcess *avdProcess = new QProcess();
    connect(this, SIGNAL(destroyed()), avdProcess, SLOT(deleteLater()));
    connect(avdProcess, SIGNAL(finished(int)), avdProcess, SLOT(deleteLater()));
    avdProcess->start(AndroidConfigurations::instance().androidToolPath(), QStringList() << "avd");
}


} // namespace Internal
} // namespace Android
