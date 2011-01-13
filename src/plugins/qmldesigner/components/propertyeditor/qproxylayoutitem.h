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

#ifndef QPROXYLAYOUTITEM_H
#define QPROXYLAYOUTITEM_H

#include <qdeclarative.h>
#include <QGraphicsLayout>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QProxyLayout : public QObject, public QGraphicsLayout
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsLayout QGraphicsLayoutItem)
    Q_PROPERTY(QGraphicsLayout *layout READ layout WRITE setLayout)
public:
    QProxyLayout(QObject *parent=0);

    void setLayout(QGraphicsLayout *);
    QGraphicsLayout *layout() const;

    virtual void setGeometry(const QRectF &);
    virtual int count() const;
    virtual QGraphicsLayoutItem *itemAt(int) const;
    virtual void removeAt(int);
    virtual void updateGeometry();

protected:
    virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint) const;

private:
    QRectF geometry;
    QGraphicsLayout *proxy;
};

class QProxyLayoutItem : public QObject, public QGraphicsLayoutItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsLayoutItem)
    Q_PROPERTY(QGraphicsLayoutItem *widget READ item WRITE setItem)
public:
    QProxyLayoutItem(QGraphicsLayoutItem * = 0);

    virtual void setGeometry(const QRectF &);

    QGraphicsLayoutItem *item() const;
    void setItem(QGraphicsLayoutItem *);

    void setEnabled(bool);

    static void registerDeclarativeTypes();

protected:
    virtual QSizeF sizeHint(Qt::SizeHint, const QSizeF &) const;

private:
    bool enabled;
    QRectF geometry;
    QGraphicsLayoutItem *other;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QProxyLayout)
QML_DECLARE_TYPE(QProxyLayoutItem)

QT_END_HEADER

#endif // QPROXYLAYOUTITEM_H

