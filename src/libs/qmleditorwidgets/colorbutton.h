/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <qmleditorwidgets_global.h>
#include <QtGui/QToolButton>
#include <QtDeclarative/qdeclarative.h>

namespace QmlEditorWidgets {

class QMLEDITORWIDGETS_EXPORT ColorButton : public QToolButton {

Q_OBJECT

Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
Q_PROPERTY(bool noColor READ noColor WRITE setNoColor)
Q_PROPERTY(bool showArrow READ showArrow WRITE setShowArrow)

public:
    ColorButton(QWidget *parent = 0) : QToolButton (parent), m_colorString("#ffffff"), m_noColor(false), m_showArrow(true) {}

    void setColor(const QString &colorStr);
    QString color() const { return m_colorString; }
    QColor convertedColor() const;
    bool noColor() const { return m_noColor; }
    void setNoColor(bool f) { m_noColor = f; update(); }
    bool showArrow() const { return m_showArrow; }
    void setShowArrow(bool b) { m_showArrow = b; }

signals:
    void colorChanged();

protected:
    void paintEvent(QPaintEvent *event);
private:
    QString m_colorString;
    bool m_noColor;
    bool m_showArrow;
};

} //QmlEditorWidgets

QML_DECLARE_TYPE(QmlEditorWidgets::ColorButton)

#endif //COLORBUTTON_H
