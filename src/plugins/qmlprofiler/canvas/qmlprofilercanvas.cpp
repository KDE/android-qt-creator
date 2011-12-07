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

#include "qmlprofilercanvas.h"

#include "qdeclarativecontext2d_p.h"

#include <QtGui/qpixmap.h>
#include <QtGui/qpainter.h>

namespace QmlProfiler {
namespace Internal {

QmlProfilerCanvas::QmlProfilerCanvas()
    : m_context2d(new Context2D(this))
{
    setFlag(QGraphicsItem::ItemHasNoContents, false);
    setAcceptedMouseButtons(Qt::LeftButton);
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

void QmlProfilerCanvas::requestPaint()
{
    update();
}

void QmlProfilerCanvas::requestRedraw()
{
    setDirty(true);
    update();
}

void QmlProfilerCanvas::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (m_context2d->size().width() != width() || m_context2d->size().height() != height()) {
        m_dirty = true;
        m_context2d->setSize(width(), height());
    }

    if (m_dirty) {
        m_context2d->reset();

        emit drawRegion(m_context2d, QRect(0, 0, width(), height()));
        setDirty(false);
    }

    p->drawPixmap(0, 0, m_context2d->pixmap());
}

void QmlProfilerCanvas::componentComplete()
{
    const QMetaObject *metaObject = this->metaObject();
    int propertyCount = metaObject->propertyCount();
    int requestPaintMethod = metaObject->indexOfMethod("requestPaint()");
    for (int ii = QmlProfilerCanvas::staticMetaObject.propertyCount(); ii < propertyCount; ++ii) {
        QMetaProperty p = metaObject->property(ii);
        if (p.hasNotifySignal())
            QMetaObject::connect(this, p.notifySignalIndex(), this, requestPaintMethod, 0, 0);
    }
    QDeclarativeItem::componentComplete();
}

}
}
