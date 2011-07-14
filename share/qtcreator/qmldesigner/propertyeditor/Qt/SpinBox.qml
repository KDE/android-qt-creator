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

QWidget { //This is a special spinBox that does color coding for states

    id: spinBox;

    property variant backendValue;
    property variant baseStateFlag;
    property alias singleStep: box.singleStep;
    property alias minimum: box.minimum
    property alias maximum: box.maximum
    property bool enabled: true

    minimumHeight: 22;

    onBaseStateFlagChanged: {
        evaluate();
    }

    onBackendValueChanged: {
        evaluate();
    }

    property variant isEnabled: spinBox.enabled
    onIsEnabledChanged: {
        evaluate();
    }

    function evaluate() {
        if (backendValue === undefined)
            return;
        if (!enabled) {
            box.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (!(baseStateFlag === undefined) && baseStateFlag) {
                if (backendValue.isInModel)
                    box.setStyleSheet("color: "+scheme.changedBaseColor);
                else
                    box.setStyleSheet("color: "+scheme.defaultColor);
            } else {
                if (backendValue.isInSubState)
                    box.setStyleSheet("color: "+scheme.changedStateColor);
                else
                    box.setStyleSheet("color: "+scheme.defaultColor);
            }
        }
    }

    property bool isInModel: backendValue.isInModel;

    onIsInModelChanged: {
        evaluate();
    }

    property bool isInSubState: backendValue.isInSubState;

    onIsInSubStateChanged: {
        evaluate();
    }

    ColorScheme { id:scheme; }

    layout: HorizontalLayout {
        spacing: 4
        QSpinBox {
            property alias backendValue: spinBox.backendValue
            toolTip: backendValue.isBound ? backendValue.expression : ""

            enabled: !backendValue.isBound && spinBox.enabled;

            keyboardTracking: false;
            id: box;
            property bool readingFromBackend: false;
            property int valueFromBackend: spinBox.backendValue.value;

            onValueFromBackendChanged: {
                readingFromBackend = true;
                if (maximum < valueFromBackend)
                    maximum = valueFromBackend;
                if (minimum > valueFromBackend)
                    minimum = valueFromBackend;
                if (valueFromBackend !== value)
                    value = valueFromBackend;
                readingFromBackend = false;

                if (!focus)
                    evaluate();
            }

            onValueChanged: {
                if (backendValue.value !== value)
                    backendValue.value = value;
            }

            onFocusChanged: {
                if (focus) {
                    transaction.start();
                } else {
                    transaction.end();
                    evaluate();
                }
            }
            onEditingFinished: {
            }
        }
    }

    ExtendedFunctionButton {
        backendValue: spinBox.backendValue;
        y: box.y + 4
        x: box.x + 2
        visible: spinBox.enabled
    }
}
