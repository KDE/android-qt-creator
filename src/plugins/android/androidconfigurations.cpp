/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "ui_addnewavddialog.h"

#include <coreplugin/icore.h>
#include <utils/persistentsettings.h>

#include <QDateTime>
#include <QSettings>
#include <QStringList>
#include <QProcess>
#include <QFileInfo>
#include <QDirIterator>
#include <QMetaObject>

#include <QStringListModel>
#include <QMessageBox>

#if defined(_WIN32)
#include <iostream>
#include <windows.h>
#define sleep(_n) Sleep(1000 * (_n))
#else
#include <unistd.h>
#endif

using namespace Utils;

namespace Android {
namespace Internal {

namespace {
    const QLatin1String SettingsGroup("AndroidConfigurations");
    const QLatin1String SDKLocationKey("SDKLocation");
    const QLatin1String NDKLocationKey("NDKLocation");
    const QLatin1String NDKToolchainVersionKey("NDKToolchainVersion");
    const QLatin1String AntLocationKey("AntLocation");
    const QLatin1String ArmGdbLocationKey("GdbLocation");
    const QLatin1String ArmGdbserverLocationKey("GdbserverLocation");
    const QLatin1String X86GdbLocationKey("X86GdbLocation");
    const QLatin1String X86GdbserverLocationKey("X86GdbserverLocation");
    const QLatin1String OpenJDKLocationKey("OpenJDKLocation");
    const QLatin1String KeystoreLocationKey("KeystoreLocation");
    const QLatin1String PartitionSizeKey("PartitionSize");
    const QLatin1String NDKGccVersionRegExp("\\d\\.\\d\\.\\d");
    const QLatin1String ArmToolchainPrefix("arm-linux-androideabi");
    const QLatin1String X86ToolchainPrefix("x86");
    const QLatin1String ArmToolsPrefix("arm-linux-androideabi");
    const QLatin1String X86ToolsPrefix("i686-android-linux");
    const QLatin1String Unknown("unknown");
    const QLatin1String keytoolName("keytool");
    const QLatin1String jarsignerName("jarsigner");
    const QLatin1String changeTimeStamp("ChangeTimeStamp");

    static QString settingsFileName()
    {
        return QString::fromLatin1("%1/qtcreator/android.xml").arg(
            QFileInfo(Core::ICore::settings(QSettings::SystemScope)->fileName()).absolutePath());
    }

