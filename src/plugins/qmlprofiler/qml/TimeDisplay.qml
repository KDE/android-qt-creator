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

import QtQuick 1.0
import Monitor 1.0

Canvas2D {
    id: timeDisplay

    property variant startTime : 0
    property variant endTime : 0
    property variant timePerPixel: 0


    Component.onCompleted: {
        requestRedraw();
    }
    onWidthChanged: {
        requestRedraw();
    }
    onHeightChanged: {
        requestRedraw();
    }

    Connections {
        target: zoomControl
        onRangeChanged: {
            startTime = zoomControl.startTime();
            endTime = zoomControl.endTime();
            requestRedraw();
        }
    }

    onDrawRegion: {
        ctxt.fillStyle = "white";
        ctxt.fillRect(0, 0, width, height);

        var totalTime = endTime - startTime;
        var spacing = width / totalTime;

        var initialBlockLength = 120;
        var timePerBlock = Math.pow(2, Math.floor( Math.log( totalTime / width * initialBlockLength ) / Math.LN2 ) );
        var pixelsPerBlock = timePerBlock * spacing;
        var pixelsPerSection = pixelsPerBlock / 5;
        var blockCount = width / pixelsPerBlock;

        var realStartTime = Math.floor(startTime/timePerBlock) * timePerBlock;
        var realStartPos = (startTime-realStartTime) * spacing;

        timePerPixel = timePerBlock/pixelsPerBlock;

        var initialColor = Math.floor(realStartTime/timePerBlock) % 2;

        ctxt.fillStyle = "#000000";
        ctxt.font = "8px sans-serif";
        for (var ii = 0; ii < blockCount+1; ii++) {
            var x = Math.floor(ii*pixelsPerBlock - realStartPos);

            ctxt.fillStyle = (ii+initialColor)%2 ? "#E6E6E6":"white";
            ctxt.fillRect(x, 0, pixelsPerBlock, height);

            ctxt.strokeStyle = "#B0B0B0";
            ctxt.beginPath();
            ctxt.moveTo(x, 0);
            ctxt.lineTo(x, height);
            ctxt.stroke();

            ctxt.fillStyle = "#000000";
            ctxt.fillText(prettyPrintTime(ii*timePerBlock + realStartTime), x + 5, height/2 + 5);
        }

        ctxt.strokeStyle = "#525252";
        ctxt.beginPath();
        ctxt.moveTo(0, height-1);
        ctxt.lineTo(width, height-1);
        ctxt.stroke();

        // gradient borders
        var gradientDark = "rgba(0, 0, 0, 0.53125)";
        var gradientClear = "rgba(0, 0, 0, 0)";
        var grad = ctxt.createLinearGradient(0, 0, 0, 6);
        grad.addColorStop(0,gradientDark);
        grad.addColorStop(1,gradientClear);
        ctxt.fillStyle = grad;
        ctxt.fillRect(0, 0, width, 6);

        grad = ctxt.createLinearGradient(0, 0, 6, 0);
        grad.addColorStop(0,gradientDark);
        grad.addColorStop(1,gradientClear);
        ctxt.fillStyle = grad;
        ctxt.fillRect(0, 0, 6, height);

        grad = ctxt.createLinearGradient(width, 0, width-6, 0);
        grad.addColorStop(0,gradientDark);
        grad.addColorStop(1,gradientClear);
        ctxt.fillStyle = grad;
        ctxt.fillRect(width-6, 0, 6, height);
    }

    function prettyPrintTime( t )
    {
        if (t <= 0) return "0";
        if (t<1000) return t+" ns";
        t = t/1000;
        if (t<1000) return t+" μs";
        t = Math.floor(t/100)/10;
        if (t<1000) return t+" ms";
        t = Math.floor(t)/1000;
        if (t<60) return t+" s";
        var m = Math.floor(t/60);
        t = Math.floor(t - m*60);
        return m+"m"+t+"s";
    }
}
