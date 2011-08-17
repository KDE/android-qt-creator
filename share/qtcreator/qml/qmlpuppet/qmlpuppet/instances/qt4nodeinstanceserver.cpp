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

#include "qt4nodeinstanceserver.h"

#include "createscenecommand.h"

#include <QDeclarativeView>
#include <QDeclarativeEngine>
#include <QFileInfo>
#include <QGraphicsObject>
#include <private/qgraphicsitem_p.h>
#include <private/qgraphicsscene_p.h>
#include <QDeclarativeContext>

namespace QmlDesigner {

Qt4NodeInstanceServer::Qt4NodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient)
    : NodeInstanceServer(nodeInstanceClient)
{
    addImportString("import QtQuick 1.0\n");
}

Qt4NodeInstanceServer::~Qt4NodeInstanceServer()
{
    delete declarativeView();
}

QSGView *Qt4NodeInstanceServer::sgView() const
{
    return 0;
}

QDeclarativeView *Qt4NodeInstanceServer::declarativeView() const
{
    return m_declarativeView.data();
}

QDeclarativeEngine *Qt4NodeInstanceServer::engine() const
{
    if (declarativeView())
        return declarativeView()->engine();

    return 0;
}

void Qt4NodeInstanceServer::initializeView(const QVector<AddImportContainer> &/*importVector*/)
{
    Q_ASSERT(!declarativeView());

    m_declarativeView = new QDeclarativeView;
#ifndef Q_WS_MAC
    declarativeView()->setAttribute(Qt::WA_DontShowOnScreen, true);
#endif
    declarativeView()->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    declarativeView()->show();
#ifdef Q_WS_MAC
    declarativeView()->setAttribute(Qt::WA_DontShowOnScreen, true);
#endif
}

void Qt4NodeInstanceServer::resizeCanvasSizeToRootItemSize()
{
    QGraphicsObject *rootGraphicsObject = qobject_cast<QGraphicsObject*>(rootNodeInstance().internalObject());
    if (rootGraphicsObject) {
        declarativeView()->scene()->addItem(rootGraphicsObject);
        declarativeView()->setSceneRect(rootGraphicsObject->boundingRect());
    }
}

void Qt4NodeInstanceServer::resetAllItems()
{
    static_cast<QGraphicsScenePrivate*>(QObjectPrivate::get(declarativeView()->scene()))->processDirtyItemsEmitted = true;

    foreach (QGraphicsItem *item, declarativeView()->items()) {
        static_cast<QGraphicsScenePrivate*>(QObjectPrivate::get(declarativeView()->scene()))->resetDirtyItem(item);
    }
}

bool Qt4NodeInstanceServer::nonInstanceChildIsDirty(QGraphicsObject *graphicsObject) const
{
    QGraphicsItemPrivate *d = QGraphicsItemPrivate::get(graphicsObject);
    if (d->dirtyChildren) {
        foreach (QGraphicsItem *child, graphicsObject->childItems()) {
            QGraphicsObject *childGraphicsObject = child->toGraphicsObject();
            if (hasInstanceForObject(childGraphicsObject))
                continue;

            QGraphicsItemPrivate *childPrivate = QGraphicsItemPrivate::get(child);
            if (childPrivate->dirty || nonInstanceChildIsDirty(childGraphicsObject))
                return true;
        }
    }

    return false;
}

QList<ServerNodeInstance> Qt4NodeInstanceServer::setupScene(const CreateSceneCommand &command)
{
    setupFileUrl(command.fileUrl());
    setupImports(command.imports());
    setupDummyData(command.fileUrl());

    static_cast<QGraphicsScenePrivate*>(QObjectPrivate::get(declarativeView()->scene()))->processDirtyItemsEmitted = true;

    QList<ServerNodeInstance> instanceList = setupInstances(command);

    declarativeView()->scene()->setSceneRect(rootNodeInstance().boundingRect());

    return instanceList;
}

void Qt4NodeInstanceServer::refreshBindings()
{
    static int counter = 0;

    engine()->rootContext()->setContextProperty(QString("__%1").arg(counter++), 0); // refreshing bindings
}

void Qt4NodeInstanceServer::changeAuxiliaryValues(const ChangeAuxiliaryCommand &command)
{
    NodeInstanceServer::changeAuxiliaryValues(command);

    refreshScreenObject();
}

void Qt4NodeInstanceServer::changePropertyValues(const ChangeValuesCommand &command)
{
    NodeInstanceServer::changePropertyValues(command);

    refreshScreenObject();
}

void Qt4NodeInstanceServer::changePropertyBindings(const ChangeBindingsCommand &command)
{
    NodeInstanceServer::changePropertyBindings(command);

    refreshScreenObject();
}

void Qt4NodeInstanceServer::removeProperties(const RemovePropertiesCommand &command)
{
    NodeInstanceServer::removeProperties(command);

    refreshScreenObject();
}

void Qt4NodeInstanceServer::refreshScreenObject()
{
    if (declarativeView()) {
        QObject *screen = rootContext()->contextProperty("screen").value<QObject*>();
        if (screen) {
            screen->metaObject()->invokeMethod(screen,
                                               "privateSetDisplay",
                                               Q_ARG(int, rootNodeInstance().size().width()),
                                               Q_ARG(int, rootNodeInstance().size().height()),
                                               Q_ARG(qreal, 210)); // TODO: dpi should be setable
        }
    }
}
} // QmlDesigner
