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

#include "componentaction.h"

#include <QComboBox>
#include "componentview.h"
#include <QStandardItemModel>
#include <QtDebug>

namespace QmlDesigner {

ComponentAction::ComponentAction(ComponentView  *componentView)
  :  QWidgetAction(componentView),
     m_componentView(componentView)
{
}

void ComponentAction::setCurrentIndex(int i)
{
    emit currentIndexChanged(i);
}

QWidget  *ComponentAction::createWidget(QWidget *parent)
{
    QComboBox *comboBox = new QComboBox(parent);
    comboBox->setMinimumWidth(120);
    comboBox->setToolTip(tr("Edit sub components defined in this file"));
    comboBox->setModel(m_componentView->standardItemModel());
    connect(comboBox, SIGNAL(currentIndexChanged(int)), SLOT(emitCurrentComponentChanged(int)));
    connect(this, SIGNAL(currentIndexChanged(int)), comboBox, SLOT(setCurrentIndex(int)));

    return comboBox;
}

void ComponentAction::emitCurrentComponentChanged(int index)
{
    emit currentComponentChanged(m_componentView->modelNode(index));
}

} // namespace QmlDesigner
