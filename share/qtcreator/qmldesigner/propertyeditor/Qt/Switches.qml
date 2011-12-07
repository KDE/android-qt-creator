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

QFrame {
    styleSheetFile: "switch.css";
    property variant specialModeIcon;
    specialModeIcon: "images/standard.png";
    maximumWidth: 300;
    minimumWidth: 300;
    layout: QHBoxLayout {
        topMargin: 0;
        bottomMargin: 0;
        leftMargin: 0;
        rightMargin: 40;
        spacing: 0;

        QPushButton {
            checkable: true;
            checked: true;
            id: standardMode;
            toolTip: qsTr("Special properties");
            //iconFromFile: "images/rect-icon.png";
            text: backendValues === undefined || backendValues.className === undefined || backendValues.className == "empty" ? "empty" : backendValues.className.value
            onClicked: {
                extendedMode.checked = false;
                layoutMode.checked = false;
                checked = true;
                standardPane.visible = true;
                extendedPane.visible = false;
                layoutPane.visible = false;
            }
        }

        QPushButton {
            id: layoutMode;
            checkable: true;
            checked: false;
            toolTip: qsTr("Layout");
            text: qsTr("Layout");
            onClicked: {
                extendedMode.checked = false;
                standardMode.checked = false;
                checked = true;
                standardPane.visible = false;
                extendedPane.visible = false;
                layoutPane.visible = true;
            }
        }

        QPushButton {
            id: extendedMode;
            toolTip: qsTr("Advanced properties");
            checkable: true;
            checked: false;
            text: qsTr("Advanced")
            onClicked: {
                standardMode.checked = false;
                layoutMode.checked = false;
                checked = true;
                standardPane.visible = false;
                extendedPane.visible = true;
                layoutPane.visible = false;
            }
        }

    }
}
