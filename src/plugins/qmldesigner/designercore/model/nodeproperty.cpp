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

#include "nodeproperty.h"
#include "internalnodeproperty.h"
#include "invalidmodelnodeexception.h"
#include "invalidpropertyexception.h"
#include "invalidargumentexception.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"

namespace QmlDesigner {

NodeProperty::NodeProperty()
{
}

NodeProperty::NodeProperty(const QString &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view)
    :  NodeAbstractProperty(propertyName, internalNode, model, view)
{

}

void NodeProperty::setModelNode(const ModelNode &modelNode)
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!modelNode.isValid())
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, name());

    if (internalNode()->hasProperty(name())) { //check if oldValue != value
        Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
        if (internalProperty->isNodeProperty()
            && internalProperty->toNodeProperty()->node() == modelNode.internalNode())
            return;
    }

    if (internalNode()->hasProperty(name()) && !internalNode()->property(name())->isNodeProperty())
        model()->d->removeProperty(internalNode()->property(name()));

    model()->d->reparentNode(internalNode(), name(), modelNode.internalNode(), false); //### we have to add a flag that this is not a list
}

ModelNode NodeProperty::modelNode() const
{
     if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

     if (internalNode()->hasProperty(name())) { //check if oldValue != value
         Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
         if (internalProperty->isNodeProperty())
             return ModelNode(internalProperty->toNodeProperty()->node(), model(), view());
     }

    return ModelNode();
}

void NodeProperty::reparentHere(const ModelNode &modelNode)
{
    NodeAbstractProperty::reparentHere(modelNode, false);
}

} // namespace QmlDesigner
