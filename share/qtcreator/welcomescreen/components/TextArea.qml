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

ScrollArea {
    id:area
    color: "white"
    width: 280
    height: 120
    contentWidth: 200

    property alias text: edit.text
    property alias wrapMode: edit.wrapMode
    property alias readOnly: edit.readOnly

    highlightOnFocus: true
    property int documentMargins: 4
    frame: true

    Item {
        anchors.left: parent.left
        anchors.top: parent.top
        height: edit.height - 8
        anchors.margins: documentMargins

        TextEdit {
            id: edit
            text: loremIpsum + loremIpsum;
            wrapMode: TextEdit.WordWrap;
            width: area.contentWidth
            selectByMouse: true
            readOnly: false
            focus: true

            // keep textcursor within scrollarea
            onCursorPositionChanged: {
                if (cursorRectangle.y >= area.contentY + area.height - 1.5*cursorRectangle.height)
                    area.contentY = cursorRectangle.y - area.height + 1.5*cursorRectangle.height
                else if (cursorRectangle.y < area.contentY)
                    area.contentY = cursorRectangle.y
            }
        }
    }

    Keys.onPressed: {
        if (event.key == Qt.Key_PageUp) {
            verticalValue = verticalValue - area.height
        } else if (event.key == Qt.Key_PageDown)
            verticalValue = verticalValue + area.height
   }
}
