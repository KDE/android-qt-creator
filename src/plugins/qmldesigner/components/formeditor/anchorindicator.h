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

#ifndef ANCHORINDICATOR_H
#define ANCHORINDICATOR_H

#include "anchorcontroller.h"
#include <QList>
#include <QHash>

namespace QmlDesigner {

class AnchorIndicator
{
public:
    AnchorIndicator(LayerItem *layerItem);
    ~AnchorIndicator();

    void show();
    void hide();
    void clear();

    void setItems(const QList<FormEditorItem*> &itemList);

    void updateItems(const QList<FormEditorItem*> &itemList);
    void updateTargetPoint(FormEditorItem *item, AnchorLine::Type anchorLine, const QPointF &targetPoint);

    void clearHighlight();
    void highlight(FormEditorItem *item, AnchorLine::Type anchorLine);

private:
    QHash<FormEditorItem*, AnchorController> m_itemControllerHash;
    LayerItem *m_layerItem;
};

} // namespace QmlDesigner

#endif // ANCHORINDICATOR_H
