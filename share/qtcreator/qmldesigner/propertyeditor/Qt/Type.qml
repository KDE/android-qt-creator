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

GroupBox {
    id: type;
    finished: finishedNotify;
    caption: qsTr("Type")

    layout: VerticalLayout {
        spacing: 6
        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: qsTr("Type")
                    windowTextColor: isBaseState ? "#000000" : "#FF0000";
                }

                QLabel {
                    text: backendValues.className.value;
                }

            }
        }
        QWidget {
            property variant isEnabled: isBaseState
            onIsEnabledChanged: idLineEdit.setStyleSheet("color: "+(isEnabled?scheme.defaultColor:scheme.disabledColor));
            ColorScheme{ id:scheme }

            layout: HorizontalLayout {
                Label {
                    text: qsTr("Id");
                }

                QLineEdit {
                    enabled: isBaseState
                    id: idLineEdit;
                    objectName: "idLineEdit";
                    readOnly: isBaseState != true;
                    text: backendValues.id.value;
                    onEditingFinished: {
                        backendValues.id.value = text;
                    }
                }
            }
        }
    }
}
