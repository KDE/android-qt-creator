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

#ifndef DESIGNDOCUMENTCONTROLLERVIEW_H
#define DESIGNDOCUMENTCONTROLLERVIEW_H

#include <abstractview.h>
#include <modelmerger.h>

namespace QmlDesigner {

class DesignDocumentControllerView : public AbstractView
{
        Q_OBJECT
public:
    DesignDocumentControllerView(QObject *parent = 0)
            : AbstractView(parent), m_modelMerger(this) {}

    virtual void nodeCreated(const ModelNode &createdNode);
    virtual void nodeAboutToBeRemoved(const ModelNode &removedNode);
    virtual void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange);
    virtual void nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    virtual void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    virtual void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    virtual void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);
    virtual void propertiesRemoved(const QList<AbstractProperty>& propertyList);
    virtual void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    virtual void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange);
    virtual void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);

    virtual void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                      const QList<ModelNode> &lastSelectedNodeList);

    virtual void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);
    virtual void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);
    void instancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList);
    void instancesCompleted(const QVector<ModelNode> &completedNodeList);
    void instanceInformationsChange(const QVector<ModelNode> &nodeList);
    void instancesRenderImageChanged(const QVector<ModelNode> &nodeList);
    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList);
    void instancesChildrenChanged(const QVector<ModelNode> &nodeList);

    void rewriterBeginTransaction();
    void rewriterEndTransaction();

    void actualStateChanged(const ModelNode &node);

    ModelNode insertModel(const ModelNode &modelNode)
    { return m_modelMerger.insertModel(modelNode); }
    void replaceModel(const ModelNode &modelNode)
    { m_modelMerger.replaceModel(modelNode); }

    void toClipboard() const;
    void fromClipboard();

    QString toText() const;
    void fromText(QString text);

private:
    ModelMerger m_modelMerger;
};

}// namespace QmlDesigner

#endif // DESIGNDOCUMENTCONTROLLERVIEW_H
