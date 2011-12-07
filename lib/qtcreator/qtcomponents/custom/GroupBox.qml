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

FocusScope {
    id: groupbox

    width: Math.max(200, contentWidth + loader.leftMargin + loader.rightMargin)
    height: contentHeight + loader.topMargin + loader.bottomMargin

    default property alias children: content.children

    property string title
    property bool checkable: false
    property int contentWidth: content.childrenRect.width
    property int contentHeight: content.childrenRect.height
    property double contentOpacity: 1

    property Component background: null
    property Item backgroundItem: loader.item

    property CheckBox checkbox: check
    property alias checked: check.checked

    Loader {
        id: loader
        anchors.fill: parent
        property int topMargin: 22
        property int bottomMargin: 4
        property int leftMargin: 4
        property int rightMargin: 4

        property alias styledItem: groupbox
        sourceComponent: background

        Item {
            id:content
            z: 1
            opacity: contentOpacity
            anchors.topMargin: loader.topMargin
            anchors.leftMargin: 8
            anchors.top:parent.top
            anchors.left:parent.left
            enabled: (!checkable || checkbox.checked)
        }

        CheckBox {
            id: check
            checked: true
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: loader.topMargin
        }
    }
}
