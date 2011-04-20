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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLGRAPHICSITEMNODEINSTANCE_H
#define QMLGRAPHICSITEMNODEINSTANCE_H

#include "graphicsobjectnodeinstance.h"
#include <QDeclarativeItem>
#include <QWeakPointer>


namespace QmlDesigner {
namespace Internal {

class QmlGraphicsItemNodeInstance : public GraphicsObjectNodeInstance
{
public:
    typedef QSharedPointer<QmlGraphicsItemNodeInstance> Pointer;
    typedef QWeakPointer<QmlGraphicsItemNodeInstance> WeakPointer;

    ~QmlGraphicsItemNodeInstance();

    static Pointer create(QObject *objectToBeWrapped);

    bool isQmlGraphicsItem() const;

    QSizeF size() const;
    QRectF boundingRect() const;
//    void updateAnchors();

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


    QList<ServerNodeInstance> stateInstances() const;

protected:
    QmlGraphicsItemNodeInstance(QDeclarativeItem *item);
    QDeclarativeItem *qmlGraphicsItem() const;
    QDeclarativeAnchors *anchors() const;
    void resetHorizontal();
    void resetVertical(); 
    void refresh();

private: //variables
    bool m_hasHeight;
    bool m_hasWidth;
    bool m_isResizable;
    double m_x;
    double m_y;
    double m_width;
    double m_height;
};

}
}
#endif // QMLGRAPHICSITEMNODEINSTANCE_H
