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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef GRADIENTLINE_H
#define GRADIENTLINE_H

#include <qmleditorwidgets_global.h>
#include <QtGui/QWidget>
#include <QtGui/QLinearGradient>

namespace QmlEditorWidgets {

class QMLEDITORWIDGETS_EXPORT GradientLine : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor activeColor READ activeColor WRITE setActiveColor NOTIFY activeColorChanged)
    Q_PROPERTY(QString gradientName READ gradientName WRITE setGradientName NOTIFY gradientNameChanged)
    Q_PROPERTY(bool active READ active WRITE setActive)
    Q_PROPERTY(QLinearGradient gradient READ gradient WRITE setGradient NOTIFY gradientChanged)

public:
    GradientLine(QWidget *parent = 0);

    QString gradientName() const { return m_gradientName; }
    void setGradientName(const QString &newName);
    QColor activeColor() const { return m_activeColor; }
    void setActiveColor(const QColor &newColor);
    bool active() const { return m_active; }
    void setActive(bool a) { m_active = a; }
    QLinearGradient gradient() const { return m_gradient; }
    void setGradient(const QLinearGradient &);

signals:
    void activeColorChanged();
    void itemNodeChanged();
    void gradientNameChanged();
    void gradientChanged();
    void openColorDialog(const QPoint &pos);
protected:
    bool event(QEvent *event);
    void keyPressEvent(QKeyEvent * event);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);

private:
    void setup();
    void readGradient();
    void updateGradient();
    int currentColorIndex() const { return m_colorIndex; }
    void setCurrentIndex(int i);

    QColor m_activeColor;
    QString m_gradientName;
    QList<QColor> m_colorList;
    QList<qreal> m_stops;
    int m_colorIndex;
    bool m_dragActive;
    QPoint m_dragStart;
    QLinearGradient m_gradient;
    int m_yOffset;
    bool m_create;
    bool m_active;
    bool m_dragOff;
    bool m_useGradient;

};

} //QmlEditorWidgets

#endif //GRADIENTLINE_H
