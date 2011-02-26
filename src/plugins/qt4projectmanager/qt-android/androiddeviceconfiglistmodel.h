/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDDEVICECONFIGLISTMODEL_H
#define ANDROIDDEVICECONFIGLISTMODEL_H

#include "androidconfigurations.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QList>
#include <QtCore/QVariantMap>

namespace Qt4ProjectManager {
namespace Internal {

class AndroidDeviceConfigListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit AndroidDeviceConfigListModel(QObject *parent = 0);
    void setCurrentIndex(int index);
    AndroidConfig config() const;

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
        int role = Qt::DisplayRole) const;

signals:
    void currentChanged();

private:
    Q_SLOT void handleDeviceConfigListChange();
    void resetCurrentIndex();
    void setInvalid();
    void setupList();

    AndroidConfig m_androidConfig;
};


} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDDEVICECONFIGLISTMODEL_H
