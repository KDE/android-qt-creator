/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androiddevicesmodel.h"
#include "androidconfigurations.h"

using namespace Android::Internal;

AndroidDevicesModel::AndroidDevicesModel(QObject *parent) :
    QAbstractItemModel(parent)
{
}

int AndroidDevicesModel::columnCount(const QModelIndex &/*parent*/ ) const
{
    return 1;
}

QVariant AndroidDevicesModel::data(const QModelIndex &index, int role ) const
{
    if (role!= Qt::DisplayRole)
        return QVariant();
    if (index.internalId() < 0) {
        if (index.row() == 0)
            return tr("Connected Android devices");
        else
            return tr("Virtual Android Devices");
    }
    if (index.internalId() == 0) // real devices
        return QVariant();
    else
        return QVariant();

}

QModelIndex AndroidDevicesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
        return createIndex(row, column, parent.row());
    else
        return createIndex(row, column, -1);
}

QModelIndex AndroidDevicesModel::parent(const QModelIndex & _index) const
{
    if (_index.internalId() < 0)
        return QModelIndex();
    else
        return index((int)_index.internalId(), 0);
}

int AndroidDevicesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        switch (parent.row()) {
        case 0:// real devices
            return 0;
        case 1:// AVDs
            return 0;
        }
        return 0;
    } else {
        return 2;
    }
}
