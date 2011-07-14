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

#ifndef COMPONENTVIEW_H
#define COMPONENTVIEW_H

#include <abstractview.h>
#include <modelnode.h>

#include <QStringList>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
QT_END_NAMESPACE

namespace QmlDesigner {

class ComponentAction;

class ComponentView : public AbstractView
{
    Q_OBJECT

public:
    enum UserRoles
    {
        ModelNodeRole = Qt::UserRole
    };

    ComponentView(QObject *parent);

    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);

    ComponentAction *action();

    void nodeCreated(const ModelNode &createdNode);
    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange);
    void nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);
    void propertiesRemoved(const QList<AbstractProperty>& propertyList);
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange);
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);
    void instancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList);
    void instancesCompleted(const QVector<ModelNode> &completedNodeList);
    void instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &/*informationChangeHash*/);
    void instancesRenderImageChanged(const QVector<ModelNode> &nodeList);
    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList);
    void instancesChildrenChanged(const QVector<ModelNode> &nodeList);
    void instancesToken(const QString &tokenName, int tokenNumber, const QVector<ModelNode> &nodeVector);

    void nodeSourceChanged(const ModelNode &modelNode, const QString &newNodeSource);

    void rewriterBeginTransaction();
    void rewriterEndTransaction();

    void actualStateChanged(const ModelNode &node);

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                      const QList<ModelNode> &lastSelectedNodeList);

    void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl);

    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);

    void auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data);

    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);

    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports);

    QStandardItemModel *standardItemModel() const;

    ModelNode modelNode(int index) const;

    void setComponentNode(const ModelNode &node);

signals:
    void componentListChanged(const QStringList &componentList);

private: //functions
    void updateModel();
    void searchForComponentAndAddToList(const ModelNode &node);
    void searchForComponentAndRemoveFromList(const ModelNode &node);
    void appendWholeDocumentAsComponent();
    void removeSingleNodeFromList(const ModelNode &node);
    int indexForNode(const ModelNode &node);

private:
    QStandardItemModel *m_standardItemModel;
    ComponentAction *m_componentAction;
};

} // namespace QmlDesigner

#endif // COMPONENTVIEW_H
