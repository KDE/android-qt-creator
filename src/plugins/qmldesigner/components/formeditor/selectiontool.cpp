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

#include "selectiontool.h"
#include "formeditorscene.h"
#include "formeditorview.h"

#include "resizehandleitem.h"


#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QtDebug>
#include <QClipboard>

namespace QmlDesigner {

static const int s_startDragDistance = 20;
static const int s_startDragTime = 50;

SelectionTool::SelectionTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView),
    m_rubberbandSelectionManipulator(editorView->scene()->manipulatorLayerItem(), editorView),
    m_singleSelectionManipulator(editorView),
    m_selectionIndicator(editorView->scene()->manipulatorLayerItem()),
    m_resizeIndicator(editorView->scene()->manipulatorLayerItem()),
    m_selectOnlyContentItems(false)
{
//    view()->setCursor(Qt::CrossCursor);
}


SelectionTool::~SelectionTool()
{
}

void SelectionTool::mousePressEvent(const QList<QGraphicsItem*> &itemList,
                                    QGraphicsSceneMouseEvent *event)
{
    m_mousePressTimer.start();
    FormEditorItem* formEditorItem = topFormEditorItem(itemList);
    if (formEditorItem
        && formEditorItem->qmlItemNode().isValid()
        && !formEditorItem->qmlItemNode().hasChildren()) {
        m_singleSelectionManipulator.begin(event->scenePos());

        if (event->modifiers().testFlag(Qt::ControlModifier))
            m_singleSelectionManipulator.select(SingleSelectionManipulator::RemoveFromSelection, m_selectOnlyContentItems);
        else if (event->modifiers().testFlag(Qt::ShiftModifier))
            m_singleSelectionManipulator.select(SingleSelectionManipulator::AddToSelection, m_selectOnlyContentItems);
        else
            m_singleSelectionManipulator.select(SingleSelectionManipulator::InvertSelection, m_selectOnlyContentItems);
    } else {
        if (event->modifiers().testFlag(Qt::AltModifier)) {
            m_singleSelectionManipulator.begin(event->scenePos());

            if (event->modifiers().testFlag(Qt::ControlModifier))
                m_singleSelectionManipulator.select(SingleSelectionManipulator::RemoveFromSelection, m_selectOnlyContentItems);
            else if (event->modifiers().testFlag(Qt::ShiftModifier))
                m_singleSelectionManipulator.select(SingleSelectionManipulator::AddToSelection, m_selectOnlyContentItems);
            else
                m_singleSelectionManipulator.select(SingleSelectionManipulator::InvertSelection, m_selectOnlyContentItems);

            m_singleSelectionManipulator.end(event->scenePos());
            view()->changeToMoveTool(event->scenePos());
        } else {
            m_rubberbandSelectionManipulator.begin(event->scenePos());
        }
    }
}

void SelectionTool::mouseMoveEvent(const QList<QGraphicsItem*> &/*itemList*/,
                                   QGraphicsSceneMouseEvent *event)
{
    if (m_singleSelectionManipulator.isActive()) {
        QPointF mouseMovementVector = m_singleSelectionManipulator.beginPoint() - event->scenePos();
        if ((mouseMovementVector.toPoint().manhattanLength() > s_startDragDistance)
            && (m_mousePressTimer.elapsed() > s_startDragTime)) {
            m_singleSelectionManipulator.end(event->scenePos());
            view()->changeToMoveTool(m_singleSelectionManipulator.beginPoint());
            return;
        }
    } else if (m_rubberbandSelectionManipulator.isActive()) {
        QPointF mouseMovementVector = m_rubberbandSelectionManipulator.beginPoint() - event->scenePos();
        if ((mouseMovementVector.toPoint().manhattanLength() > s_startDragDistance)
            && (m_mousePressTimer.elapsed() > s_startDragTime)) {
            m_rubberbandSelectionManipulator.update(event->scenePos());

            if (event->modifiers().testFlag(Qt::ControlModifier))
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::RemoveFromSelection);
            else if (event->modifiers().testFlag(Qt::ShiftModifier))
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::AddToSelection);
            else
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::ReplaceSelection);
        }
    }
}

void SelectionTool::hoverMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent * /*event*/)
{
    if (!itemList.isEmpty()) {

        ResizeHandleItem* resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.first());
        if (resizeHandle) {
            view()->changeToResizeTool();
            return;
        }

        if (topSelectedItemIsMovable(itemList))
            view()->changeToMoveTool();
    }

    FormEditorItem *topSelectableItem = 0;

    foreach(QGraphicsItem* item, itemList)
    {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);

        if (formEditorItem
            && formEditorItem->qmlItemNode().isValid()
            && !formEditorItem->qmlItemNode().instanceIsInPositioner()
            && formEditorItem->qmlItemNode().instanceIsMovable()
            && (formEditorItem->qmlItemNode().hasShowContent() || !m_selectOnlyContentItems))
        {
            topSelectableItem = formEditorItem;
            break;
        }
    }

    scene()->highlightBoundingRect(topSelectableItem);
}

