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

#ifndef TESTVIEW_H
#define TESTVIEW_H

#include <qmlmodelview.h>
#include <QVariant>
#include <QStringList>
#include <model.h>

class TestView : public QmlDesigner::QmlModelView
{
    Q_OBJECT
public:
    struct MethodCall {
        MethodCall(const QString &n, const QStringList &args) :
                name(n), arguments(args)
        {
        }

        QString name;
        QStringList arguments;
    };

    TestView(QmlDesigner::Model *model);

    void modelAttached(QmlDesigner::Model *model);
    void modelAboutToBeDetached(QmlDesigner::Model *model);
    void nodeCreated(const QmlDesigner::ModelNode &createdNode);
    void nodeAboutToBeRemoved(const QmlDesigner::ModelNode &removedNode);
    void nodeRemoved(const QmlDesigner::ModelNode &removedNode, const QmlDesigner::NodeAbstractProperty &parentProperty, AbstractView::PropertyChangeFlags propertyChange);
    void nodeReparented(const QmlDesigner::ModelNode &node, const QmlDesigner::NodeAbstractProperty &newPropertyParent, const QmlDesigner::NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeIdChanged(const QmlDesigner::ModelNode& node, const QString& newId, const QString& oldId);
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);

    void bindingPropertiesChanged(const QList<QmlDesigner::BindingProperty>& propertyList, PropertyChangeFlags propertyChange);
    void variantPropertiesChanged(const QList<QmlDesigner::VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    void propertiesAboutToBeRemoved(const QList<QmlDesigner::AbstractProperty> &propertyList);



    void fileUrlChanged(const QUrl &oldBaseUrl, const QUrl &newBaseUrl);
    void selectedNodesChanged(const QList<QmlDesigner::ModelNode> &selectedNodeList,
                              const QList<QmlDesigner::ModelNode> &lastSelectedNodeList);

    void nodeOrderChanged(const QmlDesigner::NodeListProperty &listProperty, const QmlDesigner::ModelNode &movedNode, int oldIndex);

    void stateChanged(const QmlDesigner::QmlModelState &newQmlModelState, const QmlDesigner::QmlModelState &oldQmlModelState);

    QList<MethodCall> &methodCalls();

    QString lastFunction() const;

    QmlDesigner::NodeInstanceView *nodeInstanceView() const;

    QmlDesigner::NodeInstance instanceForModelNode(const QmlDesigner::ModelNode &modelNode);

private:
    QList<MethodCall> m_methodCalls;
    static QString serialize(AbstractView::PropertyChangeFlags change);
};

bool operator==(TestView::MethodCall call1, TestView::MethodCall call2);
QDebug operator<<(QDebug debug, TestView::MethodCall call);

#endif // TESTVIEW_H
