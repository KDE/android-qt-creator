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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "androidconfigurations.h"
#include "androidconstants.h"

#include <coreplugin/icore.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>
#include <QtCore/QStringBuilder>
#include <QtGui/QDesktopServices>
#include <QtCore/QStringList>
#include <QtCore/QProcess>
#include <QDebug>

namespace Qt4ProjectManager {
namespace Internal {

namespace {
    const QLatin1String SettingsGroup("AndroidDeviceConfigs");
    const QLatin1String SDKLocationKey("SDKLocation");
    const QLatin1String NDKLocationKey("NDKLocation");
    const QLatin1String NDKToolchainVersionKey("NDKToolchainVersion");
    const QLatin1String AntLocationKey("AntLocation");
    bool androidDevicesLessThan(const AndroidDevice & dev1, const AndroidDevice & dev2)
    {
        return dev1.sdk< dev2.sdk;
    }
}


AndroidConfig::AndroidConfig(const QSettings &settings)
    : SDKLocation(settings.value(SDKLocationKey).toString()),
      NDKLocation(settings.value(NDKLocationKey).toString()),
      NDKToolchainVersion(settings.value(NDKToolchainVersionKey).toString()),
      AntLocation(settings.value(AntLocationKey).toString())
{
}

AndroidConfig::AndroidConfig()
{
}

void AndroidConfig::save(QSettings &settings) const
{
    settings.setValue(SDKLocationKey, SDKLocation);
    settings.setValue(NDKLocationKey, NDKLocation);
    settings.setValue(NDKToolchainVersionKey, NDKToolchainVersion);
    settings.setValue(AntLocationKey, AntLocation);
}

void AndroidConfigurations::setConfig(const AndroidConfig &devConfigs)
{
    m_config = devConfigs;
    save();
    emit updated();
}

QStringList AndroidConfigurations::sdkTargets()
{
#warning TODO run android list targets and take targets fron the output
    return QStringList()<<"android-4"<<"android-5"<<"android-8"<<"android-9";
}

QStringList AndroidConfigurations::ndkToolchainVersions()
{
#warning TODO list the content of NDK_path/toolchains and get only the folders which contain "prebuilt" folder
    return QStringList()<<"arm-linux-androideabi-4.4.3"<<"arm-eabi-4.4.0";
}

QString AndroidConfigurations::adbToolPath(const QString & deviceSerialNumber)
{
    QString adbCmmand=m_config.SDKLocation+QLatin1String("/platform-tools/adb"ANDROID_EXEC_SUFFIX);
    if (deviceSerialNumber.length())
        adbCmmand+=" -s "+deviceSerialNumber;
    return adbCmmand;
}

QString AndroidConfigurations::androidToolPath()
{
    return m_config.SDKLocation+QLatin1String("/tools/android"ANDROID_EXEC_SUFFIX);
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
    return m_config.SDKLocation+QLatin1String("/tools/emulator"ANDROID_EXEC_SUFFIX);
}

QString AndroidConfigurations::stripPath()
{
    return m_config.NDKLocation+QString("/toolchains/%1/prebuilt/%2/bin/%3-strip"ANDROID_EXEC_SUFFIX).arg(m_config.NDKToolchainVersion).arg(ToolchainHost).arg(m_config.NDKToolchainVersion.left(m_config.NDKToolchainVersion.lastIndexOf('-')));
}

QString AndroidConfigurations::gdbServerPath()
{
    return m_config.NDKLocation+QString("/toolchains/%1/prebuilt/gdbserver").arg(m_config.NDKToolchainVersion);
}

QString AndroidConfigurations::gdbPath()
{
    return m_config.NDKLocation+QString("/toolchains/%1/prebuilt/%2/bin/%3-gdb"ANDROID_EXEC_SUFFIX).arg(m_config.NDKToolchainVersion).arg(ToolchainHost).arg(m_config.NDKToolchainVersion.left(m_config.NDKToolchainVersion.lastIndexOf('-')));
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
    adbProc.start(QString("%1 devices").arg(adbToolPath()));
    if (!adbProc.waitForFinished(-1))
        return devices;
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

QString AndroidConfigurations::createAVD(int apiLevel)
{
#warning TODO ANDROID use "android create avd" command
    return QString();
}

QString AndroidConfigurations::startAVD(int apiLevel)
{
    QProcess * m_avdProcess = new QProcess(this);
    connect(m_avdProcess, SIGNAL(finished(int)), m_avdProcess, SLOT(deleteLater()));

    QVector<AndroidDevice> devices;
    QProcess proc;
    proc.start(QString("%1 list avd").arg(androidToolPath())); // list avaialbe AVDs
    if (!proc.waitForFinished(-1))
        return QString();
    QList<QByteArray> avds=proc.readAll().trimmed().split('\n');
    avds.removeFirst();
    AndroidDevice dev;
    for (int i=0;i<avds.size();i++)
    {
        QString line=avds[i];
        if (!line.contains("Name:"))
            continue;

        dev.serialNumber=line.mid(line.indexOf(':')+2).trimmed();// device.left(device.indexOf('\t')).trimmed();
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
        if (dev.sdk>=apiLevel)
            devices.push_back(dev);
    }
    qSort(devices.begin(), devices.end(), androidDevicesLessThan);

    QString avdName;
    foreach(AndroidDevice device, devices)
        if (device.sdk>=apiLevel) // take first emulator how supports this package
        {
            avdName=device.serialNumber;
            break;
        }

    // if no emulators found try to create one
    if (!avdName.length())
        avdName=createAVD(apiLevel);

    if (!avdName.length())// stop here if no emulators found
        return avdName;

    // start the emulator
    m_avdProcess->start(emulatorToolPath()+QLatin1String(" -avd ")+avdName);
    if (!m_avdProcess->waitForStarted(-1))
        return QString();

    // wait until the emulator is online
    proc.start(adbToolPath()+ QLatin1String(" -e wait-for-device"));
    if (!proc.waitForFinished(-1))
        return QString();

    sleep(5);// wait for pm to start

    // get connected devices
    devices =connectedDevices(apiLevel);
    foreach(AndroidDevice device, devices)
        if (device.sdk==apiLevel)
            return device.serialNumber;
    // this should not happen, but ...
    return QString();
}

int AndroidConfigurations::getSDKVersion(const QString & device)
{
    QProcess adbProc;
    adbProc.start(QString("%1 shell getprop ro.build.version.sdk").arg(adbToolPath(device)));
    if (!adbProc.waitForFinished(-1))
        return -1;
    return adbProc.readAll().trimmed().toInt();
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
}

void AndroidConfigurations::load()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    m_config=AndroidConfig(*settings);
    settings->endGroup();
}

AndroidConfigurations *AndroidConfigurations::m_instance = 0;

} // namespace Internal
} // namespace Qt4ProjectManager
