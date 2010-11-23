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

#include "androiddeviceconfigurations.h"

#include <coreplugin/icore.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>
#include <QtCore/QStringBuilder>
#include <QtGui/QDesktopServices>

#include <algorithm>
#include <cctype>

typedef Core::SshConnectionParameters::AuthType AuthType;

namespace Qt4ProjectManager {
namespace Internal {

namespace {
    const QLatin1String SettingsGroup("AndroidDeviceConfigs");
    const QLatin1String SDKLocationKey("SDKLocation");
    const QLatin1String NDKLocationKey("SDKLocation");
    const QLatin1String NameKey("Name");
    const QLatin1String TypeKey("Type");
    const QLatin1String HostKey("Host");
    const QLatin1String SshPortKey("SshPort");
    const QLatin1String PortsSpecKey("FreePortsSpec");
    const QLatin1String UserNameKey("Uname");
    const QLatin1String AuthKey("Authentication");
    const QLatin1String KeyFileKey("KeyFile");
    const QLatin1String PasswordKey("Password");
    const QLatin1String TimeoutKey("Timeout");
    const QLatin1String InternalIdKey("InternalId");

    const QString DefaultKeyFile =
        QDesktopServices::storageLocation(QDesktopServices::HomeLocation)
        + QLatin1String("/.ssh/id_rsa");
    const int DefaultSshPortHW(22);
    const int DefaultSshPortSim(6666);
    const int DefaultGdbServerPortHW(10000);
    const int DefaultGdbServerPortSim(13219);
    const QString DefaultHostNameHW(QLatin1String("192.168.2.15"));
    const QString DefaultHostNameSim(QLatin1String("localhost"));
    const QString DefaultUserName(QLatin1String("developer"));
    const AuthType DefaultAuthType(Core::SshConnectionParameters::AuthByKey);
    const int DefaultTimeout(30);
    const AndroidConfig::DeviceType DefaultDeviceType(AndroidConfig::Physical);
};

class DevConfIdMatcher
{
public:
    DevConfIdMatcher(quint64 id) : m_id(id) {}
    bool operator()(const AndroidConfig &devConfig)
    {
        return devConfig.internalId == m_id;
    }
private:
    const quint64 m_id;
};

class PortsSpecParser
{
    struct ParseException {
        ParseException(const char *error) : error(error) {}
        const char * const error;
    };

public:
    PortsSpecParser(const QString &portsSpec)
        : m_pos(0), m_portsSpec(portsSpec) { }

    /*
     * Grammar: Spec -> [ ElemList ]
     *          ElemList -> Elem [ ',' ElemList ]
     *          Elem -> Port [ '-' Port ]
     */
    AndroidPortList parse()
    {
        try {
            if (!atEnd())
                parseElemList();
        } catch (ParseException &e) {
            qWarning("Malformed ports specification: %s", e.error);
        }
        return m_portList;
    }

private:
    void parseElemList()
    {
        if (atEnd())
            throw ParseException("Element list empty.");
        parseElem();
        if (atEnd())
            return;
        if (nextChar() != ',') {
            throw ParseException("Element followed by something else "
                "than a comma.");
        }
        ++m_pos;
        parseElemList();
    }

    void parseElem()
    {
        const int startPort = parsePort();
        if (atEnd() || nextChar() != '-') {
            m_portList.addPort(startPort);            
            return;
        }
        ++m_pos;
        const int endPort = parsePort();
        if (endPort < startPort)
            throw ParseException("Invalid range (end < start).");
        m_portList.addRange(startPort, endPort);
    }

    int parsePort()
    {
        if (atEnd())
            throw ParseException("Empty port string.");
        int port = 0;
        do {
            const char next = nextChar();
            if (!std::isdigit(next))
                break;
            port = 10*port + next - '0';
            ++m_pos;
        } while (!atEnd());
        if (port == 0 || port >= 2 << 16)
            throw ParseException("Invalid port value.");
        return port;
    }

    bool atEnd() const { return m_pos == m_portsSpec.length(); }
    char nextChar() const { return m_portsSpec.at(m_pos).toAscii(); }

    AndroidPortList m_portList;
    int m_pos;
    const QString &m_portsSpec;
};

AndroidConfig::AndroidConfig(const QString &name, AndroidConfig::DeviceType devType)
    : name(name),
      type(devType),
      portsSpec(defaultPortsSpec(type)),
      internalId(AndroidConfigurations::instance().m_nextId++)
{
}

AndroidConfig::AndroidConfig(const QSettings &settings,
                                     quint64 &nextId)
    : SDKLocation(settings.value(SDKLocationKey).toString()),
      NDKLocation(settings.value(NDKLocationKey).toString()),
      name(settings.value(NameKey).toString()),
      type(static_cast<DeviceType>(settings.value(TypeKey, DefaultDeviceType).toInt())),
      portsSpec(settings.value(PortsSpecKey, defaultPortsSpec(type)).toString()),
      internalId(settings.value(InternalIdKey, nextId).toULongLong())
{
    if (internalId == nextId)
        ++nextId;
}

AndroidConfig::AndroidConfig()
    : name(QCoreApplication::translate("AndroidDeviceConfig", "(Invalid device)")),
      internalId(InvalidId)
{
}

QString AndroidConfig::portsRegExpr()
{
    const QLatin1String portExpr("(\\d)+");
    const QString listElemExpr = QString::fromLatin1("%1(-%1)?").arg(portExpr);
    return QString::fromLatin1("((%1)(,%1)*)?").arg(listElemExpr);
}

int AndroidConfig::defaultSshPort(DeviceType type) const
{
    return type == Physical ? DefaultSshPortHW : DefaultSshPortSim;
}

QString AndroidConfig::defaultPortsSpec(DeviceType type) const
{
    return QLatin1String(type == Physical ? "10000-10100" : "13219,14168");
}

QString AndroidConfig::defaultHost(DeviceType type) const
{
    return type == Physical ? DefaultHostNameHW : DefaultHostNameSim;
}

bool AndroidConfig::isValid() const
{
    return internalId != InvalidId;
}

AndroidPortList AndroidConfig::freePorts() const
{
    return PortsSpecParser(portsSpec).parse();
}

void AndroidConfig::save(QSettings &settings) const
{
    settings.setValue(SDKLocationKey, SDKLocation);
    settings.setValue(NDKLocationKey, NDKLocation);
    settings.setValue(NameKey, name);
    settings.setValue(TypeKey, type);
    settings.setValue(InternalIdKey, internalId);
}

void AndroidConfigurations::setConfig(const AndroidConfig &devConfigs)
{
    m_config = devConfigs;
    save();
    emit updated();
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
    m_config=AndroidConfig(*settings, m_nextId);
    settings->endGroup();
}

AndroidConfigurations *AndroidConfigurations::m_instance = 0;

} // namespace Internal
} // namespace Qt4ProjectManager
