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

#ifndef ABSTRACTFORMEDITORTOOL_H
#define ABSTRACTFORMEDITORTOOL_H

#include "formeditoritem.h"

QT_BEGIN_NAMESPACE
class QGraphicsItem;
QT_END_NAMESPACE

namespace QmlDesigner {

class FormEditorView;

class AbstractFormEditorTool
{

public:
    AbstractFormEditorTool(FormEditorView* view);

    virtual ~AbstractFormEditorTool();

    virtual void mousePressEvent(const QList<QGraphicsItem*> &itemList,
                         QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                           QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList,
                                       QGraphicsSceneMouseEvent *event) = 0;

    virtual void hoverMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent *event) = 0;

    virtual void dropEvent(QGraphicsSceneDragDropEvent * event);
    virtual void dragEnterEvent(QGraphicsSceneDragDropEvent * event);
    virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent * event);
    virtual void dragMoveEvent(QGraphicsSceneDragDropEvent * event);

    virtual void keyPressEvent(QKeyEvent *event) = 0;
    virtual void keyReleaseEvent(QKeyEvent *keyEvent) = 0;

    virtual void itemsAboutToRemoved(const QList<FormEditorItem*> &itemList) = 0;
    //    virtual QVariant itemChange(QList<QGraphicsItem*> itemList,
//                        QGraphicsItem::GraphicsItemChange change,
//                        const QVariant &value ) = 0;
//    virtual void update() = 0;
    virtual void clear() = 0;

    virtual void formEditorItemsChanged(const QList<FormEditorItem*> &itemList) = 0;

    virtual void instancesCompleted(const QList<FormEditorItem*> &itemList) = 0;

    void setItems(const QList<FormEditorItem*> &itemList);
    QList<FormEditorItem*> items() const;

    static QGraphicsItem* topMovableGraphicsItem(const QList<QGraphicsItem*> &itemList);
    static FormEditorItem* topMovableFormEditorItem(const QList<QGraphicsItem*> &itemList);
    bool topItemIsMovable(const QList<QGraphicsItem*> &itemList);
    bool topSelectedItemIsMovable(const QList<QGraphicsItem*> &itemList);
    bool topItemIsResizeHandle(const QList<QGraphicsItem*> &itemList);

    static FormEditorItem* topFormEditorItem(const QList<QGraphicsItem*> &itemList);
    static FormEditorItem* topFormEditorItemWithRootItem(const QList<QGraphicsItem*> &itemList);
protected:

    virtual void selectedItemsChanged(const QList<FormEditorItem*> &itemList) = 0;


    FormEditorView *view() const;
    FormEditorScene* scene() const;
private:
    FormEditorView *m_view;
    QList<FormEditorItem*> m_itemList;
};

}

#endif // ABSTRACTFORMEDITORTOOL_H
