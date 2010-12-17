/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "basiclayouts.h"


QT_BEGIN_NAMESPACE

QBoxLayoutObject::QBoxLayoutObject(QObject *parent)
: QLayoutObject(parent), _layout(0)
{
}

QBoxLayoutObject::QBoxLayoutObject(QBoxLayout *layout, QObject *parent)
: QLayoutObject(parent), _layout(layout)
{
}

QLayout *QBoxLayoutObject::layout() const
{
    return _layout;
}

void QBoxLayoutObject::addWidget(QWidget *wid)
{
    _layout->addWidget(wid);
}

void QBoxLayoutObject::clearWidget()
{
}

QHBoxLayoutObject::QHBoxLayoutObject(QObject *parent)
: QBoxLayoutObject(new QHBoxLayout, parent)
{
}


QVBoxLayoutObject::QVBoxLayoutObject(QObject *parent)
: QBoxLayoutObject(new QVBoxLayout, parent)
{
}

QT_END_NAMESPACE

void BasicLayouts::registerDeclarativeTypes()
{
    qmlRegisterType<QBoxLayoutObject>("Bauhaus",1,0,"QBoxLayout");
    qmlRegisterType<QHBoxLayoutObject>("Bauhaus",1,0,"QHBoxLayout");
    qmlRegisterType<QVBoxLayoutObject>("Bauhaus",1,0,"QVBoxLayout");
}

