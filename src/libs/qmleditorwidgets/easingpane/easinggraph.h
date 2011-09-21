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

#ifndef EASINGGRAPH_H
#define EASINGGRAPH_H

#include <QtGui/QWidget>
#include <QtCore/QEasingCurve>
#include <QtCore/QHash>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)


class EasingGraph: public QWidget
{
    Q_OBJECT

    Q_PROPERTY (QString easingShape READ easingShape WRITE setEasingShape NOTIFY easingShapeChanged)
    Q_PROPERTY (QString easingExtremes READ easingExtremes WRITE setEasingExtremes NOTIFY easingExtremesChanged)
    Q_PROPERTY (QString easingName READ easingName WRITE setEasingName NOTIFY easingNameChanged)
    Q_PROPERTY (qreal overshoot READ overshoot WRITE setOvershoot NOTIFY overshootChanged)
    Q_PROPERTY (qreal amplitude READ amplitude WRITE setAmplitude NOTIFY amplitudeChanged)
    Q_PROPERTY (qreal period READ period WRITE setPeriod NOTIFY periodChanged)
    Q_PROPERTY (qreal duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY (QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY (QColor zeroColor READ zeroColor WRITE setZeroColor NOTIFY zeroColorChanged)

public:
    EasingGraph(QWidget *parent=0);
    ~EasingGraph();

    QEasingCurve::Type easingType() const;
    QEasingCurve easingCurve() const;

    QString easingShape() const;
    void setEasingShape(const QString &newShape);
    QString easingExtremes() const;
    void setEasingExtremes(const QString &newExtremes);
    QString easingName() const;
    void setEasingName(const QString &newName);
    qreal overshoot() const;
    void setOvershoot(qreal newOvershoot);
    qreal amplitude() const;
    void setAmplitude(qreal newAmplitude);
    qreal period() const;
    void setPeriod(qreal newPeriod);
    qreal duration() const;
    void setDuration(qreal newDuration);

    QColor color() const;
    void setColor(const QColor &newColor);
    QColor zeroColor() const;
    void setZeroColor(const QColor &newColor);

    QRectF boundingRect() const;
    //void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
    void paintEvent(QPaintEvent *);

signals:
    void easingShapeChanged();
    void easingExtremesChanged();
    void easingNameChanged();
    void overshootChanged();
    void amplitudeChanged();
    void durationChanged();
    void periodChanged();
    void colorChanged();
    void zeroColorChanged();

private:
    QColor m_color;
    QColor m_zeroColor; // the color for the "zero" and "one" lines
    qreal m_duration;

    QString m_easingExtremes;

    QEasingCurve m_curveFunction;
    QHash <QString,QEasingCurve::Type> m_availableNames;
};

QT_END_NAMESPACE

//QML_DECLARE_TYPE(EasingGraph)

QT_END_HEADER

#endif // EASINGGRAPH_H
