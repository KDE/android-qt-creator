/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

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
    return 0;
//    return parent.isValid() ? 0 : m_devConfigs.count();
}

QVariant AndroidDeviceConfigListModel::data(const QModelIndex &index, int role) const
{
    return QVariant();
//    if (!index.isValid() || index.row() >= rowCount()
//        || role != Qt::DisplayRole)
//        return QVariant();
//    return m_devConfigs.at(index.row()).name;
}

} // namespace Internal
} // namespace Qt4ProjectManager
