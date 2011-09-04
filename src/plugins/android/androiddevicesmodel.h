/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDDEVICESMODEL_H
#define ANDROIDDEVICESMODEL_H

#include <QAbstractItemModel>

namespace Android {
namespace Internal {

class AndroidDevicesModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit AndroidDevicesModel(QObject *parent = 0);
    void update();

protected:
    virtual int	columnCount(const QModelIndex & parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual QModelIndex	index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    virtual QModelIndex	parent(const QModelIndex & index) const;
    virtual int	rowCount(const QModelIndex & parent = QModelIndex()) const;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDDEVICESMODEL_H
