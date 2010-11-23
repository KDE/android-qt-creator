#ifndef ANDROIDDEVICESMODEL_H
#define ANDROIDDEVICESMODEL_H

#include <QAbstractItemModel>

class AndroidDevicesModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit AndroidDevicesModel(QObject *parent = 0);
    void update();

protected:
    virtual int	columnCount( const QModelIndex & parent = QModelIndex() ) const;
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    virtual QModelIndex	index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex	parent( const QModelIndex & index ) const;
    virtual int	rowCount( const QModelIndex & parent = QModelIndex() ) const;
};

#endif // ANDROIDDEVICESMODEL_H
