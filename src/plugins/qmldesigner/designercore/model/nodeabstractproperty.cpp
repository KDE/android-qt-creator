/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "nodeabstractproperty.h"
#include "nodeproperty.h"
#include "internalproperty.h"
#include "internalnodelistproperty.h"
#include "invalidmodelnodeexception.h"
#include "invalidpropertyexception.h"
#include "invalidreparentingexception.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"

namespace QmlDesigner {

NodeAbstractProperty::NodeAbstractProperty()
{
}

NodeAbstractProperty::NodeAbstractProperty(const NodeAbstractProperty &property, AbstractView *view)
    : AbstractProperty(property.name(), property.internalNode(), property.model(), view)
{
}

NodeAbstractProperty::NodeAbstractProperty(const QString &propertyName, const Internal::InternalNodePointer &internalNode, Model *model, AbstractView *view)
    : AbstractProperty(propertyName, internalNode, model, view)
{
}

NodeAbstractProperty::NodeAbstractProperty(const Internal::InternalNodeAbstractProperty::Pointer &property, Model *model, AbstractView *view)
    : AbstractProperty(property, model, view)
{}

void NodeAbstractProperty::reparentHere(const ModelNode &modelNode)
{
    if (internalNode()->hasProperty(name()) && !internalNode()->property(name())->isNodeAbstractProperty())
        reparentHere(modelNode, isNodeListProperty());
    else
        reparentHere(modelNode, parentModelNode().metaInfo().propertyIsListProperty(name()) || isDefaultProperty()); //we could use the metasystem instead?
}

void NodeAbstractProperty::reparentHere(const ModelNode &modelNode,  bool isNodeList)
{
    if (modelNode.parentProperty() == *this)
        return;
    Internal::WriteLocker locker(model());
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (isNodeProperty()) {
        NodeProperty nodeProperty(toNodeProperty());
        if (nodeProperty.modelNode().isValid())
            throw InvalidReparentingException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (modelNode.isAncestorOf(parentModelNode()))
        throw InvalidReparentingException(__LINE__, __FUNCTION__, __FILE__);

    if (internalNode()->hasProperty(name()) && !internalNode()->property(name())->isNodeAbstractProperty())
        model()->m_d->removeProperty(internalNode()->property(name()));

    if (modelNode.hasParentProperty()) {
        Internal::InternalNodeAbstractProperty::Pointer oldParentProperty = modelNode.internalNode()->parentProperty();

        model()->m_d->reparentNode(internalNode(), name(), modelNode.internalNode(), isNodeList);

        Q_ASSERT(!oldParentProperty.isNull());


    } else {
        model()->m_d->reparentNode(internalNode(), name(), modelNode.internalNode(), isNodeList);
    }
}

bool NodeAbstractProperty::isEmpty() const
{
    Internal::InternalNodeAbstractProperty::Pointer property = internalNode()->nodeAbstractProperty(name());
    if (property.isNull())
        return true;
    else
        return property->isEmpty();
}

int NodeAbstractProperty::indexOf(const ModelNode &node) const
{
    Internal::InternalNodeAbstractProperty::Pointer property = internalNode()->nodeAbstractProperty(name());
    if (property.isNull())
        return 0;

    return property->indexOf(node.internalNode());
}

int NodeAbstractProperty::count() const
{
    Internal::InternalNodeAbstractProperty::Pointer property = internalNode()->nodeAbstractProperty(name());
    if (property.isNull())
        return 0;
    else
        return property->count();
}

QList<ModelNode> NodeAbstractProperty::allSubNodes()
{
    if (!internalNode()
        || !internalNode()->isValid()
        || !internalNode()->hasProperty(name())
        || !internalNode()->property(name())->isNodeAbstractProperty())
        return QList<ModelNode>();

    Internal::InternalNodeAbstractProperty::Pointer property = internalNode()->nodeAbstractProperty(name());
    return toModelNodeList(property->allSubNodes(), view());
}

/*!
  \brief Returns if the the two property handles reference the same property in the same node
*/
bool operator ==(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2)
{
    return AbstractProperty(property1) == AbstractProperty(property2);
}

/*!
  \brief Returns if the the two property handles do not reference the same property in the same node
  */
bool operator !=(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2)
{
    return !(property1 == property2);
}

uint qHash(const NodeAbstractProperty &property)
{
    return qHash(AbstractProperty(property));
}

QDebug operator<<(QDebug debug, const NodeAbstractProperty &property)
{
    return debug.nospace() << "NodeAbstractProperty(" << (property.isValid() ? property.name() : QLatin1String("invalid")) << ')';
}

QTextStream& operator<<(QTextStream &stream, const NodeAbstractProperty &property)
{
    stream << "NodeAbstractProperty(" << property.name() << ')';

    return stream;
}
} // namespace QmlDesigner
