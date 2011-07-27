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

#include "selectiontool.h"
#include "movetool.h"
#include "resizetool.h"
#include "anchortool.h"
#include "dragtool.h"
#include "itemcreatortool.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "formeditornodeinstanceview.h"
#include "formeditoritem.h"
#include "formeditorscene.h"
#include <rewritertransaction.h>
#include <modelnode.h>
#include <itemlibraryinfo.h>
#include <metainfo.h>
#include <model.h>
#include <QApplication>
#include <QtDebug>
#include <QPair>
#include <QString>
#include <QDir>
#include <QAction>
#include <QTimer>
#include <zoomaction.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <commondefines.h>


namespace QmlDesigner {

FormEditorView::FormEditorView(QObject *parent)
    : QmlModelView(parent),
      m_formEditorWidget(new FormEditorWidget(this)),
      m_scene(new FormEditorScene(m_formEditorWidget.data(), this)),
      m_moveTool(new MoveTool(this)),
      m_selectionTool(new SelectionTool(this)),
      m_resizeTool(new ResizeTool(this)),
      m_anchorTool(new AnchorTool(this)),
      m_dragTool(new DragTool(this)),
      m_itemCreatorTool(new ItemCreatorTool(this)),
      m_currentTool(m_selectionTool),
      m_transactionCounter(0)
{
    connect(widget()->zoomAction(), SIGNAL(zoomLevelChanged(double)), SLOT(updateGraphicsIndicators()));
    connect(widget()->showBoundingRectAction(), SIGNAL(toggled(bool)), scene(), SLOT(setShowBoundingRects(bool)));
    connect(widget()->selectOnlyContentItemsAction(), SIGNAL(toggled(bool)), this, SLOT(setSelectOnlyContentItemsAction(bool)));

}

FormEditorScene* FormEditorView::scene() const
{
    return m_scene.data();
}

FormEditorView::~FormEditorView()
{
    delete m_selectionTool;
    m_selectionTool = 0;
    delete m_moveTool;
    m_moveTool = 0;
    delete m_resizeTool;
    m_resizeTool = 0;
    delete m_anchorTool;
    m_anchorTool = 0;
    delete m_dragTool;
    m_dragTool = 0;


    // delete scene after tools to prevent double deletion
    // of items
    delete m_scene.data();
    delete m_formEditorWidget.data();
}

void FormEditorView::modelAttached(Model *model)
{
    Q_ASSERT(model);

    QmlModelView::modelAttached(model);

    Q_ASSERT(m_scene->formLayerItem());

    if (rootQmlObjectNode().toQmlItemNode().isValid())
        setupFormEditorItemTree(rootQmlObjectNode().toQmlItemNode());

    m_formEditorWidget->updateActions();
}


//This method does the setup of the initial FormEditorItem tree in the scene
void FormEditorView::setupFormEditorItemTree(const QmlItemNode &qmlItemNode)
{
    m_scene->addFormEditorItem(qmlItemNode);

    foreach (const QmlObjectNode &nextNode, qmlItemNode.allDirectSubNodes()) //TODO instance children
        //If the node has source for components/custom parsers we ignore it.
        if (QmlItemNode(nextNode).isValid() && nextNode.modelNode().nodeSourceType() == ModelNode::NodeWithoutSource)
            setupFormEditorItemTree(nextNode.toQmlItemNode());
}

void FormEditorView::removeNodeFromScene(const QmlItemNode &qmlItemNode)
{
    if (qmlItemNode.isValid()) {
        QList<QmlItemNode> nodeList;
        nodeList.append(qmlItemNode.allSubModelNodes());
        nodeList.append(qmlItemNode);

        QList<FormEditorItem*> removedItemList;
        removedItemList.append(scene()->itemsForQmlItemNodes(nodeList));
        m_currentTool->itemsAboutToRemoved(removedItemList);

        qDeleteAll(removedItemList);
    }
}

void FormEditorView::hideNodeFromScene(const QmlItemNode &qmlItemNode)
{
    if (qmlItemNode.isValid()) {

        FormEditorItem *item = m_scene->itemForQmlItemNode(qmlItemNode);

        QList<QmlItemNode> nodeList;
        nodeList.append(qmlItemNode.allSubModelNodes());
        nodeList.append(qmlItemNode);

        QList<FormEditorItem*> removedItemList;
        removedItemList.append(scene()->itemsForQmlItemNodes(nodeList));
        m_currentTool->itemsAboutToRemoved(removedItemList);
        item->setFormEditorVisible(false);
    }
}

void FormEditorView::nodeCreated(const ModelNode &createdNode)
{
    QmlModelView::nodeCreated(createdNode);
    ModelNode node(createdNode);
    //If the node has source for components/custom parsers we ignore it.
    if (QmlItemNode(node).isValid() && node.nodeSourceType() == ModelNode::NodeWithoutSource) //only setup QmlItems
        setupFormEditorItemTree(QmlItemNode(node));
}

void FormEditorView::modelAboutToBeDetached(Model *model)
{
    m_selectionTool->clear();
    m_moveTool->clear();
    m_resizeTool->clear();
    m_anchorTool->clear();
    m_dragTool->clear();
    m_scene->clearFormEditorItems();
    m_formEditorWidget->updateActions();
    m_formEditorWidget->resetView();
    scene()->resetScene();

    QmlModelView::modelAboutToBeDetached(model);
}

void FormEditorView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
    reset();
}

void FormEditorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    QmlItemNode qmlItemNode(removedNode);

    removeNodeFromScene(qmlItemNode);

    QmlModelView::nodeAboutToBeRemoved(removedNode);
}

 void FormEditorView::rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion)
 {
     foreach (FormEditorItem *item, m_scene->allFormEditorItems()) {
         item->setParentItem(0);
         item->setParent(0);
     }

     foreach (FormEditorItem *item, m_scene->allFormEditorItems()) {
         m_scene->removeItemFromHash(item);
         delete item;
     }

     QmlModelView::rootNodeTypeChanged(type, majorVersion, minorVersion);

     QmlItemNode newItemNode(rootModelNode());
     if (newItemNode.isValid()) //only setup QmlItems
         setupFormEditorItemTree(newItemNode);

     m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
 }

void FormEditorView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{
    foreach(const AbstractProperty &property, propertyList) {
        if (property.isNodeAbstractProperty()) {
            NodeAbstractProperty nodeAbstractProperty = property.toNodeAbstractProperty();
            QList<FormEditorItem*> removedItemList;

            foreach(const ModelNode &modelNode, nodeAbstractProperty.allSubNodes()) {
                QmlItemNode qmlItemNode(modelNode);

                if (qmlItemNode.isValid() && m_scene->hasItemForQmlItemNode(qmlItemNode)) {
                    FormEditorItem *item = m_scene->itemForQmlItemNode(qmlItemNode);
                    removedItemList.append(item);

                    delete item;
                }
            }

            m_currentTool->itemsAboutToRemoved(removedItemList);
        }
    }

    QmlModelView::propertiesAboutToBeRemoved(propertyList);
}

static inline bool hasNodeSourceParent(const ModelNode &node)
{
    if (node.parentProperty().isValid() && node.parentProperty().parentModelNode().isValid()) {
        ModelNode parent = node.parentProperty().parentModelNode();
        if (parent.nodeSourceType() != ModelNode::NodeWithoutSource)
            return true;
        return hasNodeSourceParent(parent);
    }
    return false;
}

void FormEditorView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
{
    if (hasNodeSourceParent(node))
        hideNodeFromScene(node);

    QmlModelView::nodeReparented(node, newPropertyParent, oldPropertyParent, propertyChange);
}

void FormEditorView::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    QmlModelView::variantPropertiesChanged(propertyList, propertyChange);
}

void FormEditorView::bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    QmlModelView::bindingPropertiesChanged(propertyList, propertyChange);
}

