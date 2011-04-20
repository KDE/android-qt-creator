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
                value = valueFromBackend;
                readingFromBackend = false;
                evaluate();
            }

            onValueChanged: {                
                backendValue.value = value;
            }

            onFocusChanged: {
                if (focus)
                transaction.start();
                else
                transaction.end();
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
