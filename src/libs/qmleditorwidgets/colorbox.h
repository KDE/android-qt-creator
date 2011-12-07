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

#ifndef COLORBOX_H
#define COLORBOX_H

#include <qmleditorwidgets_global.h>
#include <QtGui/QWidget>
#include <QtDeclarative/qdeclarative.h>

namespace QmlEditorWidgets {

class QMLEDITORWIDGETS_EXPORT ColorBox : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString strColor READ strColor WRITE setStrColor NOTIFY colorChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int hue READ hue WRITE setHue NOTIFY hueChanged)
    Q_PROPERTY(int saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int alpha READ alpha WRITE setAlpha NOTIFY alphaChanged)

public:
    ColorBox(QWidget *parent = 0) : QWidget(parent), m_color(Qt::white), m_saturatedColor(Qt::white), m_lastHue(0)
    {
        setFixedWidth(130);
        setFixedHeight(130);
    }

    void setHue(int newHue);
    int hue() const;
    void setAlpha(int newAlpha);
    int alpha() const { return m_color.alpha(); }
    void setStrColor(const QString &colorStr);
    void setColor(const QColor &color);
    QString strColor() const;
    QColor color() const { return m_color; }
    int saturation() const { return m_color.hsvSaturation(); }
    void setSaturation(int newsaturation);
    int value() const { return m_color.value(); }
    void setValue(int newvalue);

signals:
    void colorChanged();
    void hueChanged();
    void saturationChanged();
    void valueChanged();
    void alphaChanged();

protected:
    void paintEvent(QPaintEvent *event);

    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void setCurrent(int x, int y);

private:
    QColor m_color;
    QColor m_saturatedColor;
    bool m_mousePressed;
    int m_lastHue;
    QPixmap m_cache;
};

} //QmlEditorWidgets

QML_DECLARE_TYPE(QmlEditorWidgets::ColorBox)

#endif //COLORBOX_H
