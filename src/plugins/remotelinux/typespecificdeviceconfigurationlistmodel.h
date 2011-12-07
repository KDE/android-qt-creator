/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#ifndef TYPESPECIFICDEVICECONFIGURATIONLISTMODEL_H
#define TYPESPECIFICDEVICECONFIGURATIONLISTMODEL_H

#include "linuxdeviceconfiguration.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QSharedPointer>

namespace RemoteLinux {
namespace Internal {

class TypeSpecificDeviceConfigurationListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit TypeSpecificDeviceConfigurationListModel(const QString &osType, QObject *parent = 0);
    ~TypeSpecificDeviceConfigurationListModel();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
        int role = Qt::DisplayRole) const;

    QSharedPointer<const LinuxDeviceConfiguration> deviceAt(int idx) const;
    QSharedPointer<const LinuxDeviceConfiguration> defaultDeviceConfig() const;
    QSharedPointer<const LinuxDeviceConfiguration> find(LinuxDeviceConfiguration::Id id) const;
    int indexForInternalId(LinuxDeviceConfiguration::Id id) const;

signals:
    void updated();

private:
    const QString m_targetOsType;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // TYPESPECIFICDEVICECONFIGURATIONLISTMODEL_H
