/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include "imageview.h"

#include <QtCore/QFile>
#include <QtGui/QWheelEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QGraphicsRectItem>
#include <QtGui/QPixmap>
#ifndef QT_NO_SVG
#include <QtSvg/QGraphicsSvgItem>
#endif
#include <QtGui/QImageReader>
#include <qmath.h>


namespace ImageViewer {
namespace Constants {
    const qreal DEFAULT_SCALE_FACTOR = 1.2;
}

namespace Internal {

struct ImageViewPrivate
{
    ImageViewPrivate() : imageItem(0), backgroundItem(0), outlineItem(0) {}
    QGraphicsItem *imageItem;
    QGraphicsRectItem *backgroundItem;
    QGraphicsRectItem *outlineItem;
};

ImageView::ImageView(QWidget *parent)
    : QGraphicsView(parent),
    d(new ImageViewPrivate())
{
    setScene(new QGraphicsScene(this));
    setTransformationAnchor(AnchorUnderMouse);
    setDragMode(ScrollHandDrag);
    setViewportUpdateMode(FullViewportUpdate);
    setFrameShape(QFrame::NoFrame);
    setRenderHint(QPainter::SmoothPixmapTransform);

    // Prepare background check-board pattern
    QPixmap tilePixmap(64, 64);
    tilePixmap.fill(Qt::white);
    QPainter tilePainter(&tilePixmap);
    QColor color(220, 220, 220);
    tilePainter.fillRect(0, 0, 0x20, 0x20, color);
    tilePainter.fillRect(0x20, 0x20, 0x20, 0x20, color);
    tilePainter.end();

    setBackgroundBrush(tilePixmap);
}

ImageView::~ImageView()
{
    delete d;
}

void ImageView::drawBackground(QPainter *p, const QRectF &)
{
    p->save();
    p->resetTransform();
    p->drawTiledPixmap(viewport()->rect(), backgroundBrush().texture());
    p->restore();
}

bool ImageView::openFile(QString fileName)
{
#ifndef QT_NO_SVG
    bool isSvg = false;
#endif
    QByteArray format = QImageReader::imageFormat(fileName);

    // if it is impossible to recognize a file format - file will not be open correctly
    if (format.isEmpty())
        return false;

#ifndef QT_NO_SVG
    if (format.startsWith("svg"))
        isSvg = true;
#endif

    QGraphicsScene *s = scene();

    bool drawBackground = (d->backgroundItem ? d->backgroundItem->isVisible() : false);
    bool drawOutline = (d->outlineItem ? d->outlineItem->isVisible() : true);

    s->clear();
    resetTransform();

    // image
#ifndef QT_NO_SVG
    if (isSvg) {
        d->imageItem = new QGraphicsSvgItem(fileName);
    } else
#endif
    {
        QPixmap pixmap(fileName);
        QGraphicsPixmapItem *pixmapItem = new QGraphicsPixmapItem(pixmap);
        pixmapItem->setTransformationMode(Qt::SmoothTransformation);
        d->imageItem = pixmapItem;
    }
    d->imageItem->setCacheMode(QGraphicsItem::NoCache);
    d->imageItem->setZValue(0);

    // background item
    d->backgroundItem = new QGraphicsRectItem(d->imageItem->boundingRect());
    d->backgroundItem->setBrush(Qt::white);
    d->backgroundItem->setPen(Qt::NoPen);
    d->backgroundItem->setVisible(drawBackground);
    d->backgroundItem->setZValue(-1);

    // outline
    d->outlineItem = new QGraphicsRectItem(d->imageItem->boundingRect());
    QPen outline(Qt::black, 1, Qt::DashLine);
    outline.setCosmetic(true);
    d->outlineItem->setPen(outline);
    d->outlineItem->setBrush(Qt::NoBrush);
    d->outlineItem->setVisible(drawOutline);
    d->outlineItem->setZValue(1);

    s->addItem(d->backgroundItem);
    s->addItem(d->imageItem);
    s->addItem(d->outlineItem);

    // if image size is 0x0, then it is not loaded
    if (d->imageItem->boundingRect().height() == 0 && d->imageItem->boundingRect().width() == 0)
        return false;
    emitScaleFactor();

    return true;
}

void ImageView::setViewBackground(bool enable)
{
    if (!d->backgroundItem)
          return;

    d->backgroundItem->setVisible(enable);
}

void ImageView::setViewOutline(bool enable)
{
    if (!d->outlineItem)
        return;

    d->outlineItem->setVisible(enable);
}

void ImageView::doScale(qreal factor)
{
    scale(factor, factor);
    emitScaleFactor();
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    qreal factor = qPow(Constants::DEFAULT_SCALE_FACTOR, event->delta() / 240.0);
    doScale(factor);
    event->accept();
}

void ImageView::zoomIn()
{
    doScale(Constants::DEFAULT_SCALE_FACTOR);
}

void ImageView::zoomOut()
{
    doScale(1. / Constants::DEFAULT_SCALE_FACTOR);
}

void ImageView::resetToOriginalSize()
{
    resetTransform();
    emitScaleFactor();
}

void ImageView::fitToScreen()
{
    fitInView(d->imageItem, Qt::KeepAspectRatio);
    emitScaleFactor();
}

void ImageView::emitScaleFactor()
{
    // get scale factor directly from the transform matrix
    qreal factor = transform().m11();
    emit scaleFactorChanged(factor);
}

} // namespace Internal
} // namespace ImageView
