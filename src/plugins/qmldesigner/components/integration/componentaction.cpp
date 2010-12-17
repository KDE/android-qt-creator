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

#include "componentaction.h"

#include <QComboBox>
#include "componentview.h"
#include <QStandardItemModel>
#include <QtDebug>

namespace QmlDesigner {

ComponentAction::ComponentAction(QObject *parent)
  :  QWidgetAction(parent),
     m_componentView(new ComponentView(this))
{
}

void ComponentAction::setModel(Model* model)
{
    if (model == m_componentView->model())
        return;

    blockSignals(true);

    if (model)
        model->attachView(m_componentView.data());
    else if (m_componentView->model())
        m_componentView->model()->detachView(m_componentView.data());

    blockSignals(false);
}

QWidget  *ComponentAction::createWidget(QWidget *parent)
{
    QComboBox *comboBox = new QComboBox(parent);
    comboBox->setModel(m_componentView->standardItemModel());
    connect(comboBox, SIGNAL(currentIndexChanged(int)), SLOT(emitCurrentComponentChanged(int)));

    return comboBox;
}

void ComponentAction::emitCurrentComponentChanged(int index)
{
    emit currentComponentChanged(m_componentView->modelNode(index));
}

} // namespace QmlDesigner
