/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
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
**************************************************************************/

#include "s60devices.h"

#include <utils/environment.h>

#include <coreplugin/icore.h>

#include <QtCore/QSettings>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

namespace {
    const char * const SYMBIAN_SDKS_KEY = "HKEY_LOCAL_MACHINE\\Software\\Symbian\\EPOC SDKs";
    const char * const SYMBIAN_PATH_KEY = "CommonPath";
    const char * const SYMBIAN_DEVICES_FILE = "devices.xml";
    const char * const DEVICES_LIST = "devices";
    const char * const DEVICE = "device";
    const char * const DEVICE_ID = "id";
    const char * const DEVICE_NAME = "name";
    const char * const DEVICE_DEFAULT = "default";
    const char * const DEVICE_EPOCROOT = "epocroot";
    const char * const DEVICE_TOOLSROOT = "toolsroot";
    const char * const GNUPOC_SETTINGS_GROUP = "GnuPocSDKs";
    const char * const AUTODETECT_SETTINGS_GROUP = "SymbianSDKs";
    const char * const SDK_QT_ASSOC_SETTINGS_KEY_ROOT = "SymbianSDK";
    const char * const SETTINGS_DEFAULT_SDK_POSTFIX = ",default";
}

namespace Qt4ProjectManager {
namespace Internal {

static int findDefaultDevice(const QList<S60Devices::Device> &d)
{
    const int count = d.size();
    for (int i = 0; i < count; i++)
        if (d.at(i).isDefault)
            return i;
    return -1;
}

S60Devices::Device::Device() :
    isDefault(false)
{
}

bool S60Devices::Device::equals(const Device &rhs) const
{
    return id == rhs.id && name == rhs.name && isDefault == rhs.isDefault
           && epocRoot == rhs.epocRoot && toolsRoot == rhs.toolsRoot
           && qt == rhs.qt;
}

QString S60Devices::Device::toHtml() const
{
    QString rc;
    QTextStream str(&rc);
    str << "<html><body><table>"
            << "<tr><td><b>" << QCoreApplication::translate("Qt4ProjectManager::Internal::S60Devices::Device", "Id:")
            << "</b></td><td>" << id << "</td></tr>"
            << "<tr><td><b>" << QCoreApplication::translate("Qt4ProjectManager::Internal::S60Devices::Device", "Name:")
            << "</b></td><td>" << name << "</td></tr>"
            << "<tr><td><b>" << QCoreApplication::translate("Qt4ProjectManager::Internal::S60Devices::Device", "EPOC:")
            << "</b></td><td>" << epocRoot << "</td></tr>"
            << "<tr><td><b>" << QCoreApplication::translate("Qt4ProjectManager::Internal::S60Devices::Device", "Tools:")
            << "</b></td><td>" << toolsRoot << "</td></tr>"
            << "<tr><td><b>" << QCoreApplication::translate("Qt4ProjectManager::Internal::S60Devices::Device", "Qt:")
            << "</b></td><td>" << qt << "</td></tr>";
    return rc;
}

// ------------ S60Devices
S60Devices::S60Devices(QObject *parent) : QObject(parent)
{
}

QList<S60Devices::Device> S60Devices::devices() const
{
    return m_devices;
}

void S60Devices::setDevices(const QList<Device> &devices)
{
    if (m_devices != devices) {
        m_devices = devices;
        // Ensure a default device
        if (!m_devices.isEmpty() && findDefaultDevice(m_devices) == -1)
            m_devices.front().isDefault = true;
        writeSettings();
        emit qtVersionsChanged();
    }
}

const QList<S60Devices::Device> &S60Devices::devicesList() const
{
    return m_devices;
}

QList<S60Devices::Device> &S60Devices::devicesList()
{
    return m_devices;
}

S60Devices *S60Devices::createS60Devices(QObject *parent)
{
    S60Devices *rc = 0;
#ifdef Q_OS_WIN
    AutoDetectS60QtDevices *ad = new AutoDetectS60QtDevices(parent);
    ad->detectDevices();
    rc = ad;
#else
    rc = new GnuPocS60Devices(parent);
#endif
    rc->readSettings();
    return rc;
}

int S60Devices::findById(const QString &id) const
{
    const int count = m_devices.size();
    for (int i = 0; i < count; i++)
        if (m_devices.at(i).id == id)
            return i;
    return -1;
}

int S60Devices::findByEpocRoot(const QString &er) const
{
    const int count = m_devices.size();
    for (int i = 0; i < count; i++)
        if (m_devices.at(i).epocRoot == er)
            return i;
    return -1;
}

S60Devices::Device S60Devices::deviceForId(const QString &id) const
{
    const int index = findById(id);
    return index == -1 ? Device() : m_devices.at(index);
}

S60Devices::Device S60Devices::deviceForEpocRoot(const QString &root) const
{
    const int index = findByEpocRoot(root);
    return index == -1 ? Device() : m_devices.at(index);
}

S60Devices::Device S60Devices::defaultDevice() const
{
    const int index = findDefaultDevice(m_devices);
    return index == -1 ? Device() : m_devices.at(index);
}

S60Devices::StringStringPairList S60Devices::readSdkQtAssociationSettings(const QSettings *settings,
                                                                          const QString &group,
                                                                          int *defaultIndexPtr)
{
    StringStringPairList rc;
    // Read out numbered pairs of EpocRoot/QtDir as many as exist
    // "SymbianSDK1=/epoc,/qt[,default]".
    const QChar separator = QLatin1Char(',');
    const QString keyRoot = group + QLatin1Char('/') + QLatin1String(SDK_QT_ASSOC_SETTINGS_KEY_ROOT);
    int defaultIndex = -1;
    for (int i = 1; ; i++) {
        // Split pairs of epocroot/qtdir.
        const QVariant valueV = settings->value(keyRoot + QString::number(i));
        if (!valueV.isValid())
            break;
        // Check for default postfix
        QString value = valueV.toString();
        if (value.endsWith(QLatin1String(SETTINGS_DEFAULT_SDK_POSTFIX))) {
            value.truncate(value.size() - qstrlen(SETTINGS_DEFAULT_SDK_POSTFIX));
            defaultIndex = rc.size();
        }
        // Split into SDK and Qt
        const int separatorPos = value.indexOf(separator);
        if (separatorPos == -1)
            break;
        const QString epocRoot = value.left(separatorPos);
        const QString qtDir = value.mid(separatorPos + 1);
        rc.push_back(StringStringPair(epocRoot, qtDir));
    }
    if (defaultIndexPtr)
        *defaultIndexPtr = defaultIndex;
    return rc;
}

void S60Devices::writeSdkQtAssociationSettings(QSettings *settings, const QString &group) const
{
    // Write out as numbered pairs of EpocRoot/QtDir and indicate default
    // "SymbianSDK1=/epoc,/qt[,default]".
    settings->beginGroup(group);
    settings->remove(QString()); // remove all keys
    if (const int count = devicesList().size()) {
        const QString keyRoot = QLatin1String(SDK_QT_ASSOC_SETTINGS_KEY_ROOT);
        const QChar separator = QLatin1Char(',');
        for (int i = 0; i < count; i++) {
            const QString key = keyRoot + QString::number(i + 1);
            QString value = devicesList().at(i).epocRoot;
            value += separator;
            value += devicesList().at(i).qt;
            // Indicate default by postfix ",default"
            if (devicesList().at(i).isDefault)
                value += QLatin1String(SETTINGS_DEFAULT_SDK_POSTFIX);
            settings->setValue(key, value);
        }
    }
    settings->endGroup();
}

void S60Devices::readSettings()
{
}

void S60Devices::writeSettings()
{
}

// ------------------ S60Devices

AutoDetectS60Devices::AutoDetectS60Devices(QObject *parent) :
        S60Devices(parent)
{
}

QString AutoDetectS60Devices::errorString() const
{
    return m_errorString;
}

// as pointed to by environment.
static QStringList commonProgramFilesPaths()
{
    const QChar pathSep = QLatin1Char(';');
    QStringList rc;
    const QByteArray commonX86 = qgetenv("CommonProgramFiles(x86)");
    if (!commonX86.isEmpty())
        rc += QString::fromLocal8Bit(commonX86).split(pathSep);
    const QByteArray common = qgetenv("CommonProgramFiles");
    if (!common.isEmpty())
        rc += QString::fromLocal8Bit(common).split(pathSep);
    return rc;
}

// Find the "devices.xml" file containing the SDKs
static QString devicesXmlFile(QString *errorMessage)
{
    const QString devicesFile = QLatin1String(SYMBIAN_DEVICES_FILE);
    // Try registry
    const QSettings settings(QLatin1String(SYMBIAN_SDKS_KEY), QSettings::NativeFormat);
    const QString devicesRegistryXmlPath = settings.value(QLatin1String(SYMBIAN_PATH_KEY)).toString();
    if (!devicesRegistryXmlPath.isEmpty())
        return QDir::cleanPath(devicesRegistryXmlPath + QLatin1Char('/') + devicesFile);
    // Look up common program data files
    const QString symbianDir = QLatin1String("/symbian/");
    foreach(const QString &commonDataDir, commonProgramFilesPaths()) {
        const QFileInfo fi(commonDataDir + symbianDir + devicesFile);
        if (fi.isFile())
            return fi.absoluteFilePath();
    }
    // None found...
    *errorMessage = QString::fromLatin1("The file '%1' containing the device SDK configuration "
                                        "could not be found looking at the registry key "
                                        "%2\\%3 or the common program data directories.").
                                        arg(devicesFile, QLatin1String(SYMBIAN_SDKS_KEY),
                                            QLatin1String(SYMBIAN_PATH_KEY));
    return QString();
}

bool AutoDetectS60Devices::detectDevices()
{
    devicesList().clear();
    m_errorString.clear();
    const QString devicesXmlPath = devicesXmlFile(&m_errorString);
    if (devicesXmlPath.isEmpty())
        return false;
    QFile devicesFile(devicesXmlPath);
    if (!devicesFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        m_errorString = QString::fromLatin1("Could not open the devices file %1: %2").arg(devicesXmlPath, devicesFile.errorString());
        return false;
    }
    QXmlStreamReader xml(&devicesFile);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == DEVICES_LIST) {
            if (xml.attributes().value("version") == "1.0") {
                // Look for correct device
                while (!(xml.isEndElement() && xml.name() == DEVICES_LIST) && !xml.atEnd()) {
                    xml.readNext();
                    if (xml.isStartElement() && xml.name() == DEVICE) {
                        Device device;
                        device.id = xml.attributes().value(DEVICE_ID).toString();
                        device.name = xml.attributes().value(DEVICE_NAME).toString();
                        if (xml.attributes().value(DEVICE_DEFAULT).toString() == "yes")
                            device.isDefault = true;
                        else
                            device.isDefault = false;
                        while (!(xml.isEndElement() && xml.name() == DEVICE) && !xml.atEnd()) {
                            xml.readNext();
                            if (xml.isStartElement() && xml.name() == DEVICE_EPOCROOT) {
                                device.epocRoot = QDir::fromNativeSeparators(xml.readElementText());
                            } else if (xml.isStartElement() && xml.name() == DEVICE_TOOLSROOT) {
                                device.toolsRoot = QDir::fromNativeSeparators(xml.readElementText());
                            }
                        }
                        if (device.toolsRoot.isEmpty())
                            device.toolsRoot = device.epocRoot;
                        devicesList().append(device);
                    }
                }
            } else {
                xml.raiseError("Invalid 'devices' element version.");
            }
        }
    }
    devicesFile.close();
    if (xml.hasError()) {
        m_errorString = QString::fromLatin1("Syntax error in devices file %1: %2").
                        arg(devicesXmlPath, xml.errorString());
        return false;
    }
    return true;
}

