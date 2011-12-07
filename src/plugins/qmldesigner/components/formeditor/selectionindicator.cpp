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

#include "selectionindicator.h"

#include <QPen>
#include <cmath>
#include <QGraphicsScene>
#include <formeditorview.h>
#include <formeditorwidget.h>
#include <zoomaction.h>

namespace QmlDesigner {

SelectionIndicator::SelectionIndicator(LayerItem *layerItem)
    : m_layerItem(layerItem)
{
}

SelectionIndicator::~SelectionIndicator()
{
    clear();
}

void SelectionIndicator::show()
{
    foreach (QGraphicsPolygonItem *item, m_indicatorShapeHash.values())
        item->show();
}

void SelectionIndicator::hide()
{
    foreach (QGraphicsPolygonItem *item, m_indicatorShapeHash.values())
        item->hide();
}

void SelectionIndicator::clear()
{
    if (m_layerItem) {
        foreach (QGraphicsItem *item, m_indicatorShapeHash.values())
            m_layerItem->scene()->removeItem(item);
    }
    m_indicatorShapeHash.clear();
}

//static void alignVertices(QPolygonF &polygon, double factor)
//{
//    QMutableVectorIterator<QPointF> iterator(polygon);
//    while (iterator.hasNext()) {
//        QPointF &vertex = iterator.next();
//        vertex.setX(std::floor(vertex.x()) + factor);
//        vertex.setY(std::floor(vertex.y()) + factor);
//    }
//
//}

void SelectionIndicator::setItems(const QList<FormEditorItem*> &itemList)
{
    clear();

    foreach (FormEditorItem *item, itemList) {
        if (!item->qmlItemNode().isValid())
            continue;

        QGraphicsPolygonItem *newSelectionIndicatorGraphicsItem = new QGraphicsPolygonItem(m_layerItem.data());
        m_indicatorShapeHash.insert(item, newSelectionIndicatorGraphicsItem);
        QPolygonF boundingRectInSceneSpace(item->mapToScene(item->qmlItemNode().instanceBoundingRect()));
        //        alignVertices(boundingRectInSceneSpace, 0.5 / item->formEditorView()->widget()->zoomAction()->zoomLevel());
        QPolygonF boundingRectInLayerItemSpace = m_layerItem->mapFromScene(boundingRectInSceneSpace);
        newSelectionIndicatorGraphicsItem->setPolygon(boundingRectInLayerItemSpace);
        newSelectionIndicatorGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, false);

        QPen pen;
        pen.setColor(QColor(108, 141, 221));
        newSelectionIndicatorGraphicsItem->setPen(pen);
        newSelectionIndicatorGraphicsItem->setCursor(m_cursor);
    }
}



void SelectionIndicator::updateItems(const QList<FormEditorItem*> &itemList)
{
    foreach (FormEditorItem *item, itemList) {
        if (m_indicatorShapeHash.contains(item)) {
            QGraphicsPolygonItem *indicatorGraphicsItem =  m_indicatorShapeHash.value(item);
            QPolygonF boundingRectInSceneSpace(item->mapToScene(item->qmlItemNode().instanceBoundingRect()));
//            alignVertices(boundingRectInSceneSpace, 0.5 / item->formEditorView()->widget()->zoomAction()->zoomLevel());
            QPolygonF boundingRectInLayerItemSpace = m_layerItem->mapFromScene(boundingRectInSceneSpace);
            indicatorGraphicsItem->setPolygon(boundingRectInLayerItemSpace);
        }
    }
}

void SelectionIndicator::setCursor(const QCursor &cursor)
{
    m_cursor = cursor;

    foreach (QGraphicsItem  *item, m_indicatorShapeHash.values())
        item->setCursor(cursor);
}

}

