/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "dummynodeinstance.h"

#include <nodemetainfo.h>

#include <invalidnodeinstanceexception.h>

namespace QmlDesigner {
namespace Internal {

DummyNodeInstance::DummyNodeInstance()
   : ObjectNodeInstance(new QObject)
{
}

DummyNodeInstance::Pointer DummyNodeInstance::create()
{
    return Pointer(new DummyNodeInstance);
}

void DummyNodeInstance::paint(QPainter * /*painter*/) const
{
}

QRectF DummyNodeInstance::boundingRect() const
{
    return QRectF();
}

QPointF DummyNodeInstance::position() const
{
    return QPointF();
}

QSizeF DummyNodeInstance::size() const
{
    return QSizeF();
}

QTransform DummyNodeInstance::transform() const
{
    return QTransform();
}

double DummyNodeInstance::opacity() const
{
    return 0.0;
}

void DummyNodeInstance::setPropertyVariant(const QString &/*name*/, const QVariant &/*value*/)
{
}

QVariant DummyNodeInstance::property(const QString &/*name*/) const
{
    return QVariant();
}

QStringList DummyNodeInstance::properties()
{
    return QStringList();
}

QStringList DummyNodeInstance::localProperties()
{
    return QStringList();
}

void DummyNodeInstance::initializePropertyWatcher(const ObjectNodeInstance::Pointer &/*objectNodeInstance*/)
{

}

} // namespace Internal
} // namespace QmlDesigner