void AutoDetectS60Devices::readSettings()
{
    // Read the associated Qt version from the settings
    // and set on the autodetected SDKs.
    bool changed = false;
    const QSettings *settings = Core::ICore::instance()->settings();
    foreach (const StringStringPair &p, readSdkQtAssociationSettings(settings, QLatin1String(GNUPOC_SETTINGS_GROUP))) {
        const int index = findByEpocRoot(p.first);
        if (index != -1 && devicesList().at(index).qt != p.second) {
            devicesList()[index].qt = p.second;
            changed = true;
        }
    }
    if (changed)
        emit qtVersionsChanged();
}

void AutoDetectS60Devices::writeSettings()
{
    writeSdkQtAssociationSettings(Core::ICore::instance()->settings(), QLatin1String(AUTODETECT_SETTINGS_GROUP));
}

// ========== AutoDetectS60QtDevices

AutoDetectS60QtDevices::AutoDetectS60QtDevices(QObject *parent) :
        AutoDetectS60Devices(parent)
{
}

// Detect a Qt version that is installed into a Symbian SDK
static QString detect_SDK_installedQt(const QString &epocRoot)
{
    const QString coreLibDllFileName = epocRoot + QLatin1String("/epoc32/release/winscw/udeb/QtCore.dll");
    QFile coreLibDllFile(coreLibDllFileName);
    if (!coreLibDllFile.exists() || !coreLibDllFile.open(QIODevice::ReadOnly))
        return QString();

    // Do not normalize these backslashes since they are in ARM binaries:
    const QByteArray indicator("\\src\\corelib\\kernel\\qobject.h");
    const int indicatorlength = indicator.size();
    const int chunkSize = 10000;

    int index = -1;
    QByteArray buffer;
    while (true) {
        buffer = coreLibDllFile.read(chunkSize);
        index = buffer.indexOf(indicator);
        if (index >= 0)
            break;
        if (buffer.size() < chunkSize || coreLibDllFile.atEnd())
            return QString();
        coreLibDllFile.seek(coreLibDllFile.pos() - indicatorlength);
    }
    coreLibDllFile.close();

    int lastIndex = index;
    while (index >= 0 && buffer.at(index))
        --index;
    if (index < 0)
        return QString();

    index += 2; // the 0 and another byte for some reason
    return QDir(QString::fromLatin1(buffer.mid(index, lastIndex-index))).absolutePath();
}

