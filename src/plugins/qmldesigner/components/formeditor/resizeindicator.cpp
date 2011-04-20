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

#include "resizeindicator.h"

#include "formeditoritem.h"

namespace QmlDesigner {

ResizeIndicator::ResizeIndicator(LayerItem *layerItem)
    : m_layerItem(layerItem)
{
    Q_ASSERT(layerItem);
}

ResizeIndicator::~ResizeIndicator()
{
    m_itemControllerHash.clear();
}

void ResizeIndicator::show()
{
    QHashIterator<FormEditorItem*, ResizeController> itemControllerIterator(m_itemControllerHash);
    while (itemControllerIterator.hasNext()) {
        ResizeController controller = itemControllerIterator.next().value();
        controller.show();
    }
}
void ResizeIndicator::hide()
{
    QHashIterator<FormEditorItem*, ResizeController> itemControllerIterator(m_itemControllerHash);
    while (itemControllerIterator.hasNext()) {
        ResizeController controller = itemControllerIterator.next().value();
        controller.hide();
    }
}

void ResizeIndicator::clear()
{
    m_itemControllerHash.clear();
}

void ResizeIndicator::setItems(const QList<FormEditorItem*> &itemList)
{
    clear();

    foreach (FormEditorItem* item, itemList) {
        if (item
                && item->qmlItemNode().isValid()
                && item->qmlItemNode().instanceIsResizable()
                && !item->qmlItemNode().instanceIsInPositioner()) {
            ResizeController controller(m_layerItem, item);
            m_itemControllerHash.insert(item, controller);
        }
    }
}

void ResizeIndicator::updateItems(const QList<FormEditorItem*> &itemList)
{
    foreach (FormEditorItem* item, itemList) {
        if (m_itemControllerHash.contains(item)) {
            ResizeController controller(m_itemControllerHash.value(item));
            controller.updatePosition();
        }
    }
}

}
