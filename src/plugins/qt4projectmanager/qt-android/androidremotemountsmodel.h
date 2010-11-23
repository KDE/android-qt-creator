/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef ANDROIDREMOTEMOUNTSMODEL_H
#define ANDROIDREMOTEMOUNTSMODEL_H

#include "androidmountspecification.h"

#include <QtCore/QAbstractTableModel>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVariantMap>

namespace Qt4ProjectManager {
namespace Internal {

class AndroidRemoteMountsModel : public QAbstractTableModel
{
    Q_OBJECT
public:   
    explicit AndroidRemoteMountsModel(QObject *parent = 0);
    int mountSpecificationCount() const { return m_mountSpecs.count(); }
    int validMountSpecificationCount() const;
    AndroidMountSpecification mountSpecificationAt(int pos) const { return m_mountSpecs.at(pos); }
    bool hasValidMountSpecifications() const;
    const QList<AndroidMountSpecification> &mountSpecs() const { return m_mountSpecs; }

    void addMountSpecification(const QString &localDir);
    void removeMountSpecificationAt(int pos);
    void setLocalDir(int pos, const QString &localDir);

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

    static const int LocalDirRow = 0;
    static const int RemoteMountPointRow = 1;

private:
    virtual int columnCount(const QModelIndex& = QModelIndex()) const;
    virtual int rowCount(const QModelIndex& = QModelIndex()) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
        int role) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value,
        int role);

    QList<AndroidMountSpecification> m_mountSpecs;
};

inline int AndroidRemoteMountsModel::columnCount(const QModelIndex &) const
{
    return 2;
}

inline int AndroidRemoteMountsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : mountSpecificationCount();
}

inline QModelIndex AndroidRemoteMountsModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDREMOTEMOUNTSMODEL_H
