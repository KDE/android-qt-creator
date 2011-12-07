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

Rectangle {
    id: rangeMover


    property color rangeColor:"#444a64b8"
    property color borderColor:"#cc4a64b8"
    property color dragMarkerColor: "#4a64b8"
    width: 20
    height: 50

    color: rangeColor

    property bool dragStarted: false
    onXChanged: {
        if (dragStarted) canvas.updateRange()
    }

    MouseArea {
        anchors.fill: parent
        drag.target: rangeMover
        drag.axis: "XAxis"
        drag.minimumX: 0
        drag.maximumX: canvas.width - rangeMover.width
        onPressed: {
            parent.dragStarted = true;
        }
        onReleased: {
            parent.dragStarted = false;
        }
    }

    Rectangle {
        id: leftRange

        // used for dragging the borders
        property real initialX: 0
        property real initialWidth: 0

        x: 0
        height: parent.height
        width: 1
        color: borderColor

        Rectangle {
            id: leftBorderHandle
            height: parent.height
            x: -width
            width: 7
            color: "#869cd1"
            visible: false
            Image {
                source: "range_handle.png"
                x: 2
                width: 4
                height: 9
                fillMode: Image.Tile
                y: rangeMover.height / 2 - 4
            }
        }

        states: State {
            name: "highlighted"
            PropertyChanges {
                target: leftBorderHandle
                visible: true
            }
        }

        onXChanged: {
            if (x !== 0) {
                rangeMover.width = initialWidth - x;
                rangeMover.x = initialX + x;
                x = 0;
                canvas.updateRange();
            }
        }

        MouseArea {
            x: -10
            width: 13
            y: 0
            height: parent.height

            drag.target: leftRange
            drag.axis: "XAxis"
            drag.minimumX: -parent.initialX
            drag.maximumX: parent.initialWidth - 2

            hoverEnabled: true

            onEntered: {
                parent.state = "highlighted";
            }
            onExited: {
                if (!pressed) parent.state = "";
            }
            onReleased: {
                if (!containsMouse) parent.state = "";
            }
            onPressed: {
                parent.initialX = rangeMover.x;
                parent.initialWidth = rangeMover.width;
            }
        }
    }

    Rectangle {
        id: rightRange

        x: rangeMover.width
        height: parent.height
        width: 1
        color: borderColor

        Rectangle {
            id: rightBorderHandle
            height: parent.height
            x: 1
            width: 7
            color: "#869cd1"
            visible: false
            Image {
                source: "range_handle.png"
                x: 2
                width: 4
                height: 9
                fillMode: Image.Tile
                y: rangeMover.height / 2 - 4
            }
        }

        states: State {
            name: "highlighted"
            PropertyChanges {
                target: rightBorderHandle
                visible: true
            }
        }

        onXChanged: {
            if (x!=rangeMover.width) {
                rangeMover.width = x;
                canvas.updateRange();
            }
        }

        MouseArea {
            x: -3
            width: 13
            y: 0
            height: parent.height

            drag.target: rightRange
            drag.axis: "XAxis"
            drag.minimumX: 1
            drag.maximumX: canvas.width - rangeMover.x

            hoverEnabled: true

            onEntered: {
                parent.state = "highlighted";
            }
            onExited: {
                if (!pressed) parent.state = "";
            }
            onReleased: {
                if (!containsMouse) parent.state = "";
            }
        }
    }

}
