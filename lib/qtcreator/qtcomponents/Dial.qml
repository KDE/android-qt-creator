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

import QtQuick 1.0
import "custom" as Components

// jens: ContainsMouse breaks drag functionality

QStyleItem {
    id: dial

    width:100
    height:100

    property alias maximumValue: range.maximumValue
    property alias minimumValue: range.minimumValue
    property alias containsMouse: mouseArea.containsMouse
    property alias value: range.value

    property bool wrapping: false
    property bool tickmarks: true // not implemented

    RangeModel {
        id: range
        minimumValue: 0.0
        maximumValue: 1.0
        stepSize: 0.0
        value: 0
    }

    MouseArea {
        id: mouseArea
        anchors.fill:parent
        property bool inDrag
        hoverEnabled:true

        onPositionChanged: {
            if (pressed) {
                value = valueFromPoint(mouseX, mouseY)
                inDrag = true
            }
        }
        onPressed: {
            value = valueFromPoint(mouseX, mouseY)
            dial.focus = true
        }

        onReleased:inDrag = false;
        function bound(val) { return Math.max(minimumValue, Math.min(maximumValue, val)); }

        function valueFromPoint(x, y)
        {
            var yy = height/2.0 - y;
            var xx = x - width/2.0;
            var a = (xx || yy) ? Math.atan2(yy, xx) : 0;

            if (a < Math.PI/ -2)
                a = a + Math.PI * 2;

            var dist = 0;
            var minv = minimumValue*100, maxv = maximumValue*100;

            if (minimumValue < 0) {
                dist = -minimumValue;
                minv = 0;
                maxv = maximumValue + dist;
            }

            var r = maxv - minv;
            var v;
            if (wrapping)
                v =  (0.5 + minv + r * (Math.PI * 3 / 2 - a) / (2 * Math.PI));
            else
                v =  (0.5 + minv + r* (Math.PI * 4 / 3 - a) / (Math.PI * 10 / 6));

            if (dist > 0)
                v -= dist;
            return maximumValue - bound(v/100)
        }
    }

    WheelArea {
        id: wheelarea
        anchors.fill: parent
        horizontalMinimumValue: dial.minimumValue
        horizontalMaximumValue: dial.maximumValue
        verticalMinimumValue: dial.minimumValue
        verticalMaximumValue: dial.maximumValue
        property double step: (dial.maximumValue - dial.minimumValue)/100

        onVerticalWheelMoved: {
            value += verticalDelta/4*step
        }

        onHorizontalWheelMoved: {
            value += horizontalDelta/4*step
        }
    }

    elementType:"dial"
    sunken: mouseArea.pressed
    maximum: range.maximumValue*90
    minimum: range.minimumValue*90
    focus:dial.focus
    value: visualPos*90
    enabled: dial.enabled
    property double visualPos : range.value
    Behavior on visualPos {
        enabled: !mouseArea.inDrag
        NumberAnimation {
            duration: 300
            easing.type: Easing.OutSine
        }
    }
}
