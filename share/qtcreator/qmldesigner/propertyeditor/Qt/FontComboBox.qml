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

QWidget {
    id: fontComboBox

    property alias currentFont: fontSelector.currentFont
    property variant backendValue
    property variant baseStateFlag;
    property alias enabled: fontSelector.enabled

    onBaseStateFlagChanged: {
        evaluate();
    }

    onBackendValueChanged: {
        evaluate();
    }

    property variant isEnabled: fontComboBox.enabled
    onIsEnabledChanged: {
        evaluate();
    }

    function evaluate() {
        if (!enabled) {
            fontSelector.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (baseStateFlag) {
                if (backendValue != null && backendValue.isInModel)
                    fontSelector.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.changedBaseColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
                else
                    fontSelector.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.defaultColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
            } else {
                if (backendValue != null && backendValue.isInSubState)
                    fontSelector.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.changedStateColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
                else
                    fontSelector.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.defaultColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
            }
        }
    }

    property bool isInModel: (backendValue === undefined || backendValue === null) ? false: backendValue.isInModel;

    onIsInModelChanged: {
        evaluate();
    }

    property bool isInSubState: (backendValue === undefined || backendValue === null) ? false: backendValue.isInSubState;

    onIsInSubStateChanged: {
        evaluate();
    }

    ColorScheme { id:scheme; }

    layout: HorizontalLayout {
        QFontComboBox {
            editable: false
            id: fontSelector

            currentFont.family: backendValue.value
            property variant fontFamily: currentFont.family
            onFontFamilyChanged: {
                if (backendValue === undefined)
                    return;
                if (backendValue.value != currentFont.family)
                    backendValue.value = currentFont.family;
            }
        }
    }
    ExtendedFunctionButton {
        backendValue: fontComboBox.backendValue
        y: 4
        x: 2
        visible: fontComboBox.enabled
    }
}
