import Qt 4.7
import Bauhaus 1.0

QExtGroupBox {
    id: colorGroupBox

    property variant finished;
    property variant backendColor
    property bool gradientEditing: gradientButtonChecked
    property variant singleColor: (backendColor === undefined || backendColor.value  === undefined) ? "#000000" : backendColor.value
    property variant gradientColor: "#000000"
    property variant color: "#000000"
    property variant oldMaximumHeight;

    property variant startupCollapse: selectionChanged === undefined ? false : selectionChanged;
    property variant firstTime: true;
    property variant caption: ""
    property bool showButtons: false
    property bool showGradientButton: false

    property bool gradientButtonChecked: buttons.gradient
    property bool noneButtonChecked: buttons.none
    property bool solidButtonChecked: buttons.solid

    property alias setGradientButtonChecked: buttons.setGradient
    property alias setNoneButtonChecked: buttons.setNone
    property alias setSolidButtonChecked: buttons.setSolid

    property alias collapseBox: colorButton.checked

    property alias alpha: colorControl.alpha

    smooth: false

    onGradientColorChanged: {
        if (gradientEditing == true)
            color = gradientColor;
        colorChanged();
    }
    onSingleColorChanged: {
        if (!gradientEditing == true && color != singleColor)
            color = singleColor;
    }

    onGradientButtonCheckedChanged: {
        if (buttons.gradient == true)
            color = gradientColor;
        if (!buttons.gradient == true && color != singleColor)
            color = singleColor;
        colorChanged();
    }

    onFinishedChanged: {
        oldMaximumHeight = maximumHeight;
        //visible = false;
        //visible = true;
        //if (finished)
        //collapsed = true;
    }

    onStartupCollapseChanged: {
        oldMaximumHeight = maximumHeight;
        if (!collapsed && firstTime) {
            collapsed = true;
            colorButton.checked = false;
            firstTime = false;
        }
    }


    property variant baseStateFlag: isBaseState
    onBaseStateFlagChanged: {
        evaluate();
    }
    onBackendColorChanged: {
        evaluate();
    }
    property variant isEnabled: colorGroupBox.enabled
    onIsEnabledChanged: {
        evaluate();
    }
    property bool isInModel: backendColor.isInModel;
    onIsInModelChanged: {
        evaluate();
    }
    property bool isInSubState: backendColor.isInSubState;
    onIsInSubStateChanged: {
        evaluate();
    }

    function evaluate() {
        if (backendColor === undefined)
            return;
        if (!enabled) {
            valueSpinBox.setStyleSheet("color: "+scheme.disabledColor);
            hueSpinBox.setStyleSheet("color: "+scheme.disabledColor);
            saturationSpinBox.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (baseStateFlag) {
                if (backendColor.isInModel) {
                    valueSpinBox.setStyleSheet("color: "+scheme.changedBaseColor);
                    hueSpinBox.setStyleSheet("color: "+scheme.changedBaseColor);
                    saturationSpinBox.setStyleSheet("color: "+scheme.changedBaseColor);
                    alphaSpinBox.setStyleSheet("color: "+scheme.changedBaseColor);
                } else {
                    valueSpinBox.setStyleSheet("color: "+scheme.defaultColor);
                    hueSpinBox.setStyleSheet("color: "+scheme.defaultColor);
                    saturationSpinBox.setStyleSheet("color: "+scheme.defaultColor);
                    alphaSpinBox.setStyleSheet("color: "+scheme.defaultColor);
                }
            } else {
                if (backendColor.isInSubState) {
                    valueSpinBox.setStyleSheet("color: "+scheme.changedStateColor);
                    hueSpinBox.setStyleSheet("color: "+scheme.changedStateColor);
                    saturationSpinBox.setStyleSheet("color: "+scheme.changedStateColor);
                    alphaSpinBox.setStyleSheet("color: "+scheme.changedStateColor);
                } else {
                    valueSpinBox.setStyleSheet("color: "+scheme.defaultColor);
                    hueSpinBox.setStyleSheet("color: "+scheme.defaultColor);
                    saturationSpinBox.setStyleSheet("color: "+scheme.defaultColor);
                    alphaSpinBox.setStyleSheet("color: "+scheme.defaultColor);
                }
            }
        }
    }

    ColorScheme { id:scheme; }

    QWidget {
        id: colorButtonWidget
        height: 28
        width: colorGroupBox.width
        layout: HorizontalLayout {
            topMargin: 0
            rightMargin: 2;

            Label {
                text: colorGroupBox.caption
                toolTip: colorGroupBox.caption
            }

            QWidget {
                layout: HorizontalLayout {
                    spacing: 6

                    LineEdit {
                        visible: gradientEditing == false
                        backendValue: colorGroupBox.backendColor
                        baseStateFlag: isBaseState
                    }

                    QWidget {
                        visible: gradientEditing == true
                        minimumHeight: 24;
                        id: lineEditWidget;
                        QLineEdit {
                            y: 2
                            text: color
                            width: lineEditWidget.width
                            height: lineEditWidget.height

                            onEditingFinished: {
                                color = text
                            }
                        }
                    }

                    ColorButton {
                        id: colorButton
                        color: colorGroupBox.color;
                        noColor: noneButtonChecked;
                        checkable: true;
                        checked: false;
                        fixedHeight: 22;
                        fixedWidth: 22;
                        width: fixedWidth
                        height: fixedHeight

                        onClicked: {
                            if (colorGroupBox.animated)
                                return;
                            if (checked) {
                                colorGroupBox.collapsed = false;
                                colorButtonWidget.visible = true;
                            } else {
                                colorGroupBox.collapsed = true;
                                colorButtonWidget.visible = true;
                            }
                        }

                        onToggled: {
                            if (colorGroupBox.animated)
                                return;
                            if (checked) {
                                colorGroupBox.collapsed = false;
                                colorButtonWidget.visible = true;
                            } else {
                                colorGroupBox.collapsed = true;
                                colorButtonWidget.visible = true;
                            }
                        }

                    }

                    ColorTypeButtons {
                        id: buttons;
                        visible: showButtons && baseStateFlag
                        showGradientButton: colorGroupBox.showGradientButton
                    }

                    QWidget {
                        visible: !(showButtons && baseStateFlag)
                        fixedHeight: 28
                        fixedWidth: 93
                        width: fixedWidth
                        height: fixedHeight
                    }
                }
            }

        }
    }


    layout: VerticalLayout {
        topMargin: 36

        QWidget {
            layout: HorizontalLayout {
                leftMargin: 12
                spacing: 0

                ColorBox {
                    id: colorControl;
                    property variant backendColor: colorGroupBox.color;
                    color: colorGroupBox.color;
                    onColorChanged: {
                        if (colorGroupBox.color != color) {
                            if (colorGroupBox.gradientEditing == true) {
                                colorGroupBox.color = color;
                            } else {
                                if (colorControl.alpha != 0)
                                    setSolidButtonChecked = true;
                                colorGroupBox.backendColor.value = color;
                            }
                        }
                    }
                }

                HueControl {
                    id: hueControl;
                    hue: colorControl.hue;
                    onHueChanged: if (colorControl.hue != hue) colorControl.hue=hue;
                }

                QWidget {
                    maximumWidth: 100
                    layout: VerticalLayout {
                        topMargin: 4
                        bottomMargin: 4
                        rightMargin: 0
                        leftMargin: 0
                        spacing: 2
                        QWidget {
                            layout: HorizontalLayout {
                                Label {
                                    text: "H"
                                    fixedWidth: 15
                                }

                                QSpinBox {
                                    id: hueSpinBox
                                    maximum: 359
                                    value: colorControl.hue;
                                    onValueChanged: if (colorControl.hue != value)
                                    colorControl.hue=value;
                                }

                            }
                        }
                        QWidget {
                            layout: HorizontalLayout {
                                Label {
                                    text: "S"
                                    fixedWidth: 15
                                }
                                QSpinBox {
                                    id: saturationSpinBox
                                    maximum: 255
                                    value: colorControl.saturation;
                                    onValueChanged: if (colorControl.saturation !=value)
                                    colorControl.saturation=value;
                                }
                            }

                        }

                        QWidget {
                            layout: HorizontalLayout {
                                Label {
                                    text: "B"
                                    fixedWidth: 15
                                }
                                QSpinBox {
                                    id: valueSpinBox
                                    maximum: 255
                                    value: colorControl.value;
                                    onValueChanged: if (Math.floor(colorControl.value)!=value)
                                    colorControl.value=value;
                                }
                            }
                        }

                        QWidget {
                            layout: HorizontalLayout {
                                topMargin: 12
                                Label {
                                    text: "A"
                                    fixedWidth: 15
                                }
                                QSpinBox {
                                    id: alphaSpinBox
                                    maximum: 255
                                    value: colorControl.alpha;
                                    onValueChanged: if (Math.floor(colorControl.alpha)!=value)
                                    colorControl.alpha=value;
                                }
                            }
                        }

                    }
                }
            }
        }
    }
}
