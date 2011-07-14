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

    id: comboBox

    property variant backendValue;
    property variant baseStateFlag;
    property alias enabled: box.enabled;

    property alias items: box.items;
    property alias currentText: box.currentText;


    onBaseStateFlagChanged: {
        evaluate();
    }

    property variant isEnabled: comboBox.enabled
    onIsEnabledChanged: {
        evaluate();
    }

    function evaluate() {
        if (backendValue === undefined)
            return;
        if (!enabled) {
            box.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (baseStateFlag) {
                if (backendValue.isInModel)
                    box.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.changedBaseColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
                else
                    box.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.defaultColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
            } else {
                if (backendValue.isInSubState)
                    box.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.changedStateColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
                else
                    box.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.defaultColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
            }
        }
    }

    ColorScheme { id:scheme; }

    layout: HorizontalLayout {
        QComboBox {
            id: box
            property variant backendValue: comboBox.backendValue
            onCurrentTextChanged: { backendValue.value = currentText; evaluate(); }
			onItemsChanged: {				
				if (comboBox.backendValue.value == curentText)
				    return;
				box.setCurrentTextSilent(comboBox.backendValue.value);
            }			
			
			property variant backendValueValue: comboBox.backendValue.value
			onBackendValueValueChanged: {			 
			    if (comboBox.backendValue.value == curentText)
				    return;					
				box.setCurrentTextSilent(comboBox.backendValue.value);
			}
            ExtendedFunctionButton {
                backendValue: comboBox.backendValue;
                y: 3
                x: 3
            }
        }
    }
}
