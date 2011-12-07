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

import QtQuick 1.1
import qtcomponents 1.0 as Components

Item {
    id: tabBar

    height: 60
    width: parent.width

    property alias model: tabs.model

    Rectangle {
        id: row
        width: 100
        height: 26
        anchors.top: parent.top
        anchors.left: parent.left
        gradient: Gradient {
            GradientStop { position: 0; color: "#f7f7f7" }
            GradientStop { position: 1; color: "#e4e4e4" }
        }
        Text {
            id: text
            horizontalAlignment: Qt.AlignHCenter; verticalAlignment: Qt.AlignVCenter
            anchors.fill: parent
            text: qsTr("Qt Creator")
        }
    }

    Item {
        anchors.top: parent.top
        anchors.left: row.right
        anchors.right: parent.right
        anchors.bottom: row.bottom

        Rectangle {
            id: left1
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: 1
            gradient: Gradient {
                GradientStop { position: 0; color: "#fcfcfc" }
                GradientStop { position: 1; color: "#f7f7f7" }
            }
        }

        Rectangle {
            id: left2
            anchors.top: parent.top
            anchors.left: left1.right
            anchors.bottom: parent.bottom
            width: 1
            color: "#313131"
        }

        Rectangle {
            id: bottom1
            height: 1
            anchors.left: left1.right
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            color: "#fbfbfb"
        }

        Rectangle {
            id: bottom2
            height: 1
            anchors.left: left2.right
            anchors.right: parent.right
            anchors.bottom: bottom1.top
            width: 1
            color: "#313131"
        }

        Rectangle {
            anchors.top: parent.top
            anchors.left: left2.right
            anchors.right: parent.right
            anchors.bottom: bottom2.top
            gradient: Gradient {
                GradientStop { position: 0.00; color: "#8e8e8e" }
                GradientStop { position: 0.07; color: "#8e8e8e" }
                GradientStop { position: 0.08; color: "#757575" }
                GradientStop { position: 0.40; color: "#666666" }
                GradientStop { position: 0.41; color: "#585858" }
                GradientStop { position: 1.00; color: "#404040" }
             }
        }
    }

    Rectangle {
        id: background
        anchors.bottom: parent.bottom
        width: parent.width
        anchors.top: row.bottom
        gradient: Gradient {
            GradientStop { position: 0; color: "#e4e4e4" }
            GradientStop { position: 1; color: "#cecece" }
        }
        Rectangle {
            color: "black"
            height: 1
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }
    }

    Row {
        height: background.height
        anchors.top: row.bottom
        anchors.topMargin: 6
        width: parent.width
        Repeater {
            id: tabs
            height: parent.height
            model: tabBar.model
            delegate: SingleTab { }
        }
    }
}
