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

import QtQuick 1.0
import "custom" as Components

Components.ProgressBar {
    id:progressbar

    property variant sizehint: backgroundItem.sizeFromContents(23, 23)
    property int orientation: Qt.Horizontal
    property string hint

    height: orientation === Qt.Horizontal ? sizehint.height : 200
    width: orientation === Qt.Horizontal ? 200 : sizehint.height

    background: QStyleItem {
        anchors.fill: parent
        elementType: "progressbar"
        // XXX: since desktop uses int instead of real, the progressbar
        // range [0..1] must be stretched to a good precision
        property int factor : 1000
        value:   indeterminate ? 0 : progressbar.value * factor // does indeterminate value need to be 1 on windows?
        minimum: indeterminate ? 0 : progressbar.minimumValue * factor
        maximum: indeterminate ? 0 : progressbar.maximumValue * factor
        enabled: progressbar.enabled
        horizontal: progressbar.orientation == Qt.Horizontal
        hint: progressbar.hint
    }
}