bool AutoDetectS60QtDevices::detectQtForDevices()
{
    bool changed = false;
    const int deviceCount = devicesList().size();
    for (int i = 0; i < deviceCount; ++i) {
        Device &device = devicesList()[i];
        if (device.qt.isEmpty()) {
            device.qt = detect_SDK_installedQt(device.epocRoot);
            if (device.qt.isEmpty()) {
                qWarning("Unable to detect Qt version for '%s'.", qPrintable(device.epocRoot));
            } else {
                changed = true;
            }
        }
    }
    if (changed)
        emit qtVersionsChanged();
    return true;
}

bool AutoDetectS60QtDevices::detectDevices()
{
    return AutoDetectS60Devices::detectDevices() && detectQtForDevices();
}

// ------- GnuPocS60Devices
GnuPocS60Devices::GnuPocS60Devices(QObject *parent) :
        S60Devices(parent)
{
}

S60Devices::Device GnuPocS60Devices::createDevice(const QString &epoc, const QString &qtDir)
{
    Device device;
    device.id = device.name = QLatin1String("GnuPoc");
    device.toolsRoot = device.epocRoot = epoc;
    device.qt = qtDir;
    return device;
}

// GnuPoc settings are just the pairs of EpocRoot and Qt Dir.
void GnuPocS60Devices::readSettings()
{
    // Read out numbered pairs of EpocRoot/QtDir as many as exist
    // "SymbianSDK1=/epoc,/qt".
    devicesList().clear();
    int defaultIndex = 0;
    const QSettings *settings = Core::ICore::instance()->settings();
    const StringStringPairList devices =readSdkQtAssociationSettings(settings, QLatin1String(GNUPOC_SETTINGS_GROUP), &defaultIndex);
    foreach (const StringStringPair &p, devices)
        devicesList().append(createDevice(p.first, p.second));
    // Ensure a default
    if (!devicesList().isEmpty()) {
        if (defaultIndex >= 0 && defaultIndex < devicesList().size()) {
            devicesList()[defaultIndex].isDefault = true;
        } else {
            devicesList().front().isDefault = true;
        }
    }
}

void GnuPocS60Devices::writeSettings()
{
    writeSdkQtAssociationSettings(Core::ICore::instance()->settings(), QLatin1String(GNUPOC_SETTINGS_GROUP));
}

QDebug operator<<(QDebug db, const S60Devices::Device &d)
{
    QDebug nospace = db.nospace();
    nospace << "id='" << d.id << "' name='" << d.name << "' default="
            << d.isDefault << " Epoc='" << d.epocRoot << "' tools='"
            << d.toolsRoot << "' Qt='" << d.qt << '\'';
    return db;
}

QDebug operator<<(QDebug dbg, const S60Devices &d)
{
    foreach(const S60Devices::Device &device, d.devices())
        dbg << device;
    return dbg;
}

} // namespace Internal
} // namespace Qt4ProjectManager
