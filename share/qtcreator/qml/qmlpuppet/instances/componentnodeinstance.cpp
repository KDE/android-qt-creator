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

#include "componentnodeinstance.h"

#include <QDeclarativeComponent>
#include <QDeclarativeContext>

#include <QtDebug>

namespace QmlDesigner {
namespace Internal {


ComponentNodeInstance::ComponentNodeInstance(QDeclarativeComponent *component)
   : ObjectNodeInstance(component)
{
}

QDeclarativeComponent *ComponentNodeInstance::component() const
{
    Q_ASSERT(qobject_cast<QDeclarativeComponent*>(object()));
    return static_cast<QDeclarativeComponent*>(object());
}

ComponentNodeInstance::Pointer ComponentNodeInstance::create(QObject  *object)
{
    QDeclarativeComponent *component = qobject_cast<QDeclarativeComponent *>(object);

    Q_ASSERT(component);

    Pointer instance(new ComponentNodeInstance(component));

    instance->populateResetValueHash();

    return instance;
}

bool ComponentNodeInstance::hasContent() const
{
    return true;
}

void ComponentNodeInstance::setNodeSource(const QString &source)
{
    QByteArray importArray;
    foreach (const QString &import, nodeInstanceServer()->imports()) {
        importArray.append(import.toUtf8());
    }

    QByteArray data(source.toUtf8());

    data.prepend(importArray);
    data.append("\n");

    component()->setData(data, QUrl(nodeInstanceServer()->fileUrl().toString() +
                                    QLatin1Char('_')+ id()));
    setId(id());

    if (component()->isError()) {
        qDebug() << source;
        foreach(const QDeclarativeError &error, component()->errors())
            qDebug() << error;
    }

}

} // Internal
} // QmlDesigner
