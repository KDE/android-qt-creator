/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
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

#ifndef ANDROIDDEVICECONFIGURATIONS_H
#define ANDROIDDEVICECONFIGURATIONS_H


#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

#ifdef Q_OS_LINUX
    const QLatin1String ToolchainHost("linux-x86");
#else
# ifdef Q_OS_DARWIN
    const QLatin1String ToolchainHost("darwin-x86");
# else
#  ifdef Q_OS_WIN32
    const QLatin1String ToolchainHost("windows");
#  else
#  error No Android supported OSs found
#  endif
# endif
#endif

class AndroidConfig
{
public:
    AndroidConfig();
    AndroidConfig(const QSettings &settings);
    void save(QSettings &settings) const;

    QString SDKLocation;
    QString NDKLocation;
    QString NDKToolchainVersion;
    QString AntLocation;
};

struct AndroidDevice{
    QString serialNumber;
    int sdk;
};

class AndroidConfigurations : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AndroidConfigurations)
public:

    static AndroidConfigurations &instance(QObject *parent = 0);
    AndroidConfig config() const { return m_config; }
    void setConfig(const AndroidConfig &config);
    QStringList sdkTargets();
    QStringList ndkToolchainVersions();
    QString adbToolPath(const QString & deviceSerialNumber="");
    QString androidToolPath();
    QString antToolPath();
    QString emulatorToolPath();
    QString gdbServerPath();
    QString gdbPath();
    QString stripPath();
    QString readelfPath();
    QString getDeployDeviceSerialNumber(int apiLevel=-1);
    bool createAVD();
    bool createAVD(const QString & target, const QString & name, int sdcardSize );
    bool removeAVD(const QString & name);
    QVector<AndroidDevice> connectedDevices(int apiLevel=-1);
    QVector<AndroidDevice> androidVirtualDevices();
    QString startAVD(int apiLevel, const QString & name = QString());
signals:
    void updated();

private:
    AndroidConfigurations(QObject *parent);
    void load();
    void save();

    int getSDKVersion(const QString & device);

private:
    static AndroidConfigurations *m_instance;
    AndroidConfig m_config;
    friend class AndroidConfig;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDDEVICECONFIGURATIONS_H
