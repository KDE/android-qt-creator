/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef VIEWLOGGER_H
#define VIEWLOGGER_H

#include "abstractview.h"

#include <QTextStream>
#include <QTime>

namespace QmlDesigner {
namespace Internal {

class ViewLogger : public QmlDesigner::AbstractView
{
    Q_OBJECT
public:
    ViewLogger(QObject *parent);

    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);

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

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                      const QList<ModelNode> &lastSelectedNodeList);

    void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl);

    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);

    void importsChanged();

    void auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data);

    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);
    void instancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList);
    void instancesCompleted(const QVector<ModelNode> &completedNodeList);
protected:
    QString time() const;

private:
    QTextStream m_output;
    QTime m_timer;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // VIEWLOGGER_H