void SelectionTool::mouseReleaseEvent(const QList<QGraphicsItem*> &/*itemList*/,
                                      QGraphicsSceneMouseEvent *event)
{
    if (m_singleSelectionManipulator.isActive()) {
        m_singleSelectionManipulator.end(event->scenePos());
    }
    else if (m_rubberbandSelectionManipulator.isActive()) {

        QPointF mouseMovementVector = m_rubberbandSelectionManipulator.beginPoint() - event->scenePos();
        if (mouseMovementVector.toPoint().manhattanLength() < s_startDragDistance) {
            m_singleSelectionManipulator.begin(event->scenePos());

            if (event->modifiers().testFlag(Qt::ControlModifier))
                m_singleSelectionManipulator.select(SingleSelectionManipulator::RemoveFromSelection, m_selectOnlyContentItems);
            else if (event->modifiers().testFlag(Qt::ShiftModifier))
                m_singleSelectionManipulator.select(SingleSelectionManipulator::AddToSelection, m_selectOnlyContentItems);
            else
                m_singleSelectionManipulator.select(SingleSelectionManipulator::InvertSelection, m_selectOnlyContentItems);

            m_singleSelectionManipulator.end(event->scenePos());
        } else {
            m_rubberbandSelectionManipulator.update(event->scenePos());

            if (event->modifiers().testFlag(Qt::ControlModifier))
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::RemoveFromSelection);
            else if (event->modifiers().testFlag(Qt::ShiftModifier))
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::AddToSelection);
            else
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::ReplaceSelection);

            m_rubberbandSelectionManipulator.end();
        }
    }

}

void SelectionTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> &/*itemList*/,
                                          QGraphicsSceneMouseEvent * /*event*/)
{

}

void SelectionTool::keyPressEvent(QKeyEvent *event)
{
    switch(event->key()) {
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            view()->changeToMoveTool();
            view()->currentTool()->keyPressEvent(event);
            break;
    }
}

void SelectionTool::keyReleaseEvent(QKeyEvent * /*keyEvent*/)
{

}

void SelectionTool::setSelectOnlyContentItems(bool selectOnlyContentItems)
{
    m_selectOnlyContentItems = selectOnlyContentItems;
}

void SelectionTool::itemsAboutToRemoved(const QList<FormEditorItem*> &/*itemList*/)
{

}

//QVariant SelectionTool::itemChange(const QList<QGraphicsItem*> &itemList,
//                                           QGraphicsItem::GraphicsItemChange change,
//                                           const QVariant &value )
//{
//    qDebug() << Q_FUNC_INFO;
//    return QVariant();
//}

//void SelectionTool::update()
//{
//
//}


void SelectionTool::clear()
{
    m_rubberbandSelectionManipulator.clear(),
    m_singleSelectionManipulator.clear();
    m_selectionIndicator.clear();
    m_resizeIndicator.clear();
}

void SelectionTool::selectedItemsChanged(const QList<FormEditorItem*> &itemList)
{
    m_selectionIndicator.setItems(itemList);
    m_resizeIndicator.setItems(itemList);
}

void SelectionTool::formEditorItemsChanged(const QList<FormEditorItem*> &itemList)
{
    m_selectionIndicator.updateItems(itemList);
    m_resizeIndicator.updateItems(itemList);
}

void SelectionTool::instancesCompleted(const QList<FormEditorItem*> &/*itemList*/)
{
}

void SelectionTool::selectUnderPoint(QGraphicsSceneMouseEvent *event)
{
    m_singleSelectionManipulator.begin(event->scenePos());

    if (event->modifiers().testFlag(Qt::ControlModifier))
        m_singleSelectionManipulator.select(SingleSelectionManipulator::RemoveFromSelection, m_selectOnlyContentItems);
    else if (event->modifiers().testFlag(Qt::ShiftModifier))
        m_singleSelectionManipulator.select(SingleSelectionManipulator::AddToSelection, m_selectOnlyContentItems);
    else
        m_singleSelectionManipulator.select(SingleSelectionManipulator::InvertSelection, m_selectOnlyContentItems);

    m_singleSelectionManipulator.end(event->scenePos());
}

}
