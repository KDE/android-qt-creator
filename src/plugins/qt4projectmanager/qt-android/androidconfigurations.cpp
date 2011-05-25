/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "ui_addnewavddialog.h"

#include <coreplugin/icore.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>
#include <QtCore/QStringBuilder>
#include <QtCore/QStringList>
#include <QtCore/QProcess>
#include <QtCore/QFileInfo>
#include <QtCore/QDirIterator>
#include <QtGui/QStringListModel>
#include <QtGui/QDesktopServices>
#include <QDebug>

#if defined(_WIN32)
#include <iostream>
#include <windows.h>
#define sleep(_n) Sleep(1000 * (_n))
#endif

namespace Qt4ProjectManager {
namespace Internal {

namespace {
    const QLatin1String SettingsGroup("AndroidConfigurations");
    const QLatin1String SDKLocationKey("SDKLocation");
    const QLatin1String NDKLocationKey("NDKLocation");
    const QLatin1String NDKToolchainVersionKey("NDKToolchainVersion");
    const QLatin1String AntLocationKey("AntLocation");
    const QLatin1String GdbLocationKey("GdbLocation");
    const QLatin1String GdbserverLocationKey("GdbserverLocation");
    const QLatin1String PartitionSizeKey("PartitionSize");
    bool androidDevicesLessThan(const AndroidDevice & dev1, const AndroidDevice & dev2)
    {
        return dev1.sdk< dev2.sdk;
    }
}


AndroidConfig::AndroidConfig(const QSettings &settings)
    : SDKLocation(settings.value(SDKLocationKey).toString()),
      NDKLocation(settings.value(NDKLocationKey).toString()),
      NDKToolchainVersion(settings.value(NDKToolchainVersionKey).toString()),
      AntLocation(settings.value(AntLocationKey).toString()),
      GdbLocation(settings.value(GdbLocationKey).toString()),
      GdbserverLocation(settings.value(GdbserverLocationKey).toString()),
      PartitionSize(settings.value(PartitionSizeKey, 1024).toInt())
{
}

AndroidConfig::AndroidConfig()
{
    PartitionSize = 1024;
}

void AndroidConfig::save(QSettings &settings) const
{
    settings.setValue(SDKLocationKey, SDKLocation);
    settings.setValue(NDKLocationKey, NDKLocation);
    settings.setValue(NDKToolchainVersionKey, NDKToolchainVersion);
    settings.setValue(AntLocationKey, AntLocation);
    settings.setValue(GdbLocationKey, GdbLocation);
    settings.setValue(GdbserverLocationKey, GdbserverLocation);
    settings.setValue(PartitionSizeKey, PartitionSize);
}

void AndroidConfigurations::setConfig(const AndroidConfig &devConfigs)
{
    m_config = devConfigs;
    save();
    updateAvailablePlatforms();
    emit updated();
}

void AndroidConfigurations::updateAvailablePlatforms()
{
    m_availablePlatforms.clear();
    QDirIterator it(m_config.NDKLocation+"/platforms", QStringList()<<"android-*", QDir::Dirs);
    while(it.hasNext())
    {
        const QString & fileName=it.next();
        m_availablePlatforms.push_back(fileName.mid(fileName.lastIndexOf('-')+1).toInt());
    }
    qSort(m_availablePlatforms.begin(), m_availablePlatforms.end(), qGreater<int>());
}

QStringList AndroidConfigurations::sdkTargets()
{
    QStringList targets;
    QProcess proc;
    proc.start(androidToolPath(), QStringList() << "list" << "target"); // list avaialbe AVDs
    if (!proc.waitForFinished(-1))
    {
        proc.terminate();
        return targets;
    }
    QList<QByteArray> avds=proc.readAll().trimmed().split('\n');
    for (int i=0;i<avds.size();i++)
    {
        QString line=avds[i];
        int index = line.indexOf("\"android-");
        if (-1 == index)
            continue;
        targets.push_back(line.mid(index+1, line.length()-index-2));
    }
    return targets;
}

QStringList AndroidConfigurations::ndkToolchainVersions()
{
#ifdef __GNUC__
#warning TODO list the content of NDK_path/toolchains and get only the folders which contain "prebuilt" folder
#endif
    return QStringList()<<"arm-linux-androideabi-4.4.3";
}

QString AndroidConfigurations::adbToolPath()
{
    return m_config.SDKLocation+QLatin1String("/platform-tools/adb"ANDROID_EXE_SUFFIX);
}

QString AndroidConfigurations::androidToolPath()
{
#ifdef Q_OS_WIN32
    // I want to switch from using android.bat to using an executable. All it really does is call
    // Java and I've made some progress on it. So if android.exe exists, return that instead.
    QFileInfo fi(m_config.SDKLocation+QLatin1String("/tools/android"ANDROID_EXE_SUFFIX));
    if (fi.exists())
        return m_config.SDKLocation + QString("/tools/android"ANDROID_EXE_SUFFIX);
    else
        return m_config.SDKLocation+QLatin1String("/tools/android"ANDROID_BAT_SUFFIX);
#else
    return m_config.SDKLocation+QLatin1String("/tools/android"ANDROID_EXE_SUFFIX);
#endif
}

QString AndroidConfigurations::antToolPath()
{
    if (m_config.AntLocation.length())
        return m_config.AntLocation;
    else
        return QLatin1String("ant");
}

QString AndroidConfigurations::emulatorToolPath()
{
    return m_config.SDKLocation+QString("/tools/emulator"ANDROID_EXE_SUFFIX);
}

QString AndroidConfigurations::stripPath()
{
    return m_config.NDKLocation+QString("/toolchains/%1/prebuilt/%2/bin/%3-strip"ANDROID_EXE_SUFFIX).arg(m_config.NDKToolchainVersion).arg(ToolchainHost).arg(m_config.NDKToolchainVersion.left(m_config.NDKToolchainVersion.lastIndexOf('-')));
}

QString AndroidConfigurations::readelfPath()
{
    return m_config.NDKLocation+QString("/toolchains/%1/prebuilt/%2/bin/%3-readelf"ANDROID_EXE_SUFFIX).arg(m_config.NDKToolchainVersion).arg(ToolchainHost).arg(m_config.NDKToolchainVersion.left(m_config.NDKToolchainVersion.lastIndexOf('-')));
}

QString AndroidConfigurations::gccPath()
{
    return m_config.NDKLocation+QString("/toolchains/%1/prebuilt/%2/bin/%3-gcc"ANDROID_EXE_SUFFIX).arg(m_config.NDKToolchainVersion).arg(ToolchainHost).arg(m_config.NDKToolchainVersion.left(m_config.NDKToolchainVersion.lastIndexOf('-')));
}

QString AndroidConfigurations::gdbServerPath()
{
    if (m_config.GdbserverLocation.length())
        return m_config.GdbserverLocation;
    return m_config.NDKLocation+QString("/toolchains/%1/prebuilt/gdbserver").arg(m_config.NDKToolchainVersion);
}

QString AndroidConfigurations::gdbPath()
{
    if (m_config.GdbLocation.length())
        return m_config.GdbLocation;
    return m_config.NDKLocation+QString("/toolchains/%1/prebuilt/%2/bin/%3-gdb"ANDROID_EXE_SUFFIX).arg(m_config.NDKToolchainVersion).arg(ToolchainHost).arg(m_config.NDKToolchainVersion.left(m_config.NDKToolchainVersion.lastIndexOf('-')));
}

QString AndroidConfigurations::getDeployDeviceSerialNumber(int apiLevel)
{
    QVector<AndroidDevice> devices=connectedDevices();

    foreach(AndroidDevice device, devices)
        if (device.sdk>=apiLevel)
            return device.serialNumber;

    return startAVD(apiLevel);
}

QVector<AndroidDevice> AndroidConfigurations::connectedDevices(int apiLevel)
{
    QVector<AndroidDevice> devices;
    QProcess adbProc;
    adbProc.start(adbToolPath(), QStringList() << "devices");
    if (!adbProc.waitForFinished(-1))
    {
        adbProc.terminate();
        return devices;
    }
    QList<QByteArray> adbDevs=adbProc.readAll().trimmed().split('\n');
    adbDevs.removeFirst();
    AndroidDevice dev;
    foreach(QByteArray device, adbDevs)
    {
        dev.serialNumber=device.left(device.indexOf('\t')).trimmed();
        dev.sdk=getSDKVersion(dev.serialNumber);
        if (-1 != apiLevel && dev.sdk != apiLevel)
            continue;
        devices.push_back(dev);
    }
    qSort(devices.begin(), devices.end(), androidDevicesLessThan);
    return devices;
}

bool AndroidConfigurations::createAVD()
{
    QRegExp rx("\\S+");
    QRegExpValidator v(rx, 0);
    QDialog d;
    Ui::AddNewAVDDialog avdDialog;
    avdDialog.setupUi(&d);
    QStringListModel model(sdkTargets());
    avdDialog.targetComboBox->setModel(&model);
    avdDialog.nameLineEdit->setValidator(&v);
    if (d.exec()!=QDialog::Accepted)
        return false;
    return createAVD(avdDialog.targetComboBox->currentText(), avdDialog.nameLineEdit->text(), avdDialog.sizeSpinBox->value());
}

bool AndroidConfigurations::createAVD(const QString & target, const QString & name, int sdcardSize )
{
    QProcess proc;
    proc.start(androidToolPath(), QStringList() << "create" << "avd" << "-t" << target
            << "-n" << name << "-c" << QString::fromLatin1("%4M").arg(sdcardSize));
    if (!proc.waitForStarted())
        return false;
    proc.write(QByteArray("no\n"));
    if (!proc.waitForFinished(-1))
    {
        proc.terminate();
        return false;
    }
    return !proc.exitCode();
}

bool AndroidConfigurations::removeAVD(const QString & name)
{
    QProcess proc;
    proc.start(androidToolPath(), QStringList() << "delete" << "avd" << "-n" << name); // list avaialbe AVDs
    if (!proc.waitForFinished(-1))
    {
        proc.terminate();
        return false;
    }
    return !proc.exitCode();
}

QVector<AndroidDevice> AndroidConfigurations::androidVirtualDevices()
{
    QVector<AndroidDevice> devices;
    QProcess proc;
    proc.start(androidToolPath(), QStringList() << "list" << "avd"); // list available AVDs
    if (!proc.waitForFinished(-1))
    {
        proc.terminate();
        return devices;
    }
    QList<QByteArray> avds=proc.readAll().trimmed().split('\n');
    avds.removeFirst();
    AndroidDevice dev;
    for (int i=0;i<avds.size();i++)
    {
        QString line=avds[i];
        if (!line.contains("Name:"))
            continue;

        dev.serialNumber=line.mid(line.indexOf(':')+2).trimmed();
        ++i;
        for(;i<avds.size();++i)
        {
            line=avds[i];
            if (!line.contains("Target:"))
                continue;
            dev.sdk=line.mid(line.lastIndexOf(' ')).remove(')').toInt();
            break;
        }
        ++i;
        devices.push_back(dev);
    }
    qSort(devices.begin(), devices.end(), androidDevicesLessThan);

    return devices;
}

QString AndroidConfigurations::startAVD(int apiLevel, const QString & name)
{
    QProcess * m_avdProcess = new QProcess();
    connect(this, SIGNAL(destroyed()), m_avdProcess, SLOT(deleteLater()));
    connect(m_avdProcess, SIGNAL(finished(int)), m_avdProcess, SLOT(deleteLater()));

    QString avdName = name;
    QVector<AndroidDevice> devices;
    if (!avdName.length())
    {
        devices=androidVirtualDevices();
        foreach(AndroidDevice device, devices)
            if (device.sdk>=apiLevel) // take first emulator how supports this package
            {
                avdName=device.serialNumber;
                break;
            }
    }
    // if no emulators found try to create one
    if (!avdName.length())
    {
        // if called from getDeployDeviceSerialNumber from depolyPackage,
        // this causes an assert:
        // "Widgets must be created in the GUI thread."
        avdName=createAVD();
    }

    if (!avdName.length())// stop here if no emulators found
        return avdName;

    // start the emulator
    m_avdProcess->start(emulatorToolPath(), QStringList() << "-partition-size" << QString::number(config().PartitionSize) << "-avd" << avdName);
    if (!m_avdProcess->waitForStarted(-1))
        return QString();

    // wait until the emulator is online
    QProcess proc;
    proc.start(adbToolPath(), QStringList() << QLatin1String("-e") << "wait-for-device");
    if (!proc.waitForFinished(-1))
    {
        proc.terminate();
        return QString();
    }
    sleep(5);// wait for pm to start

    // workaround for stupid adb bug
    proc.start(adbToolPath(), QStringList() << QLatin1String("devices"));
    if (!proc.waitForFinished(-1))
    {
        proc.terminate();
        return QString();
    }

    // get connected devices
    devices = connectedDevices(apiLevel);
    foreach(AndroidDevice device, devices)
        if (device.sdk==apiLevel)
            return device.serialNumber;
    // this should not happen, but ...
    return QString();
}

int AndroidConfigurations::getSDKVersion(const QString & device)
{

    QProcess adbProc;
    adbProc.start(adbToolPath(), QStringList()<< "-s" << device << "shell" << "getprop" << "ro.build.version.sdk");
    if (!adbProc.waitForFinished(-1))
    {
        adbProc.terminate();
        return -1;
    }
    return adbProc.readAll().trimmed().toInt();
}

QString AndroidConfigurations::bestMatch(const QString &targetAPI)
{
    int target=targetAPI.mid(targetAPI.lastIndexOf('-')+1).toInt();
    foreach(int apiLevel, m_availablePlatforms)
    {
        if(apiLevel<=target)
            return QString("android-%1").arg(apiLevel);
    }
    return "android-4";
}

AndroidConfigurations &AndroidConfigurations::instance(QObject *parent)
{
    if (m_instance == 0)
        m_instance = new AndroidConfigurations(parent);
    return *m_instance;
}

void AndroidConfigurations::save()
{
    QSettings *settings = Core::ICore::instance()->settings(QSettings::SystemScope);
    settings->beginGroup(SettingsGroup);
    m_config.save(*settings);
    settings->endGroup();
}

AndroidConfigurations::AndroidConfigurations(QObject *parent)
    : QObject(parent)
{
    load();
    updateAvailablePlatforms();
}

void AndroidConfigurations::load()
{
    QSettings *settings = Core::ICore::instance()->settings(QSettings::SystemScope);
    settings->beginGroup(SettingsGroup);
    m_config=AndroidConfig(*settings);
    settings->endGroup();
}

AndroidConfigurations *AndroidConfigurations::m_instance = 0;

} // namespace Internal
} // namespace Qt4ProjectManager
