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

#ifndef ORIGINWIDGET_H
#define ORIGINWIDGET_H

#include <QtGui/QWidget>

namespace QmlDesigner {

class OriginWidget: public QWidget {

Q_OBJECT

Q_PROPERTY(QString origin READ origin WRITE setOrigin NOTIFY originChanged)
Q_PROPERTY(bool marked READ marked WRITE setMarked)

public:
    OriginWidget(QWidget *parent = 0);

    QString origin() const
    { return m_originString; }

    void setOrigin(const QString& newOrigin);

    bool marked() const
    { return m_marked; }

    void setMarked(bool newMarked)
    { m_marked = newMarked; }

    static void registerDeclarativeType();

signals:
    void originChanged();

protected:
    void paintEvent(QPaintEvent *event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent * event);
    bool event(QEvent *event);
private:
    QString m_originString;
    bool m_pressed;
    int m_index;
    bool m_marked;
};

} // QmlDesigner


#endif //ORIGINWIDGET_H
