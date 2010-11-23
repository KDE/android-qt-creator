#include "androiddevicesmodel.h"
#include "androiddeviceconfigurations.h"

AndroidDevicesModel::AndroidDevicesModel(QObject *parent) :
    QAbstractItemModel(parent)
{
}

int AndroidDevicesModel::columnCount( const QModelIndex & parent ) const
{
    return 1;
}

QVariant AndroidDevicesModel::data( const QModelIndex & index, int role ) const
{

}

QModelIndex AndroidDevicesModel::index( int row, int column, const QModelIndex & parent ) const
{

}

QModelIndex AndroidDevicesModel::parent( const QModelIndex & index ) const
{

}

int AndroidDevicesModel::rowCount( const QModelIndex & parent ) const
{

}
