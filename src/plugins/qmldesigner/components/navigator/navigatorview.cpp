/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "navigatorview.h"
#include "navigatortreemodel.h"
#include "navigatorwidget.h"

#include <nodeproperty.h>
#include <nodelistproperty.h>
#include <QHeaderView>


namespace QmlDesigner {

NavigatorView::NavigatorView(QObject* parent) :
        AbstractView(parent),
        m_blockSelectionChangedSignal(false),
        m_widget(new NavigatorWidget),
        m_treeModel(new NavigatorTreeModel(this))
{
    m_widget->setTreeModel(m_treeModel.data());

    connect(treeWidget()->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(changeSelection(QItemSelection,QItemSelection)));
    treeWidget()->setIndentation(treeWidget()->indentation() * 0.5);

    NameItemDelegate *idDelegate = new NameItemDelegate(this,m_treeModel.data());
    IconCheckboxItemDelegate *showDelegate = new IconCheckboxItemDelegate(this,":/qmldesigner/images/eye_open.png",
                                                          ":/qmldesigner/images/placeholder.png",m_treeModel.data());

#ifdef _LOCK_ITEMS_
    IconCheckboxItemDelegate *lockDelegate = new IconCheckboxItemDelegate(this,":/qmldesigner/images/lock.png",
                                                          ":/qmldesigner/images/hole.png",m_treeModel.data());
#endif


    treeWidget()->setItemDelegateForColumn(0,idDelegate);
#ifdef _LOCK_ITEMS_
    treeWidget()->setItemDelegateForColumn(1,lockDelegate);
    treeWidget()->setItemDelegateForColumn(2,showDelegate);
#else
    treeWidget()->setItemDelegateForColumn(1,showDelegate);
#endif

}

NavigatorView::~NavigatorView()
{
    if (m_widget && !m_widget->parent())
        delete m_widget.data();
}

QWidget *NavigatorView::widget()
{
    return m_widget.data();
}

void NavigatorView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_treeModel->setView(this);

    QTreeView *treeView = treeWidget();
    treeView->expandAll();

    treeView->header()->setResizeMode(0, QHeaderView::Stretch);
    treeView->header()->resizeSection(1,26);
    treeView->setRootIsDecorated(false);
    treeView->setIndentation(20);
#ifdef _LOCK_ITEMS_
    treeView->header()->resizeSection(2,20);
#endif
}

void NavigatorView::modelAboutToBeDetached(Model *model)
{
    m_treeModel->clearView();
    AbstractView::modelAboutToBeDetached(model);
}

void NavigatorView::importAdded(const Import &)
{
    treeWidget()->update();
}

void NavigatorView::importRemoved(const Import &)
{
    treeWidget()->update();
}


void NavigatorView::nodeCreated(const ModelNode & /*createdNode*/)
{
}

void NavigatorView::nodeRemoved(const ModelNode & /*removedNode*/, const NodeAbstractProperty & /*parentProperty*/, PropertyChangeFlags /*propertyChange*/)
{
}

void NavigatorView::propertiesRemoved(const QList<AbstractProperty> & /*propertyList*/)
{
}

void NavigatorView::variantPropertiesChanged(const QList<VariantProperty> & /*propertyList*/, PropertyChangeFlags /*propertyChange*/)
{
}

void NavigatorView::bindingPropertiesChanged(const QList<BindingProperty> & /*propertyList*/, PropertyChangeFlags /*propertyChange*/)
{
}

void NavigatorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (m_treeModel->isInTree(removedNode))
        m_treeModel->removeSubTree(removedNode);
}

void NavigatorView::nodeAboutToBeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
}

void NavigatorView::nodeReparented(const ModelNode &node, const NodeAbstractProperty & /*newPropertyParent*/, const NodeAbstractProperty & /*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    bool blocked = blockSelectionChangedSignal(true);

    if (m_treeModel->isInTree(node))
        m_treeModel->removeSubTree(node);
    if (node.isInHierarchy())
        m_treeModel->addSubTree(node);

    // make sure selection is in sync again
    updateItemSelection();

    blockSelectionChangedSignal(blocked);
}

void NavigatorView::nodeIdChanged(const ModelNode& node, const QString & /*newId*/, const QString & /*oldId*/)
{
    if (m_treeModel->isInTree(node))
        m_treeModel->updateItemRow(node);
}

