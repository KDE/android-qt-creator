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

#ifndef SNAPPINGLINECREATOR_H
#define SNAPPINGLINECREATOR_H

#include "layeritem.h"
#include <QPair>

namespace QmlDesigner {

class FormEditorItem;

typedef QMultiMap<double, QPair<QRectF, FormEditorItem*> > SnapLineMap;
typedef QMapIterator<double, QPair<QRectF, FormEditorItem*> > SnapLineMapIterator;

class FormEditorItem;

class SnappingLineCreator
{
public:
    SnappingLineCreator(FormEditorItem *formEditorItem);

    void update(const QList<FormEditorItem*> &exceptionList,
                FormEditorItem *transformationSpaceItem);


    SnapLineMap topLines() const;
    SnapLineMap bottomLines() const;
    SnapLineMap leftLines() const;
    SnapLineMap rightLines() const;
    SnapLineMap horizontalCenterLines() const;
    SnapLineMap verticalCenterLines() const;

    SnapLineMap topOffsets() const;
    SnapLineMap bottomOffsets() const;
    SnapLineMap leftOffsets() const;
    SnapLineMap rightOffsets() const;

    void setMargins(double margin);
    void setSpacing(double spacing);

protected:
    void addLines(const QRectF &rectInSceneSpace, FormEditorItem *item);
    void addOffsets(const QRectF &rectInSceneSpace, FormEditorItem *item);

    void clearLines();
    void generateLines(const QList<FormEditorItem*> &exceptionList,
                       FormEditorItem *transformationSpaceItem);

private:
    SnapLineMap m_topLineMap;
    SnapLineMap m_bottomLineMap;
    SnapLineMap m_leftLineMap;
    SnapLineMap m_rightLineMap;
    SnapLineMap m_horizontalCenterLineMap;
    SnapLineMap m_verticalCenterLineMap;

    SnapLineMap m_topOffsetMap;
    SnapLineMap m_bottomOffsetMap;
    SnapLineMap m_leftOffsetMap;
    SnapLineMap m_rightOffsetMap;

    FormEditorItem *m_formEditorItem;
    double m_topOffset;
    double m_bottomOffset;
    double m_leftOffset;
    double m_rightOffset;
    double m_topMargin;
    double m_bottomMargin;
    double m_leftMargin;
    double m_rightMargin;
};

}
#endif // SNAPPINGLINECREATOR_H
