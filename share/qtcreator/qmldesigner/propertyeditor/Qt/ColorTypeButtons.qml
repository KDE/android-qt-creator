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

import Qt 4.7
import Bauhaus 1.0

QWidget {
    property bool showGradientButton

    property bool gradient: gradientButton.checked
    property bool none: noneButton.checked
    property bool solid: solidButton.checked

    property bool setGradient: false
    property bool setNone: false
    property bool setSolid: false

    onSetGradientChanged: {
        if (setGradient == true) {
            gradientButton.checked = true;
            setGradient = false;
        }
    }

    onSetNoneChanged: {
        if (setNone == true) {
            noneButton.checked = true;
            setNone = false;
        }
    }

    onSetSolidChanged: {
        if (setSolid == true) {
            solidButton.checked = true;
            setSolid = false;
        }
    }

    fixedHeight: 28
    fixedWidth: 93
    width: fixedWidth
    height: fixedHeight
    enabled: isBaseState

    QPushButton {
        id: solidButton
        x: 0
        checkable: true
        checked: true
        fixedWidth: 31
        fixedHeight: 28
        
        
        styleSheetFile: "styledbuttonleft.css"                
        iconFromFile: "images/icon_color_solid.png"
        toolTip: baseStateFlag ? qsTr("Solid color") : qsTr("Solid color (only editable in base state)")

        onToggled: {
            if (checked) {
                gradientButton.checked = false;
                noneButton.checked = false;
            }
        }
        onClicked: {
            gradientButton.checked = false;
            noneButton.checked = false;
            checked = true;
        }
    }

    QPushButton {
        visible: showGradientButton
        id: gradientButton
        x: 31
        checkable: true
        fixedWidth: 31
        fixedHeight: 28

        styleSheetFile: "styledbuttonmiddle.css"
        iconFromFile: "images/icon_color_gradient.png"
        toolTip: baseStateFlag ? qsTr("Gradient") : qsTr("Gradient (only editable in base state)")

        onToggled: {
            if (checked) {
                solidButton.checked = false;
                noneButton.checked = false;
            }
        }

        onClicked: {
            solidButton.checked = false;
            noneButton.checked = false;
            checked = true;
        }
    }

    QPushButton {
        id: noneButton
        x: showGradientButton ? 62 : 31;
        checkable: true
        fixedWidth: 31
        fixedHeight: 28
        styleSheetFile: "styledbuttonright.css"
        iconFromFile: "images/icon_color_none.png"
        toolTip: baseStateFlag ? qsTr("Transparent") : qsTr("Transparent (only editable in base state)")

        onToggled: {
            if (checked) {
                gradientButton.checked = false;
                solidButton.checked = false;
            }
        }

        onClicked: {
            gradientButton.checked = false;
            solidButton.checked = false;
            checked = true;
        }

    }
}
