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

#ifndef SGITEMNODEINSTANCE_H
#define SGITEMNODEINSTANCE_H

#include <QtGlobal>

#if QT_VERSION >= 0x050000

#include "objectnodeinstance.h"

#include <QSGItem>
#include <designersupportfunctions.h>

namespace QmlDesigner {
namespace Internal {

class SGItemNodeInstance : public ObjectNodeInstance
{
public:
    typedef QSharedPointer<SGItemNodeInstance> Pointer;
    typedef QWeakPointer<SGItemNodeInstance> WeakPointer;

    ~SGItemNodeInstance();

    static Pointer create(QObject *objectToBeWrapped);
    void initialize(const ObjectNodeInstance::Pointer &objectNodeInstance);

    bool isSGItem() const;

    QRectF boundingRect() const;
    QPointF position() const;
    QSizeF size() const;
    QTransform transform() const;
    QTransform customTransform() const;
    QTransform sceneTransform() const;
    double opacity() const;

    QObject *parent() const;

    double rotation() const;
    double scale() const;
    QPointF transformOriginPoint() const;
    double zValue() const;

    bool equalSGItem(QSGItem *item) const;

    bool hasContent() const;

    QList<ServerNodeInstance> childItems() const;
    QList<ServerNodeInstance> childItemsForChild(QSGItem *childItem) const;

    bool isMovable() const;
    void setMovable(bool movable);

    void setPropertyVariant(const QString &name, const QVariant &value);
    void setPropertyBinding(const QString &name, const QString &expression);

    QVariant property(const QString &name) const;
    void resetProperty(const QString &name);

    void reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const QString &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const QString &newParentProperty);

    int penWidth() const;

    bool hasAnchor(const QString &name) const;
    QPair<QString, ServerNodeInstance> anchor(const QString &name) const;
    bool isAnchoredBySibling() const;
    bool isAnchoredByChildren() const;
    void doComponentComplete();

    bool isResizable() const;
    void setResizable(bool resizeable);

    void setHasContent(bool hasContent);

    QList<ServerNodeInstance> stateInstances() const;

    QImage renderImage() const;

    DesignerSupport *designerSupport() const;
    Qt5NodeInstanceServer *qt5NodeInstanceServer() const;

protected:
    SGItemNodeInstance(QSGItem*);
    QSGItem *sgItem() const;
    void resetHorizontal();
    void resetVertical();
    void refresh();
    QRectF boundingRectWithStepChilds(QSGItem *parentItem) const;

private: //variables
    bool m_hasHeight;
    bool m_hasWidth;
    bool m_isResizable;
    bool m_hasContent;
    bool m_isMovable;
    double m_x;
    double m_y;
    double m_width;
    double m_height;
};

} // namespace Internal
} // namespace QmlDesigner

#endif  // QT_VERSION
#endif  // SGITEMNODEINSTANCE_H

