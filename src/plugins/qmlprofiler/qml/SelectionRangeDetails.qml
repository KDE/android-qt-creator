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
import Monitor 1.0

Item {
    id: selectionRangeDetails

    property string startTime
    property string endTime
    property string duration
    property bool showDuration

    width: 170
    height: col.height + 30
    z: 1
    visible: false
    x: 200
    y: 125

    // shadow
    BorderImage {
        property int px: 4
        source: "dialog_shadow.png"

        border {
            left: px; top: px
            right: px; bottom: px
        }
        width: parent.width + 2*px - 1
        height: parent.height
        x: -px + 1
        y: px + 1
    }

    // title bar
    Rectangle {
        width: parent.width
        height: 20
        color: "#4a64b8"
        radius: 5
        border.width: 1
        border.color: "#a0a0a0"
    }
    Item {
        width: parent.width+1
        height: 11
        y: 10
        clip: true
        Rectangle {
            width: parent.width-1
            height: 15
            y: -5
            color: "#4a64b8"
            border.width: 1
            border.color: "#a0a0a0"
        }
    }

    //title
    Text {
        id: typeTitle
        text: "  "+qsTr("Selection")
        font.bold: true
        height: 18
        y: 2
        verticalAlignment: Text.AlignVCenter
        width: parent.width
        color: "white"
    }

    // Details area
    Rectangle {
        color: "white"
        width: parent.width
        height: col.height + 10
        y: 20
        border.width: 1
        border.color: "#a0a0a0"
        Column {
            id: col
            x: 10
            y: 5
            Detail {
                label: qsTr("Start")
                content:  selectionRangeDetails.startTime
            }
            Detail {
                label: qsTr("End")
                visible: selectionRangeDetails.showDuration
                content:  selectionRangeDetails.endTime
            }
            Detail {
                label: qsTr("Duration")
                visible: selectionRangeDetails.showDuration
                content: selectionRangeDetails.duration
            }
        }
    }

    MouseArea {
        width: col.width + 45
        height: col.height + 30
        drag.target: parent
        onClicked: {
            if ((selectionRange.x < flick.contentX) ^ (selectionRange.x+selectionRange.width > flick.contentX + flick.width)) {
                root.recenter(selectionRange.startTime + selectionRange.duration/2);
            }
        }
    }

    Text {
        id: closeIcon
        x: selectionRangeDetails.width - 14
        y: 4
        text:"X"
        color: "white"
        MouseArea {
            anchors.fill: parent
            anchors.leftMargin: -8
            onClicked: {
                root.selectionRangeMode = false;
                root.updateRangeButton();
            }
        }
    }

}
