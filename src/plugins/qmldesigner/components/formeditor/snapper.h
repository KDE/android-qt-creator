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

#ifndef SNAPPER_H
#define SNAPPER_H

#include "formeditoritem.h"

QT_BEGIN_NAMESPACE
class QLineF;
QT_END_NAMESPACE

namespace QmlDesigner {


class Snapper
{
public:
    Snapper();

    void setContainerFormEditorItem(FormEditorItem *formEditorItem);
    FormEditorItem *containerFormEditorItem() const;

    void setTransformtionSpaceFormEditorItem(FormEditorItem *formEditorItem);
    FormEditorItem *transformtionSpaceFormEditorItem() const;

    double snappedVerticalOffset(const QRectF &boundingRect) const;
    double snappedHorizontalOffset(const QRectF &boundingRect) const;

    double snapTopOffset(const QRectF &boundingRect) const;
    double snapRightOffset(const QRectF &boundingRect) const;
    double snapLeftOffset(const QRectF &boundingRect) const;
    double snapBottomOffset(const QRectF &boundingRect) const;

    QList<QLineF> horizontalSnappedLines(const QRectF &boundingRect, QList<QRectF> *boundingRects = 0) const;
    QList<QLineF> verticalSnappedLines(const QRectF &boundingRect, QList<QRectF> *boundingRects = 0) const;

    void setSnappingDistance(double snappingDistance);
    double snappingDistance() const;

    void updateSnappingLines(const QList<FormEditorItem*> &exceptionList);
    void updateSnappingLines(FormEditorItem* exceptionItem);

    QList<QGraphicsItem*> generateSnappingLines(const QList<QRectF> &boundingRectList,
                                                QGraphicsItem *layerItem,
                                                const QTransform &transform);
    QList<QGraphicsItem*> generateSnappingLines(const QRectF &boundingRect,
                                                QGraphicsItem *layerItem,
                                                const QTransform &transform);
protected:
    double snappedOffsetForLines(const SnapLineMap &snappingLineMap,
                         double value) const;

    double snappedOffsetForOffsetLines(const SnapLineMap &snappingOffsetMap,
                         Qt::Orientation orientation,
                         double value,
                         double lowerLimit,
                         double upperLimit) const;

    QList<QLineF> findSnappingLines(const SnapLineMap &snappingLineMap,
                         Qt::Orientation orientation,
                         double snapLine,
                         double lowerLimit,
                         double upperLimit,
                         QList<QRectF> *boundingRects = 0) const;

    QList<QLineF> findSnappingOffsetLines(const SnapLineMap &snappingOffsetMap,
                                    Qt::Orientation orientation,
                                    double snapLine,
                                    double lowerLimit,
                                    double upperLimit,
                                    QList<QRectF> *boundingRects = 0) const;

    QLineF createSnapLine(Qt::Orientation orientation,
                         double snapLine,
                         double lowerEdge,
                         double upperEdge,
                         const QRectF &itemRect) const;

//    bool canSnap(QList<double> snappingLineList, double value) const;
private:
    FormEditorItem *m_containerFormEditorItem;
    FormEditorItem *m_transformtionSpaceFormEditorItem;
    double m_snappingDistance;
};

}
#endif // SNAPPER_H