FormEditorWidget *FormEditorView::widget() const
{
    Q_ASSERT(!m_formEditorWidget.isNull());
    return m_formEditorWidget.data();
}

void FormEditorView::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId)
{
    QmlModelView::nodeIdChanged(node, newId, oldId);
}

void FormEditorView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                          const QList<ModelNode> &lastSelectedNodeList)
{
    QmlModelView::selectedNodesChanged(selectedNodeList, lastSelectedNodeList);

    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedNodeList)));
    if (scene()->paintMode() == FormEditorScene::AnchorMode) {
        foreach (FormEditorItem *item, m_scene->itemsForQmlItemNodes(toQmlItemNodeList(selectedNodeList)))
            item->update();

        foreach (FormEditorItem *item, m_scene->itemsForQmlItemNodes(toQmlItemNodeList(lastSelectedNodeList)))
            item->update();
    }

    if (selectedNodeList.count() != 1)
        m_formEditorWidget->setFeedbackNode(QmlItemNode());

    m_scene->update();
}

void FormEditorView::scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList)
{
    QmlModelView::scriptFunctionsChanged(node, scriptFunctionList);
}

void FormEditorView::propertiesRemoved(const QList<AbstractProperty> &/*propertyList*/)
{
}

AbstractFormEditorTool* FormEditorView::currentTool() const
{
    return m_currentTool;
}

bool FormEditorView::changeToMoveTool()
{
    if (m_currentTool == m_moveTool)
        return true;

    if (!isMoveToolAvailable())
        return false;

    scene()->setPaintMode(FormEditorScene::NormalMode);
    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = m_moveTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
    return true;
}

void FormEditorView::changeToDragTool()
{
    if (m_currentTool == m_dragTool)
        return;

    scene()->setPaintMode(FormEditorScene::NormalMode);
    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = m_dragTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
}


bool FormEditorView::changeToMoveTool(const QPointF &beginPoint)
{
    if (m_currentTool == m_moveTool)
        return true;

    if (!isMoveToolAvailable())
        return false;

    scene()->setPaintMode(FormEditorScene::NormalMode);
    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = m_moveTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
    m_moveTool->beginWithPoint(beginPoint);
    return true;
}

void FormEditorView::changeToSelectionTool()
{
    if (m_currentTool == m_selectionTool)
        return;

    scene()->setPaintMode(FormEditorScene::NormalMode);
    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = m_selectionTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
}

void FormEditorView::changeToItemCreatorTool()
{
    if(m_currentTool == m_itemCreatorTool)
        return;

    scene()->setPaintMode(FormEditorScene::NormalMode);
    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = m_itemCreatorTool;
    m_currentTool->clear();
    setSelectedQmlItemNodes(QList<QmlItemNode>());
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
}

void FormEditorView::changeToSelectionTool(QGraphicsSceneMouseEvent *event)
{
    if (m_currentTool == m_selectionTool)
        return;

    scene()->setPaintMode(FormEditorScene::NormalMode);
    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = m_selectionTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));

    m_selectionTool->selectUnderPoint(event);
}

void FormEditorView::changeToResizeTool()
{
    if (m_currentTool == m_resizeTool)
        return;

    scene()->setPaintMode(FormEditorScene::NormalMode);
    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = m_resizeTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
}

void FormEditorView::changeToAnchorTool()
{
    if (m_currentTool == m_anchorTool)
        return;

    scene()->setPaintMode(FormEditorScene::AnchorMode);
    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = m_anchorTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
}

void FormEditorView::changeToTransformTools()
{
    if (m_currentTool == m_moveTool ||
       m_currentTool == m_resizeTool ||
       m_currentTool == m_selectionTool)
        return;

    changeToSelectionTool();
}

void FormEditorView::nodeSlidedToIndex(const NodeListProperty &listProperty, int /*newIndex*/, int /*oldIndex*/)
{
    QList<ModelNode> newOrderModelNodeList = listProperty.toModelNodeList();
    foreach(const ModelNode &node, newOrderModelNodeList) {
        FormEditorItem *item = m_scene->itemForQmlItemNode(QmlItemNode(node));
        if (item) {
            FormEditorItem *oldParentItem = item->parentItem();
            item->setParentItem(0);
            item->setParentItem(oldParentItem);
        }
    }

    m_currentTool->formEditorItemsChanged(scene()->allFormEditorItems());
}

void FormEditorView::auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data)
{
    QmlModelView::auxiliaryDataChanged(node, name, data);
    if (name == "invisible" && m_scene->hasItemForQmlItemNode(QmlItemNode(node))) {
        FormEditorItem *item(m_scene->itemForQmlItemNode(QmlItemNode(node)));
        bool isInvisible = data.toBool();
        if (item->isFormEditorVisible())
            item->setVisible(!isInvisible);
        ModelNode newNode(node);
        if (isInvisible)
            newNode.deselectNode();
    }
}

void FormEditorView::instancesCompleted(const QVector<ModelNode> &completedNodeList)
{
    QList<FormEditorItem*> itemNodeList;
    foreach (const ModelNode &node, completedNodeList) {
        QmlItemNode qmlItemNode(node);
        if (qmlItemNode.isValid() && scene()->hasItemForQmlItemNode(qmlItemNode)) {
            scene()->synchronizeParent(qmlItemNode);
            itemNodeList.append(scene()->itemForQmlItemNode(qmlItemNode));
        }
    }
    currentTool()->instancesCompleted(itemNodeList);
}

void FormEditorView::instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    QList<FormEditorItem*> itemNodeList;

    foreach (const ModelNode &node, informationChangeHash.keys()) {
        QmlItemNode qmlItemNode(node);
        if (qmlItemNode.isValid() && scene()->hasItemForQmlItemNode(qmlItemNode)) {
            scene()->synchronizeTransformation(qmlItemNode);
            if (qmlItemNode.isRootModelNode() && informationChangeHash.values(node).contains(Size)) {
                widget()->setRootItemRect(qmlItemNode.instanceBoundingRect());
                widget()->centerScene();
            }

            itemNodeList.append(scene()->itemForQmlItemNode(qmlItemNode));
        }
    }

    m_currentTool->formEditorItemsChanged(itemNodeList);
}

void FormEditorView::instancesRenderImageChanged(const QVector<ModelNode> &nodeList)
{
    foreach (const ModelNode &node, nodeList) {
        QmlItemNode qmlItemNode(node);
        if (qmlItemNode.isValid() && scene()->hasItemForQmlItemNode(qmlItemNode)) {
           scene()->itemForQmlItemNode(qmlItemNode)->update();
        }
    }
}

void FormEditorView::instancesPreviewImageChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void FormEditorView::instancesToken(const QString &/*tokenName*/, int /*tokenNumber*/, const QVector<ModelNode> &/*nodeVector*/)
{
}

void FormEditorView::instancesChildrenChanged(const QVector<ModelNode> &nodeList)
{
    QList<FormEditorItem*> itemNodeList;

    foreach (const ModelNode &node, nodeList) {
        QmlItemNode qmlItemNode(node);
        if (qmlItemNode.isValid() && scene()->hasItemForQmlItemNode(qmlItemNode)) {
            scene()->synchronizeParent(qmlItemNode);
            itemNodeList.append(scene()->itemForQmlItemNode(qmlItemNode));
        }
    }

    m_currentTool->formEditorItemsChanged(itemNodeList);
    m_currentTool->instancesParentChanged(itemNodeList);
}

void FormEditorView::rewriterBeginTransaction()
{
    m_transactionCounter++;
    if (m_transactionCounter == 1
        && selectedModelNodes().count() == 1)
        m_formEditorWidget->setFeedbackNode(QmlItemNode(selectedModelNodes().first()));
}

void FormEditorView::rewriterEndTransaction()
{
    m_transactionCounter--;
    if (m_transactionCounter == 0)
        m_formEditorWidget->setFeedbackNode(QmlItemNode());
}

double FormEditorView::margins() const
{
    return m_formEditorWidget->margins();
}

double FormEditorView::spacing() const
{
    return m_formEditorWidget->spacing();
}

void FormEditorView::activateItemCreator(const QString &name)
{
    if (m_currentTool == m_itemCreatorTool) {
        m_itemCreatorTool->setItemString(name);
        return;
    }
    changeToItemCreatorTool();
    m_itemCreatorTool->setItemString(name);
}

void FormEditorView::deActivateItemCreator()
{
    if (m_currentTool == m_itemCreatorTool) {
        changeToSelectionTool();
        emit ItemCreatorDeActivated();
    }
}

QList<ModelNode> FormEditorView::adjustStatesForModelNodes(const QList<ModelNode> &nodeList) const
{
    QList<ModelNode> adjustedNodeList;
    foreach (const ModelNode &node, nodeList)
        adjustedNodeList.append(node);

    return adjustedNodeList;
}

QmlItemNode findRecursiveQmlItemNode(const QmlObjectNode &firstQmlObjectNode)
{
    QmlObjectNode qmlObjectNode = firstQmlObjectNode;

    while (true)  {
        QmlItemNode itemNode = qmlObjectNode.toQmlItemNode();
        if (itemNode.isValid())
            return itemNode;
        if (qmlObjectNode.hasInstanceParent())
            qmlObjectNode = qmlObjectNode.instanceParent();
        else
            break;
    }

    return QmlItemNode();
}

void FormEditorView::instancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList)
{
    typedef QPair<ModelNode, QString> NodePropertyPair;
    foreach (const NodePropertyPair &nodePropertyPair, propertyList) {
        const QmlItemNode itemNode(nodePropertyPair.first);
        const QString propertyName = nodePropertyPair.second;
        if (itemNode.isValid() && scene()->hasItemForQmlItemNode(itemNode)) {
            static QStringList skipList = QStringList() << "x" << "y" << "width" << "height";
            if (!skipList.contains(propertyName)) {
                m_scene->synchronizeOtherProperty(itemNode, propertyName);
                m_currentTool->formEditorItemsChanged(QList<FormEditorItem*>() << m_scene->itemForQmlItemNode(itemNode));
            }
        }
    }

    QmlModelView::instancePropertyChange(propertyList);
}

void FormEditorView::updateGraphicsIndicators()
{
    m_currentTool->formEditorItemsChanged(scene()->allFormEditorItems());
}


void FormEditorView::setSelectOnlyContentItemsAction(bool selectOnlyContentItems)
{
    m_selectionTool->setSelectOnlyContentItems(selectOnlyContentItems);
}

bool FormEditorView::isMoveToolAvailable() const
{
    if (selectedQmlItemNodes().count() == 1)
        return selectedQmlItemNodes().first().instanceIsMovable() &&
               !selectedQmlItemNodes().first().instanceIsInPositioner();
    return true;
}

void FormEditorView::actualStateChanged(const ModelNode &node)
{
    QmlModelView::actualStateChanged(node);

    QmlModelState newQmlModelState(node);

    m_formEditorWidget->anchorToolAction()->setEnabled(newQmlModelState.isBaseState());

    if (!newQmlModelState.isBaseState() && currentTool() == m_anchorTool) {
        changeToTransformTools();
        m_formEditorWidget->transformToolAction()->setChecked(true);
    }

//    FormEditorItem *item = m_scene->itemForQmlItemNode(fxObjectNode);
//
//    m_currentTool->formEditorItemsChanged(itemList);
}

void FormEditorView::reset()
{
   QTimer::singleShot(200, this, SLOT(delayedReset()));
}

void FormEditorView::delayedReset()
{
    m_selectionTool->clear();
    m_moveTool->clear();
    m_resizeTool->clear();
    m_anchorTool->clear();
    m_dragTool->clear();
    m_scene->clearFormEditorItems();
    if (rootQmlObjectNode().toQmlItemNode().isValid())
        setupFormEditorItemTree(rootQmlObjectNode().toQmlItemNode());
}


}

