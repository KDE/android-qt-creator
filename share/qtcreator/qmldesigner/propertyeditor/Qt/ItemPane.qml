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

PropertyFrame {
    id: frame;
    x: 0
    y: 0

    ExpressionEditor {
        id: expressionEdit
    }
    layout: QVBoxLayout {
        topMargin: 0;
        bottomMargin: 0;
        leftMargin: 0;
        rightMargin: 0;
        spacing: 0;

        Type {
        }

        Geometry {
        }

        Visibility {

        }

        HorizontalWhiteLine {
            maximumHeight: 4;
            styleSheet: "QLineEdit {border: 2px solid #707070; min-height: 0px; max-height: 0px;}";
        }

        Switches {
        }

        HorizontalWhiteLine {
        }

        ScrollArea {
            styleSheetFile: ":/qmldesigner/scrollbar.css";
            widgetResizable: true;

            finished: finishedNotify;

            horizontalScrollBarPolicy: "Qt::ScrollBarAlwaysOff";
            id: standardPane;
            QFrame {
                //minimumHeight: 1100
                id: properyEditorStandard
                layout: QVBoxLayout {
                    topMargin: 0;
                    bottomMargin: 0;
                    leftMargin: 0;
                    rightMargin: 0;
                    spacing: 0;

                    WidgetLoader {
                        id: specificsTwo;
                        baseUrl: globalBaseUrl;
                        qmlData: specificQmlData;
                    }

                    WidgetLoader {
                        id: specificsOne;
                        source: specificsUrl;
                    }

                    QScrollArea {
                    }
                } // layout
            } //QWidget
        } //QScrollArea
        ExtendedPane {
            id: extendedPane;
        }
        LayoutPane {
            id: layoutPane;
        }
    }
}