void NavigatorView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{
    foreach (const AbstractProperty &property, propertyList) {
        if (property.isNodeProperty()) {
            NodeProperty nodeProperty(property.toNodeProperty());
            m_treeModel->removeSubTree(nodeProperty.modelNode());
        } else if (property.isNodeListProperty()) {
            NodeListProperty nodeListProperty(property.toNodeListProperty());
            foreach (const ModelNode &node, nodeListProperty.toModelNodeList()) {
                m_treeModel->removeSubTree(node);
            }
        }
    }
}

void NavigatorView::rootNodeTypeChanged(const QString & /*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    if (m_treeModel->isInTree(rootModelNode()))
        m_treeModel->updateItemRow(rootModelNode());
}

void NavigatorView::auxiliaryDataChanged(const ModelNode &node, const QString & /*name*/, const QVariant & /*data*/)
{
    if (m_treeModel->isInTree(node))
    {
        // update model
        m_treeModel->updateItemRow(node);

        // repaint row (id and icon)
        QModelIndex index = m_treeModel->indexForNode(node);
        treeWidget()->update( index );
        treeWidget()->update( index.sibling(index.row(),index.column()+1) );
    }
}

void NavigatorView::scriptFunctionsChanged(const ModelNode &/*node*/, const QStringList &/*scriptFunctionList*/)
{
}

void NavigatorView::instancePropertyChange(const QList<QPair<ModelNode, QString> > &/*propertyList*/)
{
}

void NavigatorView::instancesCompleted(const QVector<ModelNode> &/*completedNodeList*/)
{
}

void NavigatorView::nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &node, int oldIndex)
{
    if (m_treeModel->isInTree(node))
        m_treeModel->updateItemRowOrder(listProperty, node, oldIndex);
}

void NavigatorView::changeSelection(const QItemSelection & /*newSelection*/, const QItemSelection &/*deselected*/)
{
    if (m_blockSelectionChangedSignal)
        return;
    QSet<ModelNode> nodeSet;
    foreach (const QModelIndex &index, treeWidget()->selectionModel()->selectedIndexes()) {
        if (m_treeModel->data(index, Qt::UserRole).isValid())
            nodeSet.insert(m_treeModel->nodeForIndex(index));
    }

    bool blocked = blockSelectionChangedSignal(true);
    setSelectedModelNodes(nodeSet.toList());
    blockSelectionChangedSignal(blocked);
}

void NavigatorView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/, const QList<ModelNode> &/*lastSelectedNodeList*/)
{
    updateItemSelection();
}

void NavigatorView::updateItemSelection()
{
    QItemSelection itemSelection;
    foreach (const ModelNode &node, selectedModelNodes()) {
        const QModelIndex index = m_treeModel->indexForNode(node);
        if (index.isValid()) {
            const QModelIndex beginIndex(m_treeModel->index(index.row(), 0, index.parent()));
            const QModelIndex endIndex(m_treeModel->index(index.row(), m_treeModel->columnCount(index.parent()) - 1, index.parent()));
            if (beginIndex.isValid() && endIndex.isValid())
                itemSelection.select(beginIndex, endIndex);
        }
    }

    bool blocked = blockSelectionChangedSignal(true);
    treeWidget()->selectionModel()->select(itemSelection, QItemSelectionModel::ClearAndSelect);
    blockSelectionChangedSignal(blocked);

    // make sure selected nodes a visible
    foreach(const QModelIndex &selectedIndex, itemSelection.indexes()) {
        if (selectedIndex.column() == 0)
            expandRecursively(selectedIndex);
    }
}

QTreeView *NavigatorView::treeWidget()
{
    if (m_widget)
        return m_widget->treeView();
    return 0;
}

NavigatorTreeModel *NavigatorView::treeModel()
{
    return m_treeModel.data();
}

// along the lines of QObject::blockSignals
bool NavigatorView::blockSelectionChangedSignal(bool block)
{
    bool oldValue = m_blockSelectionChangedSignal;
    m_blockSelectionChangedSignal = block;
    return oldValue;
}

void NavigatorView::expandRecursively(const QModelIndex &index)
{
    QModelIndex currentIndex = index;
    while (currentIndex.isValid()) {
        if (!treeWidget()->isExpanded(currentIndex))
            treeWidget()->expand(currentIndex);
        currentIndex = currentIndex.parent();
    }
}

} // namespace QmlDesigner
