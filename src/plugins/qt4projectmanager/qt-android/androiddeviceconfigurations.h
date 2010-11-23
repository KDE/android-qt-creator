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

#include <coreplugin/ssh/sshconnection.h>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class AndroidPortList
{
    typedef QPair<int, int> Range;
public:
    void addPort(int port) { addRange(port, port); }
    void addRange(int startPort, int endPort) {
        m_ranges << Range(startPort, endPort);
    }
    bool hasMore() const { return !m_ranges.isEmpty(); }
    int count() const {
        int n = 0;
        foreach (const Range &r, m_ranges)
            n += r.second - r.first + 1;
        return n;
    }
    int getNext() {
        Q_ASSERT(!m_ranges.isEmpty());
        Range &firstRange = m_ranges.first();
        const int next = firstRange.first++;
        if (firstRange.first > firstRange.second)
            m_ranges.removeFirst();
        return next;
    }
    QString toString() const
    {
        QString stringRep;
        foreach (const Range &range, m_ranges) {
            stringRep += QString::number(range.first);
            if (range.second != range.first)
                stringRep += QLatin1Char('-') + QString::number(range.second);
            stringRep += QLatin1Char(',');
        }
        if (!stringRep.isEmpty())
            stringRep.remove(stringRep.length() - 1, 1); // Trailing comma.
        return stringRep;
    }

private:
    QList<Range> m_ranges;
};

class AndroidConfig
{
public:
    enum DeviceType { Physical, Simulator };
    AndroidConfig();
    AndroidConfig(const QString &name, DeviceType type);
    AndroidConfig(const QSettings &settings, quint64 &nextId);
    void save(QSettings &settings) const;
    bool isValid() const;
    AndroidPortList freePorts() const;
    static QString portsRegExpr();

    static const quint64 InvalidId = 0;

    QString SDKLocation;
    QString NDKLocation;

    QString name;
    DeviceType type;
    QString portsSpec;
    quint64 internalId;

private:
    int defaultSshPort(DeviceType type) const;
    QString defaultPortsSpec(DeviceType type) const;
    QString defaultHost(DeviceType type) const;

};

class AndroidDevConfNameMatcher
{
public:
    AndroidDevConfNameMatcher(const QString &name) : m_name(name) {}
    bool operator()(const AndroidConfig &devConfig)
    {
        return devConfig.name == m_name;
    }
private:
    const QString m_name;
};


class AndroidConfigurations : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AndroidConfigurations)
public:

    static AndroidConfigurations &instance(QObject *parent = 0);
    AndroidConfig devConfigs() const { return m_config; }
    void setConfig(const AndroidConfig &devConfigs);

signals:
    void updated();

private:
    AndroidConfigurations(QObject *parent);
    void load();
    void save();

    static AndroidConfigurations *m_instance;
    AndroidConfig m_config;
    quint64 m_nextId;
    friend class AndroidConfig;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDDEVICECONFIGURATIONS_H
