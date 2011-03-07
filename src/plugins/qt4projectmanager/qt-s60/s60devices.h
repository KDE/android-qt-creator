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

#ifndef S60DEVICES_H
#define S60DEVICES_H

#include <projectexplorer/toolchain.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QPair>

QT_BEGIN_NAMESPACE
class QDebug;
class QSettings;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

// List of S60 devices.
class S60Devices : public QObject
{
    Q_OBJECT
public:
    struct Device {
        Device();
        bool equals(const Device &rhs) const;

        QString toHtml() const;

        QString id;
        QString name;
        bool isDefault;
        QString epocRoot;
        QString toolsRoot;
        QString qt;
    };

    // Construct a devices object, does autodetection if applicable
    // and restores settings.
    static S60Devices *createS60Devices(QObject *parent);

    QList<Device> devices() const;
    // Set devices, write settings and emit changed signals accordingly.
    void setDevices(const QList<Device> &device);

    Device deviceForId(const QString &id) const;
    Device deviceForEpocRoot(const QString &root) const;
    Device defaultDevice() const;

    int findByEpocRoot(const QString &er) const;

signals:
    void qtVersionsChanged();

protected:
    typedef QPair<QString, QString> StringStringPair;
    typedef QList<StringStringPair> StringStringPairList;

    explicit S60Devices(QObject *parent = 0);

    const QList<Device> &devicesList() const;
    QList<Device> &devicesList();

    int findById(const QString &id) const;

    // Helpers to serialize the association of Symbian SDK<->Qt
    // to QSettings (pair of SDK/Qt).
    static StringStringPairList readSdkQtAssociationSettings(const QSettings *settings,
                                                             const QString &group,
                                                             int *defaultIndex = 0);
    void writeSdkQtAssociationSettings(QSettings *settings, const QString &group) const;

private:
    virtual void readSettings(); // empty stubs
    virtual void writeSettings();

    QList<Device> m_devices;
};

inline bool operator==(const S60Devices::Device &d1, const S60Devices::Device &d2)
{ return d1.equals(d2); }
inline bool operator!=(const S60Devices::Device &d1, const S60Devices::Device &d2)
{ return !d1.equals(d2); }

// Autodetected Symbian Devices (as parsed from devices.xml file on Windows)
// with a manually set version of Qt. Currently not used, but might be by
// makefile-based builds on Windows.
class AutoDetectS60Devices : public S60Devices
{
    Q_OBJECT
public:
    explicit AutoDetectS60Devices(QObject *parent = 0);
    virtual bool detectDevices();
    QString errorString() const;

private:
    // Write and restore Qt-SDK associations.
    virtual void readSettings();
    virtual void writeSettings();

    QString m_errorString;
};

// Autodetected Symbian-Qt-Devices (with Qt installed
// into the SDK) for ABLD, Raptor. Completely autodetected.
class AutoDetectS60QtDevices : public AutoDetectS60Devices
{
    Q_OBJECT
public:
    explicit AutoDetectS60QtDevices(QObject *parent = 0);
    // Overwritten to detect associated Qt versions in addition.
    virtual bool detectDevices();

private:
    // No settings as it is completely autodetected.
    virtual void readSettings()  {}
    virtual void writeSettings() {}

    bool detectQtForDevices();
};

// Manually configured Symbian Devices completely based on QSettings.
class GnuPocS60Devices : public S60Devices
{
    Q_OBJECT
public:
    explicit GnuPocS60Devices(QObject *parent = 0);

    static Device createDevice(const QString &epoc, const QString &qtDir);

private:
    virtual void readSettings();
    virtual void writeSettings();
};

QDebug operator<<(QDebug dbg, const S60Devices::Device &d);
QDebug operator<<(QDebug dbg, const S60Devices &d);

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60DEVICES_H
