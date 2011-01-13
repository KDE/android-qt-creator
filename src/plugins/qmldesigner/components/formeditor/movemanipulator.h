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

#ifndef MOVEMANIPULATOR_H
#define MOVEMANIPULATOR_H

#include <QWeakPointer>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QHash>
#include <QPointF>
#include <QRectF>

#include "controlelement.h"
#include "formeditoritem.h"
#include "rewritertransaction.h"
#include "snapper.h"
#include "formeditorview.h"

namespace QmlDesigner {

class ModelNodeChangeSet;
class Model;

class MoveManipulator
{
public:
    enum Snapping {
        UseSnapping,
        UseSnappingAndAnchoring,
        NoSnapping
    };

    enum State {
        UseActualState,
        UseBaseState
    };

    MoveManipulator(LayerItem *layerItem, FormEditorView *view);
    ~MoveManipulator();
    void setItems(const QList<FormEditorItem*> &itemList);
    void setItem(FormEditorItem* item);

    void begin(const QPointF& beginPoint);
    void update(const QPointF& updatePoint, Snapping useSnapping, State stateToBeManipulated = UseActualState);
    void reparentTo(FormEditorItem *newParent);
    void end(const QPointF& endPoint);

    void moveBy(double deltaX, double deltaY);

    void beginRewriterTransaction();
    void endRewriterTransaction();

    QPointF beginPoint() const;

    void clear();

    bool isActive() const;

protected:
    void setOpacityForAllElements(qreal opacity);

    QPointF findSnappingOffset(const QHash<FormEditorItem*, QRectF> &boundingRectHash);

    void deleteSnapLines();

    QHash<FormEditorItem*, QRectF> tanslatedBoundingRects(const QHash<FormEditorItem*, QRectF> &boundingRectHash, const QPointF& offset);
    QPointF calculateBoundingRectMovementOffset(const QPointF& updatePoint);
    QRectF boundingRect(FormEditorItem* item, const QPointF& updatePoint);

    void generateSnappingLines(const QHash<FormEditorItem*, QRectF> &boundingRectHash);
    void updateHashes();

    bool itemsCanReparented() const;

    void setPosition(QmlItemNode itemNode, const QPointF &position);

private:
    Snapper m_snapper;
    QWeakPointer<LayerItem> m_layerItem;
    QWeakPointer<FormEditorView> m_view;
    QList<FormEditorItem*> m_itemList;
    QHash<FormEditorItem*, QRectF> m_beginItemRectHash;
    QHash<FormEditorItem*, QPointF> m_beginPositionHash;
    QHash<FormEditorItem*, QPointF> m_beginPositionInSceneSpaceHash;
    QPointF m_beginPoint;
    QHash<FormEditorItem*, double> m_beginTopMarginHash;
    QHash<FormEditorItem*, double> m_beginLeftMarginHash;
    QHash<FormEditorItem*, double> m_beginRightMarginHash;
    QHash<FormEditorItem*, double> m_beginBottomMarginHash;
    QHash<FormEditorItem*, double> m_beginHorizontalCenterHash;
    QHash<FormEditorItem*, double> m_beginVerticalCenterHash;
    QList<QGraphicsItem*> m_graphicsLineList;
    bool m_isActive;
    RewriterTransaction m_rewriterTransaction;
};

}
#endif // MOVEMANIPULATOR_H
