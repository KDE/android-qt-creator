/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "reparentcontainer.h"

namespace QmlDesigner {

ReparentContainer::ReparentContainer()
    : m_instanceId(-1),
    m_oldParentInstanceId(-1),
    m_newParentInstanceId(-1)
{
}

ReparentContainer::ReparentContainer(qint32 instanceId,
                  qint32 oldParentInstanceId,
                  const QString &oldParentProperty,
                  qint32 newParentInstanceId,
                  const QString &newParentProperty)
    : m_instanceId(instanceId),
    m_oldParentInstanceId(oldParentInstanceId),
    m_oldParentProperty(oldParentProperty),
    m_newParentInstanceId(newParentInstanceId),
    m_newParentProperty(newParentProperty)
{
}

qint32 ReparentContainer::instanceId() const
{
    return m_instanceId;
}

qint32 ReparentContainer::oldParentInstanceId() const
{
    return m_oldParentInstanceId;
}

QString ReparentContainer::oldParentProperty() const
{
    return m_oldParentProperty;
}

qint32 ReparentContainer::newParentInstanceId() const
{
    return m_newParentInstanceId;
}

QString ReparentContainer::newParentProperty() const
{
    return m_newParentProperty;
}

QDataStream &operator<<(QDataStream &out, const ReparentContainer &container)
{
    out << container.instanceId();
    out << container.oldParentInstanceId();
    out << container.oldParentProperty();
    out << container.newParentInstanceId();
    out << container.newParentProperty();

    return out;
}

QDataStream &operator>>(QDataStream &in, ReparentContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_oldParentInstanceId;
    in >> container.m_oldParentProperty;
    in >> container.m_newParentInstanceId;
    in >> container.m_newParentProperty;

    return in;
}

} // namespace QmlDesigner
