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

#ifndef ACTIONMANAGER_H
#define ACTIONMANAGER_H

#include "coreplugin/core_global.h"
#include "coreplugin/uniqueidmanager.h"
#include "coreplugin/icontext.h"

#include <QtCore/QObject>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QAction;
class QShortcut;
class QString;
QT_END_NAMESPACE

namespace Core {

class ActionContainer;
class Command;

class CORE_EXPORT ActionManager : public QObject
{
    Q_OBJECT
public:
    ActionManager(QObject *parent = 0) : QObject(parent) {}
    virtual ~ActionManager() {}

    virtual ActionContainer *createMenu(const Id &id) = 0;
    virtual ActionContainer *createMenuBar(const Id &id) = 0;

    virtual Command *registerAction(QAction *action, const Id &id, const Context &context) = 0;
    virtual Command *registerShortcut(QShortcut *shortcut, const Id &id, const Context &context) = 0;

    virtual Command *command(const Id &id) const = 0;
    virtual ActionContainer *actionContainer(const Id &id) const = 0;

    virtual QList<Command *> commands() const = 0;

    virtual void unregisterAction(QAction *action, const Id &id) = 0;

signals:
    void commandListChanged();
};

} // namespace Core

#endif // ACTIONMANAGER_H
