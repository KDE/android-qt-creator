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

#include "qmlpropertychangesnodeinstance.h"
#include "qmlstatenodeinstance.h"
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QDeclarativeExpression>
#include <private/qdeclarativebinding_p.h>
#include <QMutableListIterator>


#include <private/qdeclarativestate_p_p.h>
#include <private/qdeclarativepropertychanges_p.h>
#include <private/qdeclarativeproperty_p.h>

namespace QmlDesigner {
namespace Internal {

QmlPropertyChangesNodeInstance::QmlPropertyChangesNodeInstance(QDeclarativePropertyChanges *propertyChangesObject) :
        ObjectNodeInstance(propertyChangesObject)
{
}

QmlPropertyChangesNodeInstance::Pointer QmlPropertyChangesNodeInstance::create(QObject *object)
{
    QDeclarativePropertyChanges *propertyChange = qobject_cast<QDeclarativePropertyChanges*>(object);

    Q_ASSERT(propertyChange);

    Pointer instance(new QmlPropertyChangesNodeInstance(propertyChange));

    instance->populateResetHashes();

    return instance;
}

void QmlPropertyChangesNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    QMetaObject metaObject = QDeclarativePropertyChanges::staticMetaObject;

    if (metaObject.indexOfProperty(name.toLatin1()) > 0) { // 'restoreEntryValues', 'explicit'
        ObjectNodeInstance::setPropertyVariant(name, value);
    } else {
        changesObject()->changeValue(name.toLatin1(), value);
        QObject *targetObject = changesObject()->object();
        if (targetObject && nodeInstanceServer()->activeStateInstance().isWrappingThisObject(changesObject()->state())) {
            ServerNodeInstance targetInstance = nodeInstanceServer()->instanceForObject(targetObject);
            targetInstance.setPropertyVariant(name, value);
        }
    }
}

void QmlPropertyChangesNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    QMetaObject metaObject = QDeclarativePropertyChanges::staticMetaObject;

    if (metaObject.indexOfProperty(name.toLatin1()) > 0) { // 'restoreEntryValues', 'explicit'
        ObjectNodeInstance::setPropertyBinding(name, expression);
    } else {
        changesObject()->changeExpression(name.toLatin1(), expression);
    }
}

QVariant QmlPropertyChangesNodeInstance::property(const QString &name) const
{
    return changesObject()->property(name.toLatin1());
}

void QmlPropertyChangesNodeInstance::resetProperty(const QString &name)
{
    changesObject()->removeProperty(name.toLatin1());
}


void QmlPropertyChangesNodeInstance::reparent(const ServerNodeInstance &oldParentInstance, const QString &oldParentProperty, const ServerNodeInstance &newParentInstance, const QString &newParentProperty)
{
    changesObject()->detachFromState();

    ObjectNodeInstance::reparent(oldParentInstance.internalInstance(), oldParentProperty, newParentInstance.internalInstance(), newParentProperty);

    changesObject()->attachToState();
}

QDeclarativePropertyChanges *QmlPropertyChangesNodeInstance::changesObject() const
{
    Q_ASSERT(qobject_cast<QDeclarativePropertyChanges*>(object()));
    return static_cast<QDeclarativePropertyChanges*>(object());
}

} // namespace Internal
} // namespace QmlDesigner