    bool androidDevicesLessThan(const AndroidDeviceInfo &dev1, const AndroidDeviceInfo &dev2)
    {
        return dev1.sdk < dev2.sdk;
    }
}

QLatin1String AndroidConfigurations::toolchainPrefix(ProjectExplorer::Abi::Architecture architecture)
{
    switch (architecture) {
    case ProjectExplorer::Abi::ArmArchitecture:
        return ArmToolchainPrefix;
    case ProjectExplorer::Abi::X86Architecture:
        return X86ToolchainPrefix;
    default:
        return Unknown;
    }
}


QLatin1String AndroidConfigurations::toolsPrefix(ProjectExplorer::Abi::Architecture architecture)
{
    switch (architecture) {
    case ProjectExplorer::Abi::ArmArchitecture:
        return ArmToolsPrefix;
    case ProjectExplorer::Abi::X86Architecture:
        return X86ToolsPrefix;
    default:
        return Unknown;
    }
}

AndroidConfig::AndroidConfig(const QSettings &settings)
{
    // user settings
    armGdbLocation = Utils::FileName::fromString(settings.value(ArmGdbLocationKey).toString());
    armGdbserverLocation = Utils::FileName::fromString(settings.value(ArmGdbserverLocationKey).toString());
    x86GdbLocation = Utils::FileName::fromString(settings.value(X86GdbLocationKey).toString());
    x86GdbserverLocation = Utils::FileName::fromString(settings.value(X86GdbserverLocationKey).toString());
    partitionSize = settings.value(PartitionSizeKey, 1024).toInt();
    sdkLocation = Utils::FileName::fromString(settings.value(SDKLocationKey).toString());
    ndkLocation = Utils::FileName::fromString(settings.value(NDKLocationKey).toString());
    antLocation = Utils::FileName::fromString(settings.value(AntLocationKey).toString());
    openJDKLocation = Utils::FileName::fromString(settings.value(OpenJDKLocationKey).toString());
    keystoreLocation = Utils::FileName::fromString(settings.value(KeystoreLocationKey).toString());

    QRegExp versionRegExp(NDKGccVersionRegExp);
    const QString &value = settings.value(NDKToolchainVersionKey).toString();
    if (versionRegExp.exactMatch(value))
        ndkToolchainVersion = value;
    else
        ndkToolchainVersion = value.mid(versionRegExp.indexIn(value));
    // user settings

    PersistentSettingsReader reader;
    if (reader.load(settingsFileName())
            && settings.value(changeTimeStamp).toInt() != QFileInfo(settingsFileName()).lastModified().toMSecsSinceEpoch() / 1000) {
        // persisten settings
        sdkLocation = Utils::FileName::fromString(reader.restoreValue(SDKLocationKey).toString());
        ndkLocation = Utils::FileName::fromString(reader.restoreValue(NDKLocationKey).toString());
        antLocation = Utils::FileName::fromString(reader.restoreValue(AntLocationKey).toString());
        openJDKLocation = Utils::FileName::fromString(reader.restoreValue(OpenJDKLocationKey).toString());
        keystoreLocation = Utils::FileName::fromString(reader.restoreValue(KeystoreLocationKey).toString());

        QRegExp versionRegExp(NDKGccVersionRegExp);
        const QString &value = reader.restoreValue(NDKToolchainVersionKey).toString();
        if (versionRegExp.exactMatch(value))
            ndkToolchainVersion = value;
        else
            ndkToolchainVersion = value.mid(versionRegExp.indexIn(value));

        if (armGdbLocation.isEmpty())
            armGdbLocation = Utils::FileName::fromString(reader.restoreValue(ArmGdbLocationKey).toString());

        if (armGdbserverLocation.isEmpty())
            armGdbserverLocation = Utils::FileName::fromString(reader.restoreValue(ArmGdbserverLocationKey).toString());

        if (x86GdbLocation.isEmpty())
            x86GdbLocation = Utils::FileName::fromString(reader.restoreValue(X86GdbLocationKey).toString());

        if (x86GdbserverLocation.isEmpty())
            x86GdbserverLocation = Utils::FileName::fromString(reader.restoreValue(X86GdbserverLocationKey).toString());
        // persistent settings
    }

}

AndroidConfig::AndroidConfig()
{
    partitionSize = 1024;
}

void AndroidConfig::save(QSettings &settings) const
{
    QFileInfo fileInfo(settingsFileName());
    if (fileInfo.exists())
        settings.setValue(changeTimeStamp, fileInfo.lastModified().toMSecsSinceEpoch() / 1000);

    // user settings
    settings.setValue(SDKLocationKey, sdkLocation.toString());
    settings.setValue(NDKLocationKey, ndkLocation.toString());
    settings.setValue(NDKToolchainVersionKey, ndkToolchainVersion);
    settings.setValue(AntLocationKey, antLocation.toString());
    settings.setValue(OpenJDKLocationKey, openJDKLocation.toString());
    settings.setValue(KeystoreLocationKey, keystoreLocation.toString());
    settings.setValue(ArmGdbLocationKey, armGdbLocation.toString());
    settings.setValue(ArmGdbserverLocationKey, armGdbserverLocation.toString());
    settings.setValue(X86GdbLocationKey, x86GdbLocation.toString());
    settings.setValue(X86GdbserverLocationKey, x86GdbserverLocation.toString());
    settings.setValue(PartitionSizeKey, partitionSize);
    // user settings

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
    Utils::FileName path = m_config.ndkLocation;
    QDirIterator it(path.appendPath(QLatin1String("platforms")).toString(), QStringList() << QLatin1String("android-*"), QDir::Dirs);
    while (it.hasNext()) {
        const QString &fileName = it.next();
        m_availablePlatforms.push_back(fileName.mid(fileName.lastIndexOf(QLatin1Char('-')) + 1).toInt());
    }
    qSort(m_availablePlatforms.begin(), m_availablePlatforms.end(), qGreater<int>());
}

QStringList AndroidConfigurations::sdkTargets(int minApiLevel) const
{
    QStringList targets;
    QProcess proc;
    proc.start(androidToolPath().toString(), QStringList() << QLatin1String("list") << QLatin1String("target")); // list avaialbe AVDs
    if (!proc.waitForFinished(-1)) {
        proc.terminate();
        return targets;
    }
    while (proc.canReadLine()) {
        QString line = proc.readLine().trimmed();
        int index = line.indexOf(QLatin1String("\"android-"));
        if (index == -1)
            continue;
        QString apiLevel = line.mid(index + 1, line.length() - index - 2);
        if (apiLevel.mid(apiLevel.lastIndexOf(QLatin1Char('-')) + 1).toInt() >= minApiLevel)
            targets.push_back(apiLevel);
    }
    return targets;
}

QStringList AndroidConfigurations::ndkToolchainVersions() const
{
    QRegExp versionRegExp(NDKGccVersionRegExp);
    QStringList result;
    Utils::FileName path = m_config.ndkLocation;
    QDirIterator it(path.appendPath(QLatin1String("toolchains")).toString(),
                    QStringList() << QLatin1String("*"), QDir::Dirs);
    while (it.hasNext()) {
        const QString &fileName = it.next();
        int idx = versionRegExp.indexIn(fileName);
        if (idx == -1)
            continue;
        QString version = fileName.mid(idx);
        if (!result.contains(version))
            result.append(version);
    }
    return result;
}

Utils::FileName AndroidConfigurations::adbToolPath() const
{
    Utils::FileName path = m_config.sdkLocation;
    return path.appendPath(QLatin1String("platform-tools/adb" ANDROID_EXE_SUFFIX));
}

Utils::FileName AndroidConfigurations::androidToolPath() const
{
#ifdef Q_OS_WIN32
    // I want to switch from using android.bat to using an executable. All it really does is call
    // Java and I've made some progress on it. So if android.exe exists, return that instead.
    Utils::FileName path = m_config.sdkLocation;
    path.appendPath(QLatin1String("tools/android"ANDROID_EXE_SUFFIX));
    if (path.toFileInfo().exists())
        return path;
    path = m_config.sdkLocation;
    return path.appendPath(QLatin1String("tools/android"ANDROID_BAT_SUFFIX));
#else
    Utils::FileName path = m_config.sdkLocation;
    return path.appendPath(QLatin1String("tools/android"));
#endif
}

Utils::FileName AndroidConfigurations::antToolPath() const
{
    if (!m_config.antLocation.isEmpty())
        return m_config.antLocation;
    else
        return Utils::FileName::fromString(QLatin1String("ant"));
}

Utils::FileName AndroidConfigurations::emulatorToolPath() const
{
    Utils::FileName path = m_config.sdkLocation;
    return path.appendPath(QLatin1String("tools/emulator" ANDROID_EXE_SUFFIX));
}

Utils::FileName AndroidConfigurations::toolPath(ProjectExplorer::Abi::Architecture architecture) const
{
    Utils::FileName path = m_config.ndkLocation;
    return path.appendPath(QString::fromLatin1("toolchains/%1-%2/prebuilt/%3/bin/%4")
            .arg(toolchainPrefix(architecture))
            .arg(m_config.ndkToolchainVersion)
            .arg(ToolchainHost)
            .arg(toolsPrefix(architecture)));
}

Utils::FileName AndroidConfigurations::stripPath(ProjectExplorer::Abi::Architecture architecture) const
{
    return toolPath(architecture).append(QLatin1String("-strip" ANDROID_EXE_SUFFIX));
}

Utils::FileName AndroidConfigurations::readelfPath(ProjectExplorer::Abi::Architecture architecture) const
{
    return toolPath(architecture).append(QLatin1String("-readelf" ANDROID_EXE_SUFFIX));
}

Utils::FileName AndroidConfigurations::gccPath(ProjectExplorer::Abi::Architecture architecture) const
{
    return toolPath(architecture).append(QLatin1String("-gcc" ANDROID_EXE_SUFFIX));
}

Utils::FileName AndroidConfigurations::gdbServerPath(ProjectExplorer::Abi::Architecture architecture) const
{
    Utils::FileName gdbServerPath;
    switch (architecture) {
    case ProjectExplorer::Abi::ArmArchitecture:
        gdbServerPath = m_config.armGdbserverLocation;
        break;
    case ProjectExplorer::Abi::X86Architecture:
        gdbServerPath = m_config.x86GdbserverLocation;
        break;
    default:
        gdbServerPath = Utils::FileName::fromString(Unknown);
        break;
    }

    if (!gdbServerPath.isEmpty())
        return gdbServerPath;
    Utils::FileName path = m_config.ndkLocation;
    return path.appendPath(QString::fromLatin1("toolchains/%1-%2/prebuilt/gdbserver")
                           .arg(toolchainPrefix(architecture))
                           .arg(m_config.ndkToolchainVersion));
}

Utils::FileName AndroidConfigurations::gdbPath(ProjectExplorer::Abi::Architecture architecture) const
{
    Utils::FileName gdbPath;
    switch (architecture) {
    case ProjectExplorer::Abi::ArmArchitecture:
        gdbPath = m_config.armGdbLocation;
        break;
    case ProjectExplorer::Abi::X86Architecture:
        gdbPath = m_config.x86GdbLocation;
        break;
    default:
        gdbPath = Utils::FileName::fromString(Unknown);
        break;
    }
    if (!gdbPath.isEmpty())
        return gdbPath;
    return toolPath(architecture).append(QLatin1String("-gdb" ANDROID_EXE_SUFFIX));
}

Utils::FileName AndroidConfigurations::openJDKPath() const
{
    return m_config.openJDKLocation;
}

Utils::FileName AndroidConfigurations::openJDKBinPath() const
{
    Utils::FileName path = m_config.openJDKLocation;
    if (!path.isEmpty())
        return path.appendPath(QLatin1String("bin"));
    return path;
}

Utils::FileName AndroidConfigurations::keytoolPath() const
{
    return openJDKBinPath().appendPath(keytoolName);
}

Utils::FileName AndroidConfigurations::jarsignerPath() const
{
    return openJDKBinPath().appendPath(jarsignerName);
}

Utils::FileName AndroidConfigurations::zipalignPath() const
{
    Utils::FileName path = m_config.sdkLocation;
    return path.appendPath(QLatin1String("tools/zipalign" ANDROID_EXE_SUFFIX));
}

QString AndroidConfigurations::getDeployDeviceSerialNumber(int *apiLevel) const
{
    QVector<AndroidDeviceInfo> devices = connectedDevices();

    foreach (AndroidDeviceInfo device, devices) {
        if (device.sdk >= *apiLevel) {
            *apiLevel = device.sdk;
            return device.serialNumber;
        }
    }
    return startAVD(apiLevel);
}

QVector<AndroidDeviceInfo> AndroidConfigurations::connectedDevices(int apiLevel) const
{
    QVector<AndroidDeviceInfo> devices;
    QProcess adbProc;
    adbProc.start(adbToolPath().toString(), QStringList() << QLatin1String("devices"));
    if (!adbProc.waitForFinished(-1)) {
        adbProc.terminate();
        return devices;
    }
    QList<QByteArray> adbDevs = adbProc.readAll().trimmed().split('\n');
    adbDevs.removeFirst();
    AndroidDeviceInfo dev;
    foreach (const QByteArray &device, adbDevs) {
        dev.serialNumber = QString::fromLatin1(device.left(device.indexOf('\t')).trimmed());
        dev.sdk = getSDKVersion(dev.serialNumber);
        if (apiLevel != -1 && dev.sdk != apiLevel)
            continue;
        devices.push_back(dev);
    }
    qSort(devices.begin(), devices.end(), androidDevicesLessThan);
    return devices;
}

bool AndroidConfigurations::createAVD(int minApiLevel) const
{
    QDialog d;
    Ui::AddNewAVDDialog avdDialog;
    avdDialog.setupUi(&d);
    QStringListModel model(sdkTargets(minApiLevel));
    avdDialog.targetComboBox->setModel(&model);
    if (!model.rowCount()) {
        QMessageBox::critical(0, tr("Create AVD Error"),
                              tr("Cannot create a new AVD, not enough android SDKs available\n"
                                 "Please install one SDK with api version >=%1").
                              arg(minApiLevel));
        return false;
    }

    QRegExp rx(QLatin1String("\\S+"));
    QRegExpValidator v(rx, 0);
    avdDialog.nameLineEdit->setValidator(&v);
    if (d.exec() != QDialog::Accepted)
        return false;
    return createAVD(avdDialog.targetComboBox->currentText(), avdDialog.nameLineEdit->text(), avdDialog.sizeSpinBox->value());
}

bool AndroidConfigurations::createAVD(const QString &target, const QString &name, int sdcardSize ) const
{
    QProcess proc;
    proc.start(androidToolPath().toString(),
               QStringList() << QLatin1String("create") << QLatin1String("avd")
               << QLatin1String("-a") << QLatin1String("-t") << target
               << QLatin1String("-n") << name
               << QLatin1String("-c") << QString::fromLatin1("%1M").arg(sdcardSize));
    if (!proc.waitForStarted())
        return false;
    proc.write(QByteArray("no\n"));
    if (!proc.waitForFinished(-1)) {
        proc.terminate();
        return false;
    }
    return !proc.exitCode();
}

bool AndroidConfigurations::removeAVD(const QString &name) const
{
    QProcess proc;
    proc.start(androidToolPath().toString(),
               QStringList() << QLatin1String("delete") << QLatin1String("avd")
               << QLatin1String("-n") << name);
    if (!proc.waitForFinished(-1)) {
        proc.terminate();
        return false;
    }
    return !proc.exitCode();
}

QVector<AndroidDeviceInfo> AndroidConfigurations::androidVirtualDevices() const
{
    QVector<AndroidDeviceInfo> devices;
    QProcess proc;
    proc.start(androidToolPath().toString(),
               QStringList() << QLatin1String("list") << QLatin1String("avd")); // list available AVDs
    if (!proc.waitForFinished(-1)) {
        proc.terminate();
        return devices;
    }
    QList<QByteArray> avds = proc.readAll().trimmed().split('\n');
    avds.removeFirst();
    AndroidDeviceInfo dev;
    for (int i = 0; i < avds.size(); i++) {
        QString line = QLatin1String(avds[i]);
        if (!line.contains(QLatin1String("Name:")))
            continue;

        dev.serialNumber = line.mid(line.indexOf(QLatin1Char(':')) + 2).trimmed();
        ++i;
        for (; i < avds.size(); ++i) {
            line = QLatin1String(avds[i]);
            if (line.contains(QLatin1String("---------")))
                break;
            if (line.contains(QLatin1String("Target:")))
                dev.sdk = line.mid(line.lastIndexOf(QLatin1Char(' '))).remove(QLatin1Char(')')).toInt();
            if (line.contains(QLatin1String("ABI:")))
                dev.cpuABI = line.mid(line.lastIndexOf(QLatin1Char(' '))).trimmed();
        }
        devices.push_back(dev);
    }
    qSort(devices.begin(), devices.end(), androidDevicesLessThan);

    return devices;
}

QString AndroidConfigurations::startAVD(int *apiLevel, const QString &name) const
{
    QProcess *m_avdProcess = new QProcess();
    connect(this, SIGNAL(destroyed()), m_avdProcess, SLOT(deleteLater()));
    connect(m_avdProcess, SIGNAL(finished(int)), m_avdProcess, SLOT(deleteLater()));

    QString avdName = name;
    QVector<AndroidDeviceInfo> devices;
    bool createAVDOnce = false;
    while (true) {
        if (avdName.isEmpty()) {
            devices = androidVirtualDevices();
            foreach (AndroidDeviceInfo device, devices)
                if (device.sdk >= *apiLevel) { // take first emulator how supports this package
                    *apiLevel = device.sdk;
                    avdName = device.serialNumber;
                    break;
                }
        }
        // if no emulators found try to create one once
        if (avdName.isEmpty() && !createAVDOnce) {
            createAVDOnce = true;
            QMetaObject::invokeMethod(const_cast<QObject*>(static_cast<const QObject*>(this)), "createAVD", Qt::AutoConnection,
                                      Q_ARG(int, *apiLevel));
        } else {
            break;
        }
    }

    if (avdName.isEmpty())// stop here if no emulators found
        return avdName;

    // start the emulator
    m_avdProcess->start(emulatorToolPath().toString(),
                        QStringList() << QLatin1String("-partition-size") << QString::number(config().partitionSize)
                        << QLatin1String("-avd") << avdName);
    if (!m_avdProcess->waitForStarted(-1)) {
        delete m_avdProcess;
        return QString();
    }

    // wait until the emulator is online
    QProcess proc;
    proc.start(adbToolPath().toString(), QStringList() << QLatin1String("-e") << QLatin1String("wait-for-device"));
    if (!proc.waitForFinished(-1)) {
        proc.terminate();
        return QString();
    }
    sleep(5);// wait for pm to start

    // workaround for stupid adb bug
    proc.start(adbToolPath().toString(), QStringList() << QLatin1String("devices"));
    if (!proc.waitForFinished(-1)) {
        proc.terminate();
        return QString();
    }

    // get connected devices
    devices = connectedDevices(*apiLevel);
    foreach (AndroidDeviceInfo device, devices)
        if (device.sdk == *apiLevel)
            return device.serialNumber;
    // this should not happen, but ...
    return QString();
}

int AndroidConfigurations::getSDKVersion(const QString &device) const
{

    QProcess adbProc;
    adbProc.start(adbToolPath().toString(),
                  QStringList() << QLatin1String("-s") << device
                  << QLatin1String("shell") << QLatin1String("getprop")
                  << QLatin1String("ro.build.version.sdk"));
    if (!adbProc.waitForFinished(-1)) {
        adbProc.terminate();
        return -1;
    }
    return adbProc.readAll().trimmed().toInt();
}

QString AndroidConfigurations::bestMatch(const QString &targetAPI) const
{
    int target = targetAPI.mid(targetAPI.lastIndexOf(QLatin1Char('-')) + 1).toInt();
    foreach (int apiLevel, m_availablePlatforms) {
        if (apiLevel <= target)
            return QString::fromLatin1("android-%1").arg(apiLevel);
    }
    return QLatin1String("android-8");
}

AndroidConfigurations &AndroidConfigurations::instance(QObject *parent)
{
    if (m_instance == 0)
        m_instance = new AndroidConfigurations(parent);
    return *m_instance;
}

void AndroidConfigurations::save()
{
    QSettings *settings = Core::ICore::instance()->settings();
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
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    m_config = AndroidConfig(*settings);
    settings->endGroup();
}

AndroidConfigurations *AndroidConfigurations::m_instance = 0;

} // namespace Internal
} // namespace Android
