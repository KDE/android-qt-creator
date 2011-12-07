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

GroupBox {
    finished: finishedNotify;
    caption: qsTr("Transformation")
    maximumHeight: 200;
    id: transformation;

    layout: VerticalLayout {
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Origin")
                }
                OriginWidget {
                    id: originWidget
                    
                    contextMenuPolicy: "Qt::ActionsContextMenu"
                    origin: backendValues.transformOrigin.value
                    onOriginChanged: {
                        backendValues.transformOrigin.value = origin;
                    }
                    marked: backendValues.transformOrigin.isInSubState
                    
                    ExtendedFunctionButton {
                        backendValue: backendValues.transformOrigin
                        y: 2
                        x: 56
                        visible: originWidget.enabled
                    }
                    
                    actions:  [
                        QAction { text: qsTr("Top left"); onTriggered: originWidget.origin = "TopLeft"; },
                        QAction { text: qsTr("Top"); onTriggered: originWidget.origin = "Top"; },
                        QAction { text: qsTr("Top right"); onTriggered: originWidget.origin = "TopRight"; },
                        QAction { text: qsTr("Left"); onTriggered: originWidget.origin = "Left"; },
                        QAction {text: qsTr("Center"); onTriggered: originWidget.origin = "Center"; },
                        QAction { text: qsTr("Right"); onTriggered: originWidget.origin = "Right"; },
                        QAction { text: qsTr("Bottom left"); onTriggered: originWidget.origin = "BottomLeft"; },
                        QAction { text: qsTr("Bottom"); onTriggered: originWidget.origin = "Bottom"; },
                        QAction { text: qsTr("Bottom right"); onTriggered: originWidget.origin = "BottomRight"; }
                    ]
                }
            }
        }
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Scale")
                }

                DoubleSpinBox {
                    text: ""
                    id: scaleSpinBox;

                    backendValue: backendValues.scale;
                    property variant backendValueValue: backendValues.scale.value;
                    minimumWidth: 60;
                    minimum: 0.01
                    maximum: 10
                    singleStep: 0.1
                    baseStateFlag: isBaseState;                    
                }
                SliderWidget {
                    id: scaleSlider;
                    backendValue: backendValues.scale;
                    property variant pureValue: backendValues.scale.value;
                    onPureValueChanged: {
                        if (value != pureValue * 10)
                            value = pureValue * 10;
                    }
                    minimum: 1;
                    maximum: 10;
                    singleStep: 1;
                    onValueChanged: {
                        if ((value > 0) && (value < 11))
                            backendValues.scale.value = value / 10;
                    }
                }
            }
        }
        IntEditor {
            backendValue: backendValues.rotation
            caption: qsTr("Rotation")
            baseStateFlag: isBaseState;
            step: 10;
            minimumValue: -360;
            maximumValue: 360;
        }

        IntEditor {
            backendValue: backendValues.z
            caption: "z"
            baseStateFlag: isBaseState;
            step: 1;
            minimumValue: -100;
            maximumValue: 100;
        }
    }
}
