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

#ifndef ANCHORLINEHANDLEITEM_H
#define ANCHORLINEHANDLEITEM_H

#include <QGraphicsPathItem>

#include "anchorlinecontroller.h"

namespace QmlDesigner {

class AnchorLineHandleItem : public QGraphicsPathItem
{
public:
    enum
    {
        Type = 0xEAEB
    };


    AnchorLineHandleItem(QGraphicsItem *parent, const AnchorLineController &AnchorLineController);

    void setHandlePath(const QPainterPath & path);

    int type() const;
    QRectF boundingRect() const;
    QPainterPath shape() const;

    AnchorLineController anchorLineController() const;
    AnchorLine::Type anchorLine() const;


    static AnchorLineHandleItem* fromGraphicsItem(QGraphicsItem *item);


    bool isTopHandle() const;
    bool isLeftHandle() const;
    bool isRightHandle() const;
    bool isBottomHandle() const;

    QPointF itemSpacePosition() const;

    AnchorLine::Type anchorLineType() const;

    void setHiglighted(bool highlight);


private:
    QWeakPointer<AnchorLineControllerData> m_anchorLineControllerData;
};

inline int AnchorLineHandleItem::type() const
{
    return Type;
}

} // namespace QmlDesigner

#endif // ANCHORLINEHANDLEITEM_H
