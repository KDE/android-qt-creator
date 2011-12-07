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

import QtQuick 1.0

Item {
    id: behavior

    signal clicked
    property bool pressed: false    // Can't be alias of mouseArea.pressed because the latter is read-only
    property alias containsMouse: mouseArea.containsMouse
    property bool checkable: false
    property bool checked: false
    property bool triState: false
    signal mouseMoved

    onCheckableChanged: { if(!checkable) checked = false }
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onPositionChanged: behavior.mouseMoved()
        onPressed: behavior.pressed = true  // needed when hover is enabled
        onEntered: if(pressed && enabled) behavior.pressed = true
        onExited: behavior.pressed = false
        onCanceled: behavior.pressed = false    // mouse stolen e.g. by Flickable
        onReleased: {
            if(behavior.pressed && behavior.enabled) { // No click if release outside area
                behavior.pressed = false
                if(behavior.checkable)
                    behavior.checked = !behavior.checked;
                behavior.clicked()
            }
        }
    }
}
