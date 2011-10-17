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

#include "updateinfobutton.h"

#include <coreplugin/coreconstants.h>
#include <utils/stylehelper.h>

#include <QtGui/QIcon>
#include <QtGui/QPainter>


namespace UpdateInfo {
namespace Internal {

UpdateInfoButton::UpdateInfoButton(QWidget *parent) :
    QAbstractButton(parent)
{
    setIcon(QIcon(QLatin1String(":/updateinfo/images/update_available_logo.png")));
}

//copied from fancytoolbutton
QSize UpdateInfoButton::minimumSizeHint() const
{
    return QSize(8, 8);
}

//copied from fancytoolbutton
QSize UpdateInfoButton::sizeHint() const
{
    return iconSize().expandedTo(QSize(64, 38));
}

//copied from fancytoolbutton and removed unused things
void UpdateInfoButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);

    QRect iconRect(0, 0, Core::Constants::TARGET_ICON_SIZE, Core::Constants::TARGET_ICON_SIZE);


    iconRect.moveCenter(rect().center());
    Utils::StyleHelper::drawIconWithShadow(icon(), iconRect, &painter, isEnabled() ? QIcon::Normal : QIcon::Disabled);

}


} //Internal
} //namespace UpdateInfo
