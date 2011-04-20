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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "nodelistproperty.h"
#include "internalproperty.h"
#include "internalnodelistproperty.h"
#include "invalidmodelnodeexception.h"
#include "invalidpropertyexception.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"
#include <qmlobjectnode.h>



namespace QmlDesigner {

NodeListProperty::NodeListProperty()
{}

NodeListProperty::NodeListProperty(const NodeListProperty &property, AbstractView *view)
    : NodeAbstractProperty(property.name(), property.internalNode(), property.model(), view)
{

}

NodeListProperty::NodeListProperty(const QString &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view) :
        NodeAbstractProperty(propertyName, internalNode, model, view)
{
}

NodeListProperty::NodeListProperty(const Internal::InternalNodeListProperty::Pointer &internalNodeListProperty, Model* model, AbstractView *view)
    : NodeAbstractProperty(internalNodeListProperty, model, view)
{
}

static QList<ModelNode> internalNodesToModelNodes(const QList<Internal::InternalNode::Pointer> &inputList, Model* model, AbstractView *view)
{
    QList<ModelNode> modelNodeList;
    foreach (const Internal::InternalNode::Pointer &internalNode, inputList) {
        modelNodeList.append(ModelNode(internalNode, model, view));
    }
    return modelNodeList;
}

const QList<ModelNode> NodeListProperty::toModelNodeList() const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "<invalid node list property>");

    if (internalNode()->hasProperty(name())) {
        Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
        if (internalProperty->isNodeListProperty()) {
            return internalNodesToModelNodes(internalProperty->toNodeListProperty()->nodeList(), model(), view());
        }
    }

    return QList<ModelNode>();
}

const QList<QmlObjectNode> NodeListProperty::toQmlObjectNodeList() const
{
    QmlModelView *fxView = view()->toQmlModelView();

    if (fxView == 0)
        return QList<QmlObjectNode>();

    QList<QmlObjectNode> fxObjectNodeList;

    foreach (const ModelNode &modelNode, toModelNodeList())
        fxObjectNodeList.append(QmlObjectNode(modelNode));

    return fxObjectNodeList;
}

void NodeListProperty::slide(int from, int to) const
{
    Internal::WriteLocker locker(model());
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "<invalid node list property>");
    if (to > toModelNodeList().count() - 1)
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "<invalid node list sliding>");

     model()->m_d->changeNodeOrder(internalNode(), name(), from, to);
}

void NodeListProperty::reparentHere(const ModelNode &modelNode)
{
    NodeAbstractProperty::reparentHere(modelNode, true);
}

ModelNode NodeListProperty::at(int index) const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "<invalid node list property>");

    Internal::InternalNodeListProperty::Pointer internalProperty = internalNode()->nodeListProperty(name());
    if (internalProperty)
        return ModelNode(internalProperty->at(index), model(), view());


    return ModelNode();
}

} // namespace QmlDesigner
