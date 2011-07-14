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

import Qt 4.7
import Bauhaus 1.0

AnimatedToolButton {
    id: extendedFunctionButton

    property variant backendValue

    hoverIconFromFile: "images/submenu.png";

    function setIcon() {
        if (backendValue == null)
            extendedFunctionButton.iconFromFile = "images/placeholder.png"
        else if (backendValue.isBound ) {
            if (backendValue.isTranslated) { //translations are a special case
                extendedFunctionButton.iconFromFile = "images/placeholder.png"
            } else {
                extendedFunctionButton.iconFromFile = "images/expression.png"
            }
        } else {
            if (backendValue.complexNode != null && backendValue.complexNode.exists) {
                extendedFunctionButton.iconFromFile = "images/behaivour.png"
            } else {
                extendedFunctionButton.iconFromFile = "images/placeholder.png"
            }
        }
    }

    onBackendValueChanged: {
        setIcon();
    }
    property bool isBoundBackend: backendValue.isBound;
    property string backendExpression: backendValue.expression;

    onIsBoundBackendChanged: {
        setIcon();
    }

    onBackendExpressionChanged: {
        setIcon();
    }

    toolButtonStyle: "Qt::ToolButtonIconOnly"
    popupMode: "QToolButton::InstantPopup";
    property bool active: false;

    iconFromFile: "images/placeholder.png";
    width: 14;
    height: 14;
    focusPolicy: "Qt::NoFocus";

    styleSheet: "*::down-arrow, *::menu-indicator { image: none; width: 0; height: 0; }";

    onActiveChanged: {
        if (active) {
            setIcon();
            opacity = 1;
        } else {
            opacity = 0;
        }
    }


    actions:  [
    QAction {
        text: qsTr("Reset")
        visible: backendValue.isInSubState || backendValue.isInModel
        onTriggered: {
            transaction.start();
            backendValue.resetValue();
            backendValue.resetValue();
            transaction.end();
        }

    },
    QAction {
        text: qsTr("Set Expression");
        onTriggered: {
            expressionEdit.globalY = extendedFunctionButton.globalY;
            expressionEdit.backendValue = extendedFunctionButton.backendValue
            expressionEdit.show();
            expressionEdit.raise();
            expressionEdit.active = true;
        }
    }
    ]
}
