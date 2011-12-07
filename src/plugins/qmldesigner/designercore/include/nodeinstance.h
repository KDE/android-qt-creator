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

#ifndef PROXYNODEINSTANCE_H
#define PROXYNODEINSTANCE_H

#include <QSharedPointer>
#include <QTransform>
#include <QPointF>
#include <QSizeF>
#include <QPair>

#include "commondefines.h"

namespace QmlDesigner {

class ModelNode;
class NodeInstanceView;
class ProxyNodeInstanceData;

class NodeInstance
{
    friend class NodeInstanceView;
public:
    static NodeInstance create(const ModelNode &node);
    NodeInstance();
    ~NodeInstance();
    NodeInstance(const NodeInstance &other);
    NodeInstance& operator=(const NodeInstance &other);

    ModelNode modelNode() const;
    bool isValid() const;
    void makeInvalid();
    QRectF boundingRect() const;
    bool hasContent() const;
    bool isAnchoredBySibling() const;
    bool isAnchoredByChildren() const;
    bool isMovable() const;
    bool isResizable() const;
    QTransform transform() const;
    QTransform sceneTransform() const;
    bool isInPositioner() const;
    QPointF position() const;
    QSizeF size() const;
    int penWidth() const;
    void paint(QPainter *painter);

    QVariant property(const QString &name) const;
    bool hasBindingForProperty(const QString &name) const;
    QPair<QString, qint32> anchor(const QString &name) const;
    bool hasAnchor(const QString &name) const;
    QString instanceType(const QString &name) const;

    qint32 parentId() const;
    qint32 instanceId() const;

    QPixmap renderPixmap() const;

protected:
    void setProperty(const QString &name, const QVariant &value);
    InformationName setInformation(InformationName name,
                        const QVariant &information,
                        const QVariant &secondInformation,
                        const QVariant &thirdInformation);

    InformationName setInformationSize(const QSizeF &size);
    InformationName setInformationBoundingRect(const QRectF &rectangle);
    InformationName setInformationTransform(const QTransform &transform);
    InformationName setInformationPenWith(int penWidth);
    InformationName setInformationPosition(const QPointF &position);
    InformationName setInformationIsInPositioner(bool isInPositioner);
    InformationName setInformationSceneTransform(const QTransform &sceneTransform);
    InformationName setInformationIsResizable(bool isResizable);
    InformationName setInformationIsMovable(bool isMovable);
    InformationName setInformationIsAnchoredByChildren(bool isAnchoredByChildren);
    InformationName setInformationIsAnchoredBySibling(bool isAnchoredBySibling);
    InformationName setInformationHasContent(bool hasContent);
    InformationName setInformationHasAnchor(const QString &sourceAnchorLine, bool hasAnchor);
    InformationName setInformationAnchor(const QString &sourceAnchorLine, const QString &targetAnchorLine, qint32 targetInstanceId);
    InformationName setInformationInstanceTypeForProperty(const QString &property, const QString &type);
    InformationName setInformationHasBindingForProperty(const QString &property, bool hasProperty);

    void setParentId(qint32 instanceId);
    void setRenderPixmap(const QImage &image);
    NodeInstance(ProxyNodeInstanceData *d);

private:
    QSharedPointer<ProxyNodeInstanceData> d;
};

bool operator ==(const NodeInstance &first, const NodeInstance &second);

}

#endif // PROXYNODEINSTANCE_H
