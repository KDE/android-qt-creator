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

#ifndef QmlItemNode_H
#define QmlItemNode_H

#include <corelib_global.h>
#include <modelnode.h>
#include "qmlobjectnode.h"
#include "qmlstate.h"

#include <QStringList>
#include <QRectF>
#include <QTransform>

namespace QmlDesigner {

class QmlModelStateGroup;
class QmlAnchors;

class CORESHARED_EXPORT QmlItemNode : public QmlObjectNode
{
    friend class CORESHARED_EXPORT QmlAnchors;
public:
    QmlItemNode() : QmlObjectNode() {}
    QmlItemNode(const ModelNode &modelNode)  : QmlObjectNode(modelNode) {}
    bool isValid() const;
    bool isRootNode() const;

    QmlModelStateGroup states() const;
    QList<QmlItemNode> children() const;
    QList<QmlObjectNode> resources() const;
    QList<QmlObjectNode> defaultPropertyChildren() const;
    QList<QmlObjectNode> allDirectSubNodes() const;
    QmlAnchors anchors() const;

    bool hasChildren() const;
    bool hasResources() const;
    bool instanceHasAnchors() const;
    bool hasShowContent() const;

    bool canReparent() const;
    bool instanceIsAnchoredBySibling() const;
    bool instanceIsAnchoredByChildren() const;
    bool instanceIsMovable() const;
    bool instanceIsResizable() const;
    bool instanceIsInPositioner() const;
    bool instanceHasRotationTransform() const;

    QRectF instanceBoundingRect() const;
    QRectF instancePaintedBoundingRect() const;
    QTransform instanceTransform() const;
    QTransform instanceSceneTransform() const;
    QPointF instancePosition() const;
    QSizeF instanceSize() const;
    int instancePenWidth() const;


    void paintInstance(QPainter *painter);

    void selectNode();
    void deselectNode();
    bool isSelected() const;

    QString simplifiedTypeName() const;

    const QList<QmlItemNode> allDirectSubModelNodes() const;
    const QList<QmlItemNode> allSubModelNodes() const;
    bool hasAnySubModelNodes() const;
};

CORESHARED_EXPORT uint qHash(const QmlItemNode &node);

class CORESHARED_EXPORT QmlModelStateGroup
{
    friend class QmlItemNode;
    friend class QmlModelView;

public:

    QmlModelStateGroup() : m_modelNode(ModelNode()) {}

    ModelNode modelNode() const { return m_modelNode; }
    QStringList names() const;
    QList<QmlModelState> allStates() const;
    QmlModelState state(const QString &name) const;
    QmlModelState addState(const QString &name);
    void removeState(const QString &name);

protected:
    QmlModelStateGroup(const ModelNode &modelNode) : m_modelNode(modelNode) {}

private:
    ModelNode m_modelNode;
};

CORESHARED_EXPORT QList<ModelNode> toModelNodeList(const QList<QmlItemNode> &fxItemNodeList);
CORESHARED_EXPORT QList<QmlItemNode> toQmlItemNodeList(const QList<ModelNode> &modelNodeList);

} //QmlDesigner


#endif // QmlItemNode_H
