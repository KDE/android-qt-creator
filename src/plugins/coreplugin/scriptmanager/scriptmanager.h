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

#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H

#include <coreplugin/core_global.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QScriptEngine;
QT_END_NAMESPACE

namespace Core {

/* Script Manager.
 * Provides a script engine that is initialized with
 * Qt Creator's interfaces and allows for running scripts.
 * @{todo} Should it actually manage script files, too? */

class CORE_EXPORT ScriptManager : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<QScriptEngine> QScriptEnginePtr;

    // A stack frame as returned by a failed invocation (exception)
    // fileName may be empty. lineNumber can be 0 for the top frame (goof-up?).
    struct StackFrame {
        QString function;
        QString fileName;
        int lineNumber;
    };
    typedef QList<StackFrame> Stack;

    ScriptManager(QObject *parent = 0) : QObject(parent) {}
    virtual ~ScriptManager() { }

    // Run a script
    virtual bool runScript(const QString &script, QString *errorMessage, Stack *errorStack) = 0;
    virtual bool runScript(const QString &script, QString *errorMessage) = 0;

    virtual QScriptEnginePtr scriptEngine() = 0;
};

} // namespace Core

#endif // SCRIPTMANAGER_H
