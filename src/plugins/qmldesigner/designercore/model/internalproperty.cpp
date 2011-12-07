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

#include "internalproperty.h"
#include "internalbindingproperty.h"
#include "internalvariantproperty.h"
#include "internalnodelistproperty.h"
#include "internalnodeproperty.h"
#include "internalnode_p.h"
#include <QVariant>
#include <QString>
#include <QRegExp>
#include <QSize>
#include <QSizeF>
#include <QPoint>
#include <QPointF>

namespace QmlDesigner {

namespace Internal {

// Creates invalid InternalProperty
InternalProperty::InternalProperty()
{
}

InternalProperty::~InternalProperty()
{
}

InternalProperty::InternalProperty(const QString &name, const InternalNode::Pointer &propertyOwner)
     : m_name(name),
     m_propertyOwner(propertyOwner)
{
    Q_ASSERT_X(!name.isEmpty(), Q_FUNC_INFO, "Name of property cannot be empty");
}

InternalProperty::Pointer InternalProperty::internalPointer() const
{
    Q_ASSERT(!m_internalPointer.isNull());
    return m_internalPointer.toStrongRef();
}

void InternalProperty::setInternalWeakPointer(const Pointer &pointer)
{
    Q_ASSERT(!pointer.isNull());
    m_internalPointer = pointer;
}


bool InternalProperty::isValid() const
{
    return m_propertyOwner && !m_name.isEmpty();
}

QString InternalProperty::name() const
{
    return m_name;
}

bool InternalProperty::isBindingProperty() const
{
    return false;
}

bool InternalProperty::isVariantProperty() const
{
    return false;
}

QSharedPointer<InternalBindingProperty> InternalProperty::toBindingProperty() const
{
    Q_ASSERT(internalPointer().dynamicCast<InternalBindingProperty>());
    return internalPointer().staticCast<InternalBindingProperty>();
}


bool InternalProperty::isNodeListProperty() const
{
     return false;
}

bool InternalProperty::isNodeProperty() const
{
    return false;
}

bool InternalProperty::isNodeAbstractProperty() const
{
    return false;
}

QSharedPointer<InternalVariantProperty> InternalProperty::toVariantProperty() const

{
    Q_ASSERT(internalPointer().dynamicCast<InternalVariantProperty>());
    return internalPointer().staticCast<InternalVariantProperty>();
}

InternalNode::Pointer InternalProperty::propertyOwner() const
{
    return m_propertyOwner.toStrongRef();
}

QSharedPointer<InternalNodeListProperty> InternalProperty::toNodeListProperty() const
{
    Q_ASSERT(internalPointer().dynamicCast<InternalNodeListProperty>());
    return internalPointer().staticCast<InternalNodeListProperty>();
}

QSharedPointer<InternalNodeProperty> InternalProperty::toNodeProperty() const
{
    Q_ASSERT(internalPointer().dynamicCast<InternalNodeProperty>());
    return internalPointer().staticCast<InternalNodeProperty>();
}

QSharedPointer<InternalNodeAbstractProperty> InternalProperty::toNodeAbstractProperty() const
{
    Q_ASSERT(internalPointer().dynamicCast<InternalNodeAbstractProperty>());
    return internalPointer().staticCast<InternalNodeAbstractProperty>();
}

void InternalProperty::remove()
{
    propertyOwner()->removeProperty(name());
    m_propertyOwner.clear();
}

QString InternalProperty::dynamicTypeName() const
{
    return m_dynamicType;
}

void InternalProperty::setDynamicTypeName(const QString &name)
{
    m_dynamicType = name;
}


void InternalProperty::resetDynamicTypeName()
{
   m_dynamicType.clear();
}


} //namespace Internal
} //namespace QmlDesigner

