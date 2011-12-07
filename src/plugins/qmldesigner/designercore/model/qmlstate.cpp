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

#include "qmlstate.h"
#include "qmlmodelview.h"
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <metainfo.h>
#include <invalidmodelnodeexception.h>
#include "bindingproperty.h"


namespace QmlDesigner {

QmlModelState::QmlModelState()
   : QmlModelNodeFacade()
{
}

QmlModelState::QmlModelState(const ModelNode &modelNode)
        : QmlModelNodeFacade(modelNode)
{
}

QmlPropertyChanges QmlModelState::propertyChanges(const ModelNode &node)
{
    //### exception if not valid

    if (isBaseState())
        return  QmlPropertyChanges();

    addChangeSetIfNotExists(node);
    foreach (const ModelNode &childNode, modelNode().nodeListProperty("changes").toModelNodeList()) {
        //### exception if not valid QmlModelStateOperation
        if (QmlPropertyChanges(childNode).target().isValid() && QmlPropertyChanges(childNode).target() == node && QmlPropertyChanges(childNode).isValid())
            return QmlPropertyChanges(childNode); //### exception if not valid(childNode);
    }
    return QmlPropertyChanges(); //not found
}

QList<QmlModelStateOperation> QmlModelState::stateOperations(const ModelNode &node) const
{
    QList<QmlModelStateOperation> returnList;
    //### exception if not valid

    if (isBaseState())
        return returnList;

    if (!modelNode().hasProperty("changes"))
        return returnList;

    Q_ASSERT(modelNode().property("changes").isNodeListProperty());

    foreach (const ModelNode &childNode, modelNode().nodeListProperty("changes").toModelNodeList()) {
        QmlModelStateOperation stateOperation(childNode);
        if (stateOperation.isValid()) {
            ModelNode targetNode = stateOperation.target();
            if (targetNode.isValid()
                && targetNode == node)
            returnList.append(stateOperation); //### exception if not valid(childNode);
        }
    }
    return returnList; //not found
}

QList<QmlPropertyChanges> QmlModelState::propertyChanges() const
{
    //### exception if not valid
    QList<QmlPropertyChanges> returnList;

    if (isBaseState())
        return returnList;

    if (!modelNode().hasProperty("changes"))
        return returnList;

    Q_ASSERT(modelNode().property("changes").isNodeListProperty());

    foreach (const ModelNode &childNode, modelNode().nodeListProperty("changes").toModelNodeList()) {
        //### exception if not valid QmlModelStateOperation
        if (QmlPropertyChanges(childNode).isValid())
            returnList.append(QmlPropertyChanges(childNode));
    }
    return returnList;
}


bool QmlModelState::hasPropertyChanges(const ModelNode &node) const
{
    //### exception if not valid

    if (isBaseState())
        return false;

    foreach(const QmlPropertyChanges &changeSet, propertyChanges()) {
        if (changeSet.target().isValid() && changeSet.target() == node)
            return true;
    }
    return false;
}

bool QmlModelState::hasStateOperation(const ModelNode &node) const
{
    //### exception if not valid

    if (isBaseState())
        return false;

    foreach(const  QmlModelStateOperation &stateOperation, stateOperations()) {
        if (stateOperation.target() == node)
            return true;
    }
    return false;
}

QList<QmlModelStateOperation> QmlModelState::stateOperations() const
{
    //### exception if not valid
    QList<QmlModelStateOperation> returnList;

    if (isBaseState())
        return returnList;

    if (!modelNode().hasProperty("changes"))
        return returnList;

    Q_ASSERT(modelNode().property("changes").isNodeListProperty());

    foreach (const ModelNode &childNode, modelNode().nodeListProperty("changes").toModelNodeList()) {
        //### exception if not valid QmlModelStateOperation
        if (QmlModelStateOperation(childNode).isValid())
            returnList.append(QmlModelStateOperation(childNode));
    }
    return returnList;
}


/*! \brief Add a ChangeSet for the specified ModelNode to this state
  The new ChangeSet if only added if no ChangeSet for the ModelNode
  does not exist, yet.
*/

void QmlModelState::addChangeSetIfNotExists(const ModelNode &node)
{
    //### exception if not valid

    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (hasPropertyChanges(node)) {
        return; //changeSet already there
    }

    ModelNode newChangeSet = modelNode().view()->createModelNode("QtQuick.PropertyChanges", 1, 0);
    modelNode().nodeListProperty("changes").reparentHere(newChangeSet);

    QmlPropertyChanges(newChangeSet).setTarget(node);
    Q_ASSERT(QmlPropertyChanges(newChangeSet).isValid());
}

void QmlModelState::removePropertyChanges(const ModelNode &node)
{
    //### exception if not valid

    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (isBaseState())
        return;

     QmlPropertyChanges theChangeSet(propertyChanges(node));
     if (theChangeSet.isValid())
         theChangeSet.modelNode().destroy();
}



/*! \brief Returns if this state affects the specified ModelNode

\return true if this state affects the specifigied ModelNode
*/
bool QmlModelState::affectsModelNode(const ModelNode &node) const
{
    if (isBaseState())
        return false;

    return !stateOperations(node).isEmpty();
}

QList<QmlObjectNode> QmlModelState::allAffectedNodes() const
{
    QList<QmlObjectNode> returnList;

    foreach (const ModelNode &childNode, modelNode().nodeListProperty("changes").toModelNodeList()) {
        //### exception if not valid QmlModelStateOperation
        if (QmlModelStateOperation(childNode).isValid() &&
            !returnList.contains(QmlModelStateOperation(childNode).target()))
            returnList.append(QmlModelStateOperation(childNode).target());
    }
    return returnList;
}

QString QmlModelState::name() const
{
    if (isBaseState())
        return QString();

    return modelNode().variantProperty("name").value().toString();
}

void QmlModelState::setName(const QString &name)
{
    if ((!isBaseState()) && (modelNode().isValid()))
        modelNode().variantProperty("name").setValue(name);
}

bool QmlModelState::isValid() const
{
    return QmlModelNodeFacade::isValid() &&
            modelNode().metaInfo().isValid() &&
            (modelNode().metaInfo().isSubclassOf("QtQuick.State", 1, 0) || isBaseState());
}

/**
  Removes state node & all subnodes.
  */
void QmlModelState::destroy()
{
    Q_ASSERT(isValid());
    modelNode().destroy();
}

/*! \brief Returns if this state is the base state

\return true if this state is the base state
*/

bool QmlModelState::isBaseState() const
{
    return !modelNode().isValid() || modelNode().isRootNode();
}

QmlModelState QmlModelState::duplicate(const QString &name) const
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    QmlItemNode parentNode(modelNode().parentProperty().parentModelNode());
    if (!parentNode.isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

//    QmlModelState newState(stateGroup().addState(name));
    PropertyListType propertyList;
    propertyList.append(qMakePair(QString("name"), QVariant(name)));
    QmlModelState newState ( qmlModelView()->createModelNode("QtQuick.State", 1, 0, propertyList) );

    foreach (const ModelNode &childNode, modelNode().nodeListProperty("changes").toModelNodeList()) {
        ModelNode newModelNode(qmlModelView()->createModelNode(childNode.type(), childNode.majorVersion(), childNode.minorVersion()));
        foreach (const BindingProperty &bindingProperty, childNode.bindingProperties())
            newModelNode.bindingProperty(bindingProperty.name()).setExpression(bindingProperty.expression());
        foreach (const VariantProperty &variantProperty, childNode.variantProperties())
            newModelNode.variantProperty(variantProperty.name()) = variantProperty.value();
        newState.modelNode().nodeListProperty("changes").reparentHere(newModelNode);
    }

    modelNode().parentProperty().reparentHere(newState);

    return newState;
}

QmlModelStateGroup QmlModelState::stateGroup() const
{
    QmlItemNode parentNode(modelNode().parentProperty().parentModelNode());
    return parentNode.states();
}

QmlModelState QmlModelState::createBaseState(const QmlModelView *view)
{
    QmlModelState fxState(view->rootModelNode());

    return fxState;
}

} // QmlDesigner
