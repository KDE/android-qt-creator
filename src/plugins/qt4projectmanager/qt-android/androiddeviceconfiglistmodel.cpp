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

#include "androiddeviceconfiglistmodel.h"

#include "androidconstants.h"

namespace Qt4ProjectManager {
namespace Internal {

AndroidDeviceConfigListModel::AndroidDeviceConfigListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    setupList();
    const AndroidConfigurations &devConfs
        = AndroidConfigurations::instance();
    connect(&devConfs, SIGNAL(updated()), this,
        SLOT(handleDeviceConfigListChange()));
}

void AndroidDeviceConfigListModel::setupList()
{
//    const AndroidConfigurations &devConfs
//        = AndroidConfigurations::instance();
//    foreach (const AndroidConfig &devConfig, devConfs.devConfigs()) {
//        if (devConfig.freePorts().hasMore())
//            m_devConfigs << devConfig;
//    }
}

void AndroidDeviceConfigListModel::setCurrentIndex(int index)
{
//    m_currentIndex = index;
//    m_currentId = m_devConfigs.at(m_currentIndex).internalId;
    emit currentChanged();
}

void AndroidDeviceConfigListModel::resetCurrentIndex()
{
//    if (m_devConfigs.isEmpty()) {
//        setInvalid();
//        return;
//    }

//    for (int i = 0; i < m_devConfigs.count(); ++i) {
//        if (m_devConfigs.at(i).internalId == m_currentId) {
//            setCurrentIndex(i);
//            return;
//        }
//    }
//    setCurrentIndex(0);
}

void AndroidDeviceConfigListModel::setInvalid()
{
//    m_currentIndex = -1;
//    m_currentId = AndroidConfig::InvalidId;
    emit currentChanged();
}

AndroidConfig AndroidDeviceConfigListModel::config() const
{
    return m_androidConfig;
}

QVariantMap AndroidDeviceConfigListModel::toMap() const
{
    QVariantMap map;
//    map.insert(AndroidDeviceIdKey, config().internalId);
    return map;
}

void AndroidDeviceConfigListModel::fromMap(const QVariantMap &map)
{
//    const quint64 oldId = m_currentId;
//    m_currentId = map.value(AndroidDeviceIdKey, 0).toULongLong();
//    resetCurrentIndex();
//    if (oldId != m_currentId)
//        emit currentChanged();
}

void AndroidDeviceConfigListModel::handleDeviceConfigListChange()
{
    setupList();
    resetCurrentIndex();
    reset();
    emit currentChanged();
}

int AndroidDeviceConfigListModel::rowCount(const QModelIndex &parent) const
{
//    return parent.isValid() ? 0 : m_devConfigs.count();
}

QVariant AndroidDeviceConfigListModel::data(const QModelIndex &index, int role) const
{
//    if (!index.isValid() || index.row() >= rowCount()
//        || role != Qt::DisplayRole)
//        return QVariant();
//    return m_devConfigs.at(index.row()).name;
}

} // namespace Internal
} // namespace Qt4ProjectManager
