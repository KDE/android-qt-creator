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

#include "qmlv8debuggerclient.h"
#include "qmlv8debuggerclientconstants.h"
#include "debuggerstringutils.h"

#include "watchhandler.h"
#include "breakpoint.h"
#include "breakhandler.h"
#include "qmlengine.h"
#include "stackhandler.h"

#include <utils/qtcassert.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/basetexteditor.h>

#include <QtGui/QTextBlock>
#include <QtCore/QVariant>
#include <QtCore/QStack>
#include <QtCore/QQueue>
#include <QtCore/QFileInfo>
#include <QtGui/QTextDocument>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>

#define DEBUG_QML 0
#if DEBUG_QML
#   define SDEBUG(s) qDebug() << s << '\n'
#else
#   define SDEBUG(s)
#endif

using namespace Core;

namespace Debugger {
namespace Internal {

typedef QPair<QByteArray, QByteArray> WatchDataPair;

struct QmlV8ObjectData {
    QByteArray type;
    QVariant value;
    QVariant properties;
};

class QmlV8DebuggerClientPrivate
{
public:
    explicit QmlV8DebuggerClientPrivate(QmlV8DebuggerClient *q) :
        q(q),
        sequence(-1),
        engine(0),
        isOldService(false)
    {
        parser = m_scriptEngine.evaluate(_("JSON.parse"));
        stringifier = m_scriptEngine.evaluate(_("JSON.stringify"));
    }

    void connect();
    void disconnect();

    void interrupt();
    void continueDebugging(QmlV8DebuggerClient::StepAction stepAction, int stepCount = 1);

    void evaluate(const QString expr, bool global = false, bool disableBreak = false,
                  int frame = -1, bool addContext = false);
    void lookup(const QList<int> handles, bool includeSource = false);
    void backtrace(int fromFrame = -1, int toFrame = -1, bool bottom = false);
    void frame(int number = -1);
    void scope(int number = -1, int frameNumber = -1);
    void scopes(int frameNumber = -1);
    void scripts(int types = 4, const QList<int> ids = QList<int>(),
                 bool includeSource = false, const QVariant filter = QVariant());
    void source(int frame = -1, int fromLine = -1, int toLine = -1);

    void setBreakpoint(const QString type, const QString target, int line = -1,
                       int column = -1, bool enabled = true,
                       const QString condition = QString(), int ignoreCount = -1);
    void changeBreakpoint(int breakpoint, bool enabled = true,
                          const QString condition = QString(), int ignoreCount = -1);
    void clearBreakpoint(int breakpoint);
    void setExceptionBreak(QmlV8DebuggerClient::Exceptions type, bool enabled = false);
    void listBreakpoints();

    void v8flags(const QString flags);
    void version();
    //void profile(ProfileCommand command); //NOT SUPPORTED
    void gc();

    QmlV8ObjectData extractData(const QVariant &data);
    void clearCache();

    void logSendMessage(const QString &msg) const;
    void logReceiveMessage(const QString &msg) const;

    //TODO:: remove this method
    void reformatRequest(QByteArray &request);

private:
    QByteArray packMessage(const QByteArray &type, const QByteArray &message = QByteArray());
    QScriptValue initObject();


public:
    QmlV8DebuggerClient *q;

    int sequence;
    QmlEngine *engine;
    QHash<BreakpointModelId, int> breakpoints;
    QHash<int, BreakpointModelId> breakpointsSync;
    QList<int> breakpointsTemp;

    QScriptValue parser;
    QScriptValue stringifier;
    QStringList scriptSourceRequests;

    QHash<int, QString> evaluatingExpression;
    QHash<int, QByteArray> localsAndWatchers;

    //Cache
    QStringList watchedExpressions;
    QVariant refsVal;
    QList<int> currentFrameScopes;

    //TODO: remove this flag
    bool isOldService;
private:
    QScriptEngine m_scriptEngine;
};

///////////////////////////////////////////////////////////////////////
//
// QmlV8DebuggerClientPrivate
//
///////////////////////////////////////////////////////////////////////

void QmlV8DebuggerClientPrivate::connect()
{
    logSendMessage(QString(_("%1 %2")).arg(_(V8DEBUG), _(CONNECT)));
    if (isOldService) {
        //    { "seq"     : <number>,
        //      "type"    : "request",
        //      "command" : "connect",
        //    }
        QScriptValue jsonVal = initObject();
        jsonVal.setProperty(_(COMMAND), QScriptValue(_(CONNECT)));
        const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
        q->sendMessage(packMessage(QByteArray(), jsonMessage.toString().toUtf8()));
    } else {
        q->sendMessage(packMessage(CONNECT));
    }
}

void QmlV8DebuggerClientPrivate::disconnect()
{
    //    { "seq"     : <number>,
    //      "type"    : "request",
    //      "command" : "disconnect",
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(DISCONNECT)));

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2")).arg(_(V8DEBUG), jsonMessage.toString()));
    q->sendMessage(packMessage(DISCONNECT, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::interrupt()
{
    logSendMessage(QString(_("%1 %2")).arg(_(V8DEBUG), _(INTERRUPT)));
    if (isOldService) {
        //    { "seq"     : <number>,
        //      "type"    : "request",
        //      "command" : "interrupt",
        //    }
        QScriptValue jsonVal = initObject();
        jsonVal.setProperty(_(COMMAND), QScriptValue(_(INTERRUPT)));
        const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
        q->sendMessage(packMessage(QByteArray(), jsonMessage.toString().toUtf8()));

    } else {
        q->sendMessage(packMessage(INTERRUPT));
    }
}

void QmlV8DebuggerClientPrivate::continueDebugging(QmlV8DebuggerClient::StepAction action,
                                                   int count)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "continue",
    //      "arguments" : { "stepaction" : <"in", "next" or "out">,
    //                      "stepcount"  : <number of steps (default 1)>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND),
                        QScriptValue(_(CONTINEDEBUGGING)));

    if (action != QmlV8DebuggerClient::Continue) {
        QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
        switch (action) {
        case QmlV8DebuggerClient::In:
            args.setProperty(_(STEPACTION), QScriptValue(_(IN)));
            break;
        case QmlV8DebuggerClient::Out:
            args.setProperty(_(STEPACTION), QScriptValue(_(OUT)));
            break;
        case QmlV8DebuggerClient::Next:
            args.setProperty(_(STEPACTION), QScriptValue(_(NEXT)));
            break;
        default:break;
        }
        if (count != 1)
            args.setProperty(_(STEPCOUNT), QScriptValue(count));
        jsonVal.setProperty(_(ARGUMENTS), args);

    }
    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::evaluate(const QString expr, bool global,
                                          bool disableBreak, int frame,
                                          bool addContext)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "evaluate",
    //      "arguments" : { "expression"    : <expression to evaluate>,
    //                      "frame"         : <number>,
    //                      "global"        : <boolean>,
    //                      "disable_break" : <boolean>,
    //                      "additional_context" : [
    //                           { "name" : <name1>, "handle" : <handle1> },
    //                           { "name" : <name2>, "handle" : <handle2> },
    //                           ...
    //                      ]
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(EVALUATE)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
    args.setProperty(_(EXPRESSION), QScriptValue(expr));

    if (frame != -1)
        args.setProperty(_(FRAME), QScriptValue(frame));

    if (global)
        args.setProperty(_(GLOBAL), QScriptValue(global));

    if (disableBreak)
        args.setProperty(_(DISABLE_BREAK), QScriptValue(disableBreak));

    if (addContext) {
        QAbstractItemModel *localsModel = engine->localsModel();
        int rowCount = localsModel->rowCount();

        QScriptValue ctxtList = parser.call(QScriptValue(), QScriptValueList() << _(ARRAY  ));
        while (rowCount) {
            QModelIndex index = localsModel->index(--rowCount, 0);
            const WatchData *data = engine->watchHandler()->watchData(LocalsWatch, index);
            QScriptValue ctxt = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
            ctxt.setProperty(_(NAME), QScriptValue(data->name));
            ctxt.setProperty(_(HANDLE), QScriptValue(int(data->id)));

            ctxtList.setProperty(rowCount, ctxt);
        }

        args.setProperty(_(ADDITIONAL_CONTEXT), QScriptValue(ctxtList));
    }

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::lookup(QList<int> handles, bool includeSource)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "lookup",
    //      "arguments" : { "handles"       : <array of handles>,
    //                      "includeSource" : <boolean indicating whether
    //                                          the source will be included when
    //                                          script objects are returned>,
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(LOOKUP)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    QScriptValue array = parser.call(QScriptValue(), QScriptValueList() << _(ARRAY));
    int index = 0;
    foreach (int handle, handles) {
        array.setProperty(index++, QScriptValue(handle));
    }
    args.setProperty(_(HANDLES), array);

    if (includeSource)
        args.setProperty(_(INCLUDESOURCE), QScriptValue(includeSource));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::backtrace(int fromFrame, int toFrame, bool bottom)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "backtrace",
    //      "arguments" : { "fromFrame" : <number>
    //                      "toFrame" : <number>
    //                      "bottom" : <boolean, set to true if the bottom of the
    //                          stack is requested>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(BACKTRACE)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    if (fromFrame != -1)
        args.setProperty(_(FROMFRAME), QScriptValue(fromFrame));

    if (toFrame != -1)
        args.setProperty(_(TOFRAME), QScriptValue(toFrame));

    if (bottom)
        args.setProperty(_(BOTTOM), QScriptValue(bottom));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::frame(int number)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "frame",
    //      "arguments" : { "number" : <frame number>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(FRAME)));

    if (number != -1) {
        QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
        args.setProperty(_(NUMBER), QScriptValue(number));

        jsonVal.setProperty(_(ARGUMENTS), args);
    }

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::scope(int number, int frameNumber)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "scope",
    //      "arguments" : { "number" : <scope number>
    //                      "frameNumber" : <frame number, optional uses selected
    //                                      frame if missing>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(SCOPE)));

    if (number != -1) {
        QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
        args.setProperty(_(NUMBER), QScriptValue(number));

        if (frameNumber != -1)
            args.setProperty(_(FRAMENUMBER), QScriptValue(frameNumber));

        jsonVal.setProperty(_(ARGUMENTS), args);
    }

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::scopes(int frameNumber)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "scopes",
    //      "arguments" : { "frameNumber" : <frame number, optional uses selected
    //                                      frame if missing>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(SCOPES)));

    if (frameNumber != -1) {
        QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
        args.setProperty(_(FRAMENUMBER), QScriptValue(frameNumber));

        jsonVal.setProperty(_(ARGUMENTS), args);
    }

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::scripts(int types, const QList<int> ids, bool includeSource,
                                         const QVariant filter)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "scripts",
    //      "arguments" : { "types"         : <types of scripts to retrieve
    //                                           set bit 0 for native scripts
    //                                           set bit 1 for extension scripts
    //                                           set bit 2 for normal scripts
    //                                         (default is 4 for normal scripts)>
    //                      "ids"           : <array of id's of scripts to return. If this is not specified all scripts are requrned>
    //                      "includeSource" : <boolean indicating whether the source code should be included for the scripts returned>
    //                      "filter"        : <string or number: filter string or script id.
    //                                         If a number is specified, then only the script with the same number as its script id will be retrieved.
    //                                         If a string is specified, then only scripts whose names contain the filter string will be retrieved.>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(SCRIPTS)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
    args.setProperty(_(TYPES), QScriptValue(types));

    if (ids.count()) {
        QScriptValue array = parser.call(QScriptValue(), QScriptValueList() << _(ARRAY));
        int index = 0;
        foreach (int id, ids) {
            array.setProperty(index++, QScriptValue(id));
        }
        args.setProperty(_(IDS), array);
    }

    if (includeSource)
        args.setProperty(_(INCLUDESOURCE), QScriptValue(includeSource));

    QScriptValue filterValue;
    if (filter.type() == QVariant::String)
        filterValue = QScriptValue(filter.toString());
    else if (filter.type() == QVariant::Int)
        filterValue = QScriptValue(filter.toInt());
    else
        QTC_CHECK(!filter.isValid());

    args.setProperty(_(FILTER), filterValue);

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::source(int frame, int fromLine, int toLine)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "source",
    //      "arguments" : { "frame"    : <frame number (default selected frame)>
    //                      "fromLine" : <from line within the source default is line 0>
    //                      "toLine"   : <to line within the source this line is not included in
    //                                    the result default is the number of lines in the script>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(SOURCE)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    if (frame != -1)
        args.setProperty(_(FRAME), QScriptValue(frame));

    if (fromLine != -1)
        args.setProperty(_(FROMLINE), QScriptValue(fromLine));

    if (toLine != -1)
        args.setProperty(_(TOLINE), QScriptValue(toLine));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::setBreakpoint(const QString type, const QString target,
                                               int line, int column, bool enabled,
                                               const QString condition, int ignoreCount)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "setbreakpoint",
    //      "arguments" : { "type"        : <"function" or "script" or "scriptId" or "scriptRegExp">
    //                      "target"      : <function expression or script identification>
    //                      "line"        : <line in script or function>
    //                      "column"      : <character position within the line>
    //                      "enabled"     : <initial enabled state. True or false, default is true>
    //                      "condition"   : <string with break point condition>
    //                      "ignoreCount" : <number specifying the number of break point hits to ignore, default value is 0>
    //                    }
    //    }
    if (type == _(EVENT) && !isOldService) {
        QByteArray params;
        QDataStream rs(&params, QIODevice::WriteOnly);
        rs <<  target.toUtf8() << enabled;
        logSendMessage(QString(_("%1 %2 %3 %4")).arg(_(V8DEBUG), _(SIGNALHANDLER), target, enabled?_("enabled"):_("disabled")));
        q->sendMessage(packMessage(SIGNALHANDLER, params));

    } else {
        QScriptValue jsonVal = initObject();
        jsonVal.setProperty(_(COMMAND), QScriptValue(_(SETBREAKPOINT)));

        QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

        args.setProperty(_(TYPE), QScriptValue(type));
        args.setProperty(_(TARGET), QScriptValue(target));

        if (line != -1)
            args.setProperty(_(LINE), QScriptValue(line));

        if (column != -1)
            args.setProperty(_(COLUMN), QScriptValue(column));

        args.setProperty(_(ENABLED), QScriptValue(enabled));

        if (!condition.isEmpty())
            args.setProperty(_(CONDITION), QScriptValue(condition));

        if (ignoreCount != -1)
            args.setProperty(_(IGNORECOUNT), QScriptValue(ignoreCount));

        jsonVal.setProperty(_(ARGUMENTS), args);

        const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
        logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
        q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
    }
}

void QmlV8DebuggerClientPrivate::changeBreakpoint(int breakpoint, bool enabled,
                                                  const QString condition, int ignoreCount)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "changebreakpoint",
    //      "arguments" : { "breakpoint"  : <number of the break point to clear>
    //                      "enabled"     : <initial enabled state. True or false,
    //                                      default is true>
    //                      "condition"   : <string with break point condition>
    //                      "ignoreCount" : <number specifying the number of break point hits            }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND),
                        QScriptValue(_(CHANGEBREAKPOINT)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    args.setProperty(_(BREAKPOINT), QScriptValue(breakpoint));

    args.setProperty(_(ENABLED), QScriptValue(enabled));

    if (!condition.isEmpty())
        args.setProperty(_(CONDITION), QScriptValue(condition));

    if (ignoreCount != -1)
        args.setProperty(_(IGNORECOUNT), QScriptValue(ignoreCount));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::clearBreakpoint(int breakpoint)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "clearbreakpoint",
    //      "arguments" : { "breakpoint" : <number of the break point to clear>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND),
                        QScriptValue(_(CLEARBREAKPOINT)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    args.setProperty(_(BREAKPOINT), QScriptValue(breakpoint));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::setExceptionBreak(QmlV8DebuggerClient::Exceptions type,
                                                   bool enabled)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "setexceptionbreak",
    //      "arguments" : { "type"    : <string: "all", or "uncaught">,
    //                      "enabled" : <optional bool: enables the break type if true>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND),
                        QScriptValue(_(SETEXCEPTIONBREAK)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    if (type == QmlV8DebuggerClient::AllExceptions)
        args.setProperty(_(TYPE), QScriptValue(_(ALL)));
    //Not Supported
    //    else if (type == QmlV8DebuggerClient::UncaughtExceptions)
    //        args.setProperty(_(TYPE),QScriptValue(_(UNCAUGHT)));

    if (enabled)
        args.setProperty(_(ENABLED), QScriptValue(enabled));

    jsonVal.setProperty(_(ARGUMENTS), args);


    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::listBreakpoints()
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "listbreakpoints",
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND),
                        QScriptValue(_(LISTBREAKPOINTS)));

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::v8flags(const QString flags)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "v8flags",
    //      "arguments" : { "flags" : <string: a sequence of v8 flags just like
    //                                  those used on the command line>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(V8FLAGS)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    args.setProperty(_(FLAGS), QScriptValue(flags));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::version()
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "version",
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(VERSION)));

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

//void QmlV8DebuggerClientPrivate::profile(ProfileCommand command)
//{
////    { "seq"       : <number>,
////      "type"      : "request",
////      "command"   : "profile",
////      "arguments" : { "command"  : "resume" or "pause" }
////    }
//    QScriptValue jsonVal = initObject();
//    jsonVal.setProperty(_(COMMAND), QScriptValue(_(PROFILE)));

//    QScriptValue args = m_parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

//    if (command == Resume)
//        args.setProperty(_(COMMAND), QScriptValue(_(RESUME)));
//    else
//        args.setProperty(_(COMMAND), QScriptValue(_(PAUSE)));

//    args.setProperty(_("modules"), QScriptValue(1));
//    jsonVal.setProperty(_(ARGUMENTS), args);

//    const QScriptValue jsonMessage = m_stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
//    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
//    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
//}

void QmlV8DebuggerClientPrivate::gc()
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "gc",
    //      "arguments" : { "type" : <string: "all">,
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND),
                        QScriptValue(_(GARBAGECOLLECTOR)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    args.setProperty(_(TYPE), QScriptValue(_(ALL)));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

QmlV8ObjectData QmlV8DebuggerClientPrivate::extractData(const QVariant &data)
{
    //    { "handle" : <handle>,
    //      "type"   : <"undefined", "null", "boolean", "number", "string", "object", "function" or "frame">
    //    }

    //    {"handle":<handle>,"type":"undefined"}

    //    {"handle":<handle>,"type":"null"}

    //    { "handle":<handle>,
    //      "type"  : <"boolean", "number" or "string">
    //      "value" : <JSON encoded value>
    //    }

    //    {"handle":7,"type":"boolean","value":true}

    //    {"handle":8,"type":"number","value":42}

    //    { "handle"              : <handle>,
    //      "type"                : "object",
    //      "className"           : <Class name, ECMA-262 property [[Class]]>,
    //      "constructorFunction" : {"ref":<handle>},
    //      "protoObject"         : {"ref":<handle>},
    //      "prototypeObject"     : {"ref":<handle>},
    //      "properties" : [ {"name" : <name>,
    //                        "ref"  : <handle>
    //                       },
    //                       ...
    //                     ]
    //    }

    //        { "handle" : <handle>,
    //          "type"                : "function",
    //          "className"           : "Function",
    //          "constructorFunction" : {"ref":<handle>},
    //          "protoObject"         : {"ref":<handle>},
    //          "prototypeObject"     : {"ref":<handle>},
    //          "name"                : <function name>,
    //          "inferredName"        : <inferred function name for anonymous functions>
    //          "source"              : <function source>,
    //          "script"              : <reference to function script>,
    //          "scriptId"            : <id of function script>,
    //          "position"            : <function begin position in script>,
    //          "line"                : <function begin source line in script>,
    //          "column"              : <function begin source column in script>,
    //          "properties" : [ {"name" : <name>,
    //                            "ref"  : <handle>
    //                           },
    //                           ...
    //                         ]
    //        }

    QmlV8ObjectData objectData;
    const QVariantMap dataMap = data.toMap();
    QString type = dataMap.value(_(TYPE)).toString();

    if (type == _("undefined")) {
        objectData.type = QByteArray("undefined");
        objectData.value = QVariant(_("undefined"));

    } else if (type == _("null")) {
        objectData.type = QByteArray("null");
        objectData.value= QVariant(_("null"));

    } else if (type == _("boolean")) {
        objectData.type = QByteArray("boolean");
        objectData.value = dataMap.value(_(VALUE));

    } else if (type == _("number")) {
        objectData.type = QByteArray("number");
        objectData.value = dataMap.value(_(VALUE));

    } else if (type == _("string")) {
        objectData.type = QByteArray("string");
        objectData.value = dataMap.value(_(VALUE));

    } else if (type == _("object")) {
        objectData.type = QByteArray("object");
        objectData.value = dataMap.value(_("className"));
        objectData.properties = dataMap.value(_("properties"));

    } else if (type == _("function")) {
        objectData.type = QByteArray("function");
        objectData.value = dataMap.value(_(NAME));
        objectData.properties = dataMap.value(_("properties"));

    } else if (type == _("script")) {
        objectData.type = QByteArray("script");
        objectData.value = dataMap.value(_(NAME));
    }

    return objectData;
}

void QmlV8DebuggerClientPrivate::clearCache()
{
    watchedExpressions.clear();
    refsVal.clear();
    currentFrameScopes.clear();
}

QByteArray QmlV8DebuggerClientPrivate::packMessage(const QByteArray &type, const QByteArray &message)
{
    SDEBUG(message);
    QByteArray request;
    QDataStream rs(&request, QIODevice::WriteOnly);
    QByteArray cmd = V8DEBUG;
    rs << cmd;
    if (!isOldService)
        rs << type;
    rs << message;
    return request;
}

QScriptValue QmlV8DebuggerClientPrivate::initObject()
{
    QScriptValue jsonVal = parser.call(QScriptValue(),
                                       QScriptValueList() << QScriptValue(_(OBJECT)));
    jsonVal.setProperty(_(SEQ), QScriptValue(++sequence));
    jsonVal.setProperty(_(TYPE), _(REQUEST));
    return jsonVal;
}

void QmlV8DebuggerClientPrivate::logSendMessage(const QString &msg) const
{
    if (engine)
        engine->logMessage(QLatin1String("V8DebuggerClient"), QmlEngine::LogSend, msg);
}

void QmlV8DebuggerClientPrivate::logReceiveMessage(const QString &msg) const
{
    if (engine)
        engine->logMessage(QLatin1String("V8DebuggerClient"), QmlEngine::LogReceive, msg);
}

//TODO::remove this method
void QmlV8DebuggerClientPrivate::reformatRequest(QByteArray &request)
{
    QDataStream ds(request);
    QByteArray header;
    ds >> header;

    if (header == "V8DEBUG") {
        QByteArray command;
        QByteArray data;
        ds >> command >> data;

        if (command == INTERRUPT) {
            interrupt();

        } else if (command == V8REQUEST) {
            const QString requestString = QLatin1String(data);
            const QVariantMap reqMap = parser.call(QScriptValue(),
                                                    QScriptValueList() <<
                                                    QScriptValue(requestString)).toVariant().toMap();
            const QString debugCommand(reqMap.value(_(COMMAND)).toString());
            if (debugCommand == _(SETBREAKPOINT)) {
                QVariantMap arguments = reqMap.value(_(ARGUMENTS)).toMap();
                QString type(arguments.value(_(TYPE)).toString());
                if (type == _(SCRIPTREGEXP)) {
                    data.replace(SCRIPTREGEXP, SCRIPT);
                }
            }
            q->sendMessage(packMessage(QByteArray(), data));

        } else if (command == SIGNALHANDLER) {
            QDataStream rs(data);
            QByteArray signalHandler;
            bool enabled;
            rs >> signalHandler >> enabled;

            setBreakpoint(_(EVENT), QString::fromUtf8(signalHandler), -1, -1, enabled);
        }
    }
}

///////////////////////////////////////////////////////////////////////
//
// QmlV8DebuggerClient
//
///////////////////////////////////////////////////////////////////////

QmlV8DebuggerClient::QmlV8DebuggerClient(QmlJsDebugClient::QDeclarativeDebugConnection *client)
    : QmlDebuggerClient(client, QLatin1String("V8Debugger")),
      d(new QmlV8DebuggerClientPrivate(this))
{
}

QmlV8DebuggerClient::~QmlV8DebuggerClient()
{
    delete d;
}

void QmlV8DebuggerClient::startSession()
{
    //TODO:: remove this check.
    //Modify messages to align with the older protocol
    if (serviceVersion() < CURRENT_SUPPORTED_VERSION) {
        d->isOldService = true;
        //Modify cached messages
        QList<QByteArray> buffer = cachedMessages();
        clearCachedMessages();
        foreach (QByteArray msg, buffer) {
            d->reformatRequest(msg);
        }
    }
    flushSendBuffer();
    d->connect();
    //Query for the V8 version. This is
    //only for logging to the debuggerlog
    d->version();
}

void QmlV8DebuggerClient::endSession()
{
    d->disconnect();
}

void QmlV8DebuggerClient::executeStep()
{
    clearExceptionSelection();
    d->continueDebugging(In);
}

void QmlV8DebuggerClient::executeStepOut()
{
    clearExceptionSelection();
    d->continueDebugging(Out);
}

void QmlV8DebuggerClient::executeNext()
{
    clearExceptionSelection();
    d->continueDebugging(Next);
}

void QmlV8DebuggerClient::executeStepI()
{
    clearExceptionSelection();
    d->continueDebugging(In);
}

void QmlV8DebuggerClient::executeRunToLine(const ContextData &data)
{
    if (d->isOldService) {
        d->setBreakpoint(QString(_(SCRIPT)), QFileInfo(data.fileName).fileName(),
                         data.lineNumber - 1);
    } else {
        d->setBreakpoint(QString(_(SCRIPTREGEXP)), QFileInfo(data.fileName).fileName(),
                         data.lineNumber - 1);
    }
    clearExceptionSelection();
    d->continueDebugging(Continue);
}

void QmlV8DebuggerClient::continueInferior()
{
    clearExceptionSelection();
    d->continueDebugging(Continue);
}

void QmlV8DebuggerClient::interruptInferior()
{
    d->interrupt();
}

void QmlV8DebuggerClient::activateFrame(int index)
{
    if (index != d->engine->stackHandler()->currentIndex())
        d->frame(index);
}

bool QmlV8DebuggerClient::acceptsBreakpoint(const BreakpointModelId &id)
{
    BreakpointType type = d->engine->breakHandler()->breakpointData(id).type;
    return (type == BreakpointOnQmlSignalHandler
            || type == BreakpointByFileAndLine
            || type == BreakpointAtJavaScriptThrow);
}

void QmlV8DebuggerClient::insertBreakpoint(const BreakpointModelId &id)
{
    BreakHandler *handler = d->engine->breakHandler();
    const BreakpointParameters &params = handler->breakpointData(id);

    if (params.type == BreakpointAtJavaScriptThrow) {
        handler->notifyBreakpointInsertOk(id);
        d->setExceptionBreak(AllExceptions, params.enabled);

    } else if (params.type == BreakpointByFileAndLine) {
        if (d->isOldService) {
            d->setBreakpoint(QString(_(SCRIPT)), QFileInfo(params.fileName).fileName(),
                             params.lineNumber - 1, -1, params.enabled,
                             QLatin1String(params.condition), params.ignoreCount);
        } else {
            d->setBreakpoint(QString(_(SCRIPTREGEXP)), QFileInfo(params.fileName).fileName(),
                             params.lineNumber - 1, -1, params.enabled,
                             QLatin1String(params.condition), params.ignoreCount);
        }

    } else if (params.type == BreakpointOnQmlSignalHandler) {
        d->setBreakpoint(QString(_(EVENT)), params.functionName,
                         -1, -1, params.enabled);
        d->engine->breakHandler()->notifyBreakpointInsertOk(id);
    }

    d->breakpointsSync.insert(d->sequence, id);
}

void QmlV8DebuggerClient::removeBreakpoint(const BreakpointModelId &id)
{
    BreakHandler *handler = d->engine->breakHandler();

    int breakpoint = d->breakpoints.value(id);
    d->breakpoints.remove(id);

    if (handler->breakpointData(id).type == BreakpointAtJavaScriptThrow) {
        d->setExceptionBreak(AllExceptions);

    } else if (handler->breakpointData(id).type == BreakpointOnQmlSignalHandler) {
        d->setBreakpoint(QString(_(EVENT)), handler->breakpointData(id).functionName,
                         -1, -1, false);

    } else {
        d->clearBreakpoint(breakpoint);
    }
}

void QmlV8DebuggerClient::changeBreakpoint(const BreakpointModelId &id)
{
    BreakHandler *handler = d->engine->breakHandler();
    const BreakpointParameters &params = handler->breakpointData(id);

    if (params.type == BreakpointAtJavaScriptThrow) {
        d->setExceptionBreak(AllExceptions, params.enabled);

    } else if (handler->breakpointData(id).type == BreakpointOnQmlSignalHandler) {
        d->setBreakpoint(QString(_(EVENT)), params.functionName,
                         -1, -1, params.enabled);

    } else {
        int breakpoint = d->breakpoints.value(id);
        d->changeBreakpoint(breakpoint, params.enabled, QLatin1String(params.condition),
                            params.ignoreCount);
    }

    BreakpointResponse br = handler->response(id);
    br.enabled = params.enabled;
    br.condition = params.condition;
    br.ignoreCount = params.ignoreCount;
    handler->setResponse(id, br);
}

void QmlV8DebuggerClient::synchronizeBreakpoints()
{
    //NOT USED
}

void QmlV8DebuggerClient::assignValueInDebugger(const QByteArray /*expr*/, const quint64 &/*id*/,
                                                const QString &property, const QString &value)
{
    StackHandler *stackHandler = d->engine->stackHandler();
    QString expression = QString(_("%1 = %2;")).arg(property).arg(value);
    if (stackHandler->isContentsValid() && stackHandler->currentFrame().isUsable()) {
        d->evaluate(expression, false, false, stackHandler->currentIndex());
    } else {
        d->engine->showMessage(QString(_("Cannot evaluate %1 in current stack frame")).arg(expression), ScriptConsoleOutput);
    }
}

void QmlV8DebuggerClient::updateWatchData(const WatchData &/*data*/)
{
    //NOT USED
}

void QmlV8DebuggerClient::executeDebuggerCommand(const QString &command)
{
    StackHandler *stackHandler = d->engine->stackHandler();
    if (stackHandler->isContentsValid() && stackHandler->currentFrame().isUsable()) {
        d->evaluate(command, false, false, stackHandler->currentIndex());
        d->evaluatingExpression.insert(d->sequence, command);
    } else {
        //Currently cannot evaluate if not in a javascript break
        d->engine->showMessage(QString(_("Cannot evaluate %1 in current stack frame")).arg(command), ScriptConsoleOutput);
        //        d->evaluate(command);
    }
}

void QmlV8DebuggerClient::synchronizeWatchers(const QStringList &watchers)
{
    SDEBUG(watchers);
    foreach (const QString &exp, watchers) {
        if (!d->watchedExpressions.contains(exp)) {
            d->watchedExpressions << exp;
            StackHandler *stackHandler = d->engine->stackHandler();
            if (stackHandler->isContentsValid() && stackHandler->currentFrame().isUsable()) {
                d->evaluate(exp, false, false, stackHandler->currentIndex());
                d->evaluatingExpression.insert(d->sequence, exp);
            }
        }
    }
}

void QmlV8DebuggerClient::expandObject(const QByteArray &iname, quint64 objectId)
{
    d->localsAndWatchers.insert(objectId, iname);
    d->lookup(QList<int>() << objectId);
}

void QmlV8DebuggerClient::setEngine(QmlEngine *engine)
{
    d->engine = engine;
}

void QmlV8DebuggerClient::getSourceFiles()
{
    d->scripts(4, QList<int>(), true, QVariant());
}

void QmlV8DebuggerClient::messageReceived(const QByteArray &data)
{
    QDataStream ds(data);
    QByteArray command;
    ds >> command;

    if (command == V8DEBUG) {
        QByteArray type;
        QByteArray response;
        ds >> type >> response;

        if (d->isOldService) {
            response = type;
            type = QByteArray(V8MESSAGE);
        }

        d->logReceiveMessage(_(V8DEBUG) + QLatin1Char(' ') +  QLatin1String(type));
        if (type == CONNECT) {
            //debugging session started

        } else if (type == INTERRUPT) {
            //debug break requested

        } else if (type == SIGNALHANDLER) {
            //break on signal handler requested

        } else if (type == V8MESSAGE) {
            const QString responseString = QLatin1String(response);
            SDEBUG(responseString);
            d->logReceiveMessage(QLatin1String(V8MESSAGE) + QLatin1Char(' ') + responseString);

            const QVariantMap resp = d->parser.call(QScriptValue(),
                                                    QScriptValueList() <<
                                                    QScriptValue(responseString)).toVariant().toMap();
            const QString type(resp.value(_(TYPE)).toString());

            if (type == _("response")) {

                bool success = resp.value(_("success")).toBool();
                if (!success) {
                    SDEBUG("Request was unsuccessful");
                }

                const QString debugCommand(resp.value(_(COMMAND)).toString());

                if (debugCommand == _(DISCONNECT)) {
                    //debugging session ended

                } else if (debugCommand == _(CONTINEDEBUGGING)) {
                    //do nothing, wait for next break

                } else if (debugCommand == _(BACKTRACE)) {
                    if (success) {
                        updateStack(resp.value(_(BODY)), resp.value(_(REFS)));
                    }

                } else if (debugCommand == _(LOOKUP)) {
                    if (success) {
                        expandLocalsAndWatchers(resp.value(_(BODY)), resp.value(_(REFS)));
                    }

                } else if (debugCommand == _(EVALUATE)) {
                    int seq = resp.value(_("request_seq")).toInt();
                    if (success) {
                        updateEvaluationResult(seq, success, resp.value(_(BODY)), resp.value(_(REFS)));
                    } else {
                        QVariantMap map;
                        map.insert(_(TYPE), QVariant(_("string")));
                        map.insert(_(VALUE), resp.value(_("message")));
                        updateEvaluationResult(seq, success, QVariant(map), QVariant());
                    }

                } else if (debugCommand == _(LISTBREAKPOINTS)) {
                    if (success) {
                        updateBreakpoints(resp.value(_(BODY)));
                    }

                } else if (debugCommand == _(SETBREAKPOINT)) {
                    //                { "seq"         : <number>,
                    //                  "type"        : "response",
                    //                  "request_seq" : <number>,
                    //                  "command"     : "setbreakpoint",
                    //                  "body"        : { "type"       : <"function" or "script">
                    //                                    "breakpoint" : <break point number of the new break point>
                    //                                  }
                    //                  "running"     : <is the VM running after sending this response>
                    //                  "success"     : true
                    //                }

                    int seq = resp.value(_("request_seq")).toInt();
                    const QVariantMap breakpointData = resp.value(_(BODY)).toMap();
                    int index = breakpointData.value(_("breakpoint")).toInt();

                    if (d->breakpointsSync.contains(seq)) {
                        BreakpointModelId id = d->breakpointsSync.take(seq);
                        d->breakpoints.insert(id, index);

                        if (d->engine->breakHandler()->state(id) != BreakpointInserted)
                            d->engine->breakHandler()->notifyBreakpointInsertOk(id);

                    } else {
                        d->breakpointsTemp.append(index);
                    }


                } else if (debugCommand == _(CHANGEBREAKPOINT)) {
                    // DO NOTHING

                } else if (debugCommand == _(CLEARBREAKPOINT)) {
                    // DO NOTHING

                } else if (debugCommand == _(SETEXCEPTIONBREAK)) {
                    //                { "seq"               : <number>,
                    //                  "type"              : "response",
                    //                  "request_seq" : <number>,
                    //                  "command"     : "setexceptionbreak",
                    //                  "body"        : { "type"    : <string: "all" or "uncaught" corresponding to the request.>,
                    //                                    "enabled" : <bool: true if the break type is currently enabled as a result of the request>
                    //                                  }
                    //                  "running"     : true
                    //                  "success"     : true
                    //                }


                } else if (debugCommand == _(FRAME)) {
                    if (success) {
                        setCurrentFrameDetails(resp.value(_(BODY)), resp.value(_(REFS)));
                    }

                } else if (debugCommand == _(SCOPE)) {
                    if (success) {
                        updateScope(resp.value(_(BODY)), resp.value(_(REFS)));
                    }

                } else if (debugCommand == _(SCOPES)) {
                } else if (debugCommand == _(SOURCE)) {
                } else if (debugCommand == _(SCRIPTS)) {
                    //                { "seq"         : <number>,
                    //                  "type"        : "response",
                    //                  "request_seq" : <number>,
                    //                  "command"     : "scripts",
                    //                  "body"        : [ { "name"             : <name of the script>,
                    //                                      "id"               : <id of the script>
                    //                                      "lineOffset"       : <line offset within the containing resource>
                    //                                      "columnOffset"     : <column offset within the containing resource>
                    //                                      "lineCount"        : <number of lines in the script>
                    //                                      "data"             : <optional data object added through the API>
                    //                                      "source"           : <source of the script if includeSource was specified in the request>
                    //                                      "sourceStart"      : <first 80 characters of the script if includeSource was not specified in the request>
                    //                                      "sourceLength"     : <total length of the script in characters>
                    //                                      "scriptType"       : <script type (see request for values)>
                    //                                      "compilationType"  : < How was this script compiled:
                    //                                                               0 if script was compiled through the API
                    //                                                               1 if script was compiled through eval
                    //                                                            >
                    //                                      "evalFromScript"   : <if "compilationType" is 1 this is the script from where eval was called>
                    //                                      "evalFromLocation" : { line   : < if "compilationType" is 1 this is the line in the script from where eval was called>
                    //                                                             column : < if "compilationType" is 1 this is the column in the script from where eval was called>
                    //                                  ]
                    //                  "running"     : <is the VM running after sending this response>
                    //                  "success"     : true
                    //                }

                    if (success) {
                        const QVariantList body = resp.value(_(BODY)).toList();

                        QStringList sourceFiles;
                        for (int i = 0; i < body.size(); ++i) {
                            const QVariantMap entryMap = body.at(i).toMap();
                            const int lineOffset = entryMap.value(QLatin1String("lineOffset")).toInt();
                            const int columnOffset = entryMap.value(QLatin1String("columnOffset")).toInt();
                            const QString name = entryMap.value(QLatin1String("name")).toString();
                            const QString source = entryMap.value(QLatin1String("source")).toString();

                            if (name.isEmpty())
                                continue;

                            if (!sourceFiles.contains(name))
                                sourceFiles << name;

                            d->engine->updateScriptSource(name, lineOffset, columnOffset, source);
                        }
                        d->engine->setSourceFiles(sourceFiles);
                    }
                } else if (debugCommand == _(VERSION)) {
                    d->logReceiveMessage(QString(_("Using V8 Version: %1")).arg(
                                             resp.value(_(BODY)).toMap().
                                             value(_("V8Version")).toString()));

                } else if (debugCommand == _(V8FLAGS)) {
                } else if (debugCommand == _(GARBAGECOLLECTOR)) {
                } else {
                    // DO NOTHING
                }

            } else if (type == _(EVENT)) {
                const QString eventType(resp.value(_(EVENT)).toString());

                if (eventType == _("break")) {
                    const QVariantMap breakData = resp.value(_(BODY)).toMap();
                    const QString invocationText = breakData.value(_("invocationText")).toString();
                    const QString scriptUrl = breakData.value(_("script")).toMap().value(_("name")).toString();
                    const QString sourceLineText = breakData.value("sourceLineText").toString();

                    bool inferiorStop = true;

                    QList<int> v8BreakpointIds;
                    {
                        const QVariantList v8BreakpointIdList = breakData.value(_("breakpoints")).toList();
                        foreach (const QVariant &breakpointId, v8BreakpointIdList)
                            v8BreakpointIds << breakpointId.toInt();
                    }

                    if (!v8BreakpointIds.isEmpty() && invocationText.startsWith(_("[anonymous]()"))
                            && scriptUrl.endsWith(_(".qml"))
                            && sourceLineText.trimmed().startsWith(QLatin1Char('('))) {

                        // we hit most likely the anonymous wrapper function automatically generated for bindings
                        // -> relocate the breakpoint to column: 1 and continue

                        int newColumn = sourceLineText.indexOf(QLatin1Char('(')) + 1;
                        BreakHandler *handler = d->engine->breakHandler();

                        foreach (int v8Id, v8BreakpointIds) {
                            BreakpointModelId internalId;
                            foreach (const BreakpointModelId &id, d->breakpoints.keys()) {
                                if (d->breakpoints.value(id) == v8Id) {
                                    internalId = id;
                                    break;
                                }
                            }

                            if (internalId.isValid()) {
                                const BreakpointParameters &params = handler->breakpointData(internalId);

                                d->clearBreakpoint(v8Id);
                                const QString type = d->isOldService ? QString(_(SCRIPT)) :QString(_(SCRIPTREGEXP));
                                d->setBreakpoint(type, QFileInfo(params.fileName).fileName(),
                                                 params.lineNumber - 1, newColumn, params.enabled,
                                                 QString(params.condition), params.ignoreCount);
                                d->breakpointsSync.insert(d->sequence, internalId);
                            }
                        }
                        d->continueDebugging(Continue);
                        inferiorStop = false;
                    }

                    if (inferiorStop) {
                        //Update breakpoint data
                        BreakHandler *handler = d->engine->breakHandler();
                        foreach (int v8Id, v8BreakpointIds) {
                            foreach (const BreakpointModelId &id, d->breakpoints.keys()) {
                                if (d->breakpoints.value(id) == v8Id) {
                                    BreakpointResponse br = handler->response(id);
                                    if (br.functionName.isEmpty()) {
                                        br.functionName = invocationText;
                                        handler->setResponse(id, br);
                                    }
                                }
                            }
                        }

                        if (d->engine->state() == InferiorRunOk) {
                            foreach (const QVariant &breakpointId, v8BreakpointIds) {
                                if (d->breakpointsTemp.contains(breakpointId.toInt()))
                                    d->clearBreakpoint(breakpointId.toInt());
                            }
                            d->engine->inferiorSpontaneousStop();
                            d->backtrace();
                        } else if (d->engine->state() == InferiorStopOk) {
                            d->backtrace();
                        }
                    }

                } else if (eventType == _("exception")) {
                    const QVariantMap body = resp.value(_(BODY)).toMap();
                    int lineNumber = body.value(_("sourceLine")).toInt() + 1;

                    const QVariantMap script = body.value(_("script")).toMap();
                    QUrl fileUrl(script.value(_(NAME)).toString());
                    QString filePath = d->engine->toFileInProject(fileUrl);

                    const QVariantMap exception = body.value(_("exception")).toMap();
                    QString errorMessage = exception.value(_("text")).toString();

                    highlightExceptionCode(lineNumber, filePath, errorMessage);

                    if (d->engine->state() == InferiorRunOk) {
                        d->engine->inferiorSpontaneousStop();
                        d->backtrace();
                    }

                    if (d->engine->state() == InferiorStopOk) {
                        d->backtrace();
                    }

                } else if (eventType == _("afterCompile")) {
                    //Currently break point relocation is disabled.
                    //Uncomment the line below when it will be enabled.
//                    d->listBreakpoints();
                }

                //Sometimes we do not get event type!
                //This is most probably due to a wrong eval expression.
                //Redirect output to console.
                if (eventType.isEmpty()) {
                     bool success = resp.value(_("success")).toBool();
                     QVariantMap map;
                     map.insert(_(TYPE), QVariant(_("string")));
                     map.insert(_(VALUE), resp.value(_("message")));
                     //Since there is no sequence value, best estimate is
                     //last sequence value
                     updateEvaluationResult(d->sequence, success, QVariant(map), QVariant());
                }

            } //EVENT
        } //V8MESSAGE

    } else {
        //DO NOTHING
    }
}


void QmlV8DebuggerClient::updateStack(const QVariant &bodyVal, const QVariant &refsVal)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "backtrace",
    //      "body"        : { "fromFrame" : <number>
    //                        "toFrame" : <number>
    //                        "totalFrames" : <number>
    //                        "frames" : <array of frames - see frame request for details>
    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }

    const QVariantMap body = bodyVal.toMap();
    const QVariantList frames = body.value(_("frames")).toList();

    int fromFrameIndex = body.value(_("fromFrame")).toInt();

    QTC_ASSERT(0 == fromFrameIndex, return);

    StackHandler *stackHandler = d->engine->stackHandler();
    StackFrames stackFrames;
    foreach (const QVariant &frame, frames) {
        stackFrames << insertStackFrame(frame, refsVal);
    }
    stackHandler->setFrames(stackFrames);

    //Populate locals and watchers wrt top frame
    //Update all Locals visible in current scope
    //Traverse the scope chain and store the local properties
    //in a list and show them in the Locals Window.
    setCurrentFrameDetails(frames.value(0), refsVal);
}

StackFrame QmlV8DebuggerClient::insertStackFrame(const QVariant &bodyVal, const QVariant &refsVal)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "frame",
    //      "body"        : { "index"          : <frame number>,
    //                        "receiver"       : <frame receiver>,
    //                        "func"           : <function invoked>,
    //                        "script"         : <script for the function>,
    //                        "constructCall"  : <boolean indicating whether the function was called as constructor>,
    //                        "debuggerFrame"  : <boolean indicating whether this is an internal debugger frame>,
    //                        "arguments"      : [ { name: <name of the argument - missing of anonymous argument>,
    //                                               value: <value of the argument>
    //                                             },
    //                                             ... <the array contains all the arguments>
    //                                           ],
    //                        "locals"         : [ { name: <name of the local variable>,
    //                                               value: <value of the local variable>
    //                                             },
    //                                             ... <the array contains all the locals>
    //                                           ],
    //                        "position"       : <source position>,
    //                        "line"           : <source line>,
    //                        "column"         : <source column within the line>,
    //                        "sourceLineText" : <text for current source line>,
    //                        "scopes"         : [ <array of scopes, see scope request below for format> ],

    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }

    const QVariantMap body = bodyVal.toMap();

    StackFrame stackFrame;
    stackFrame.level = body.value(_("index")).toInt();

    QVariantMap func = body.value(_("func")).toMap();
    if (func.contains(_(REF))) {
        func = valueFromRef(func.value(_(REF)).toInt(), refsVal).toMap();
    }
    QString functionName = d->extractData(QVariant(func)).value.toString();
    if (functionName.isEmpty())
        functionName = tr("anonymous function");
    stackFrame.function = functionName;

    QVariantMap file = body.value(_("script")).toMap();
    if (file.contains(_(REF))) {
        file = valueFromRef(file.value(_(REF)).toInt(), refsVal).toMap();
    }
    stackFrame.file = d->engine->toFileInProject(d->extractData(QVariant(file)).value.toString());
    stackFrame.usable = QFileInfo(stackFrame.file).isReadable();

    QVariantMap receiver = body.value(_("receiver")).toMap();
    if (receiver.contains(_(REF))) {
        receiver = valueFromRef(receiver.value(_(REF)).toInt(), refsVal).toMap();
    }
    stackFrame.to = d->extractData(QVariant(receiver)).value.toString();

    stackFrame.line = body.value(_("line")).toInt() + 1;

    return stackFrame;
}

void QmlV8DebuggerClient::setCurrentFrameDetails(const QVariant &bodyVal, const QVariant &refsVal)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "frame",
    //      "body"        : { "index"          : <frame number>,
    //                        "receiver"       : <frame receiver>,
    //                        "func"           : <function invoked>,
    //                        "script"         : <script for the function>,
    //                        "constructCall"  : <boolean indicating whether the function was called as constructor>,
    //                        "debuggerFrame"  : <boolean indicating whether this is an internal debugger frame>,
    //                        "arguments"      : [ { name: <name of the argument - missing of anonymous argument>,
    //                                               value: <value of the argument>
    //                                             },
    //                                             ... <the array contains all the arguments>
    //                                           ],
    //                        "locals"         : [ { name: <name of the local variable>,
    //                                               value: <value of the local variable>
    //                                             },
    //                                             ... <the array contains all the locals>
    //                                           ],
    //                        "position"       : <source position>,
    //                        "line"           : <source line>,
    //                        "column"         : <source column within the line>,
    //                        "sourceLineText" : <text for current source line>,
    //                        "scopes"         : [ <array of scopes, see scope request below for format> ],

    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }
    QVariantMap currentFrame = bodyVal.toMap();

    StackHandler *stackHandler = d->engine->stackHandler();
    int frameIndex = currentFrame.value(QLatin1String("index")).toInt();

    d->clearCache();
    d->refsVal = refsVal;
    //Set "this" variable
    {
        WatchData data;
        data.exp = QByteArray("this");
        data.name = QLatin1String(data.exp);
        data.iname = QByteArray("local.") + data.exp;
        QVariantMap receiver = currentFrame.value(_("receiver")).toMap();
        if (receiver.contains(_(REF))) {
            receiver = valueFromRef(receiver.value(_(REF)).toInt(), refsVal).toMap();
        }
        data.id = receiver.value(_("handle")).toInt();
        QmlV8ObjectData receiverData = d->extractData(QVariant(receiver));
        data.type = receiverData.type;
        data.value = receiverData.value.toString();
        data.setHasChildren(receiverData.properties.toList().count());
        d->engine->watchHandler()->beginCycle();
        d->engine->watchHandler()->insertData(data);
        d->engine->watchHandler()->endCycle();
    }

    const QVariantList currentFrameScopes = currentFrame.value(_("scopes")).toList();
    foreach (const QVariant &scope, currentFrameScopes) {
        //Do not query for global types (0)
        //Showing global properties increases clutter.
        if (scope.toMap().value(_("type")).toInt() == 0)
            continue;
        int scopeIndex = scope.toMap().value(_("index")).toInt();
        d->currentFrameScopes.append(scopeIndex);
        d->scope(scopeIndex);
    }
    stackHandler->setCurrentIndex(frameIndex);
    d->engine->gotoLocation(stackHandler->currentFrame());
}

void QmlV8DebuggerClient::updateScope(const QVariant &bodyVal, const QVariant &refsVal)
{
//    { "seq"         : <number>,
//      "type"        : "response",
//      "request_seq" : <number>,
//      "command"     : "scope",
//      "body"        : { "index"      : <index of this scope in the scope chain. Index 0 is the top scope
//                                        and the global scope will always have the highest index for a
//                                        frame>,
//                        "frameIndex" : <index of the frame>,
//                        "type"       : <type of the scope:
//                                         0: Global
//                                         1: Local
//                                         2: With
//                                         3: Closure
//                                         4: Catch >,
//                        "object"     : <the scope object defining the content of the scope.
//                                        For local and closure scopes this is transient objects,
//                                        which has a negative handle value>
//                      }
//      "running"     : <is the VM running after sending this response>
//      "success"     : true
//    }
    QVariantMap bodyMap = bodyVal.toMap();

    //Check if the frameIndex is same as current Stack Index
    StackHandler *stackHandler = d->engine->stackHandler();
    if (bodyMap.value(_("frameIndex")).toInt() != stackHandler->currentIndex())
        return;

    QVariantMap object = bodyMap.value(_("object")).toMap();
    if (object.contains(_(REF))) {
        object = valueFromRef(object.value(_(REF)).toInt(),
                               refsVal).toMap();
    }

    const QVariantList properties = object.value(_("properties")).toList();

    QList<int> handlesToLookup;
    QList<WatchData> locals;
    foreach (const QVariant &property, properties) {
        QVariantMap localData = property.toMap();
        WatchData data;
        data.exp = localData.value(_(NAME)).toByteArray();
        //Check for v8 specific local data
        if (data.exp.startsWith('.') || data.exp.isEmpty())
            continue;

        data.name = QLatin1String(data.exp);
        data.iname = QByteArray("local.") + data.exp;

        int handle = localData.value(_(REF)).toInt();
        localData = valueFromRef(handle, d->refsVal).toMap();
        if (localData.isEmpty()) {
            handlesToLookup << handle;
            d->localsAndWatchers.insert(handle, data.exp);

        } else {
            data.id = localData.value(_(HANDLE)).toInt();

            QmlV8ObjectData objectData = d->extractData(QVariant(localData));
            data.type = objectData.type;
            data.value = objectData.value.toString();

            data.setHasChildren(objectData.properties.toList().count());

            locals << data;
        }
    }

    if (!handlesToLookup.isEmpty())
        d->lookup(handlesToLookup);

    if (!locals.isEmpty()) {
        d->engine->watchHandler()->beginCycle(false);
        d->engine->watchHandler()->insertBulkData(locals);
        d->engine->watchHandler()->endCycle();
    }
}

void QmlV8DebuggerClient::updateEvaluationResult(int sequence, bool success, const QVariant &bodyVal,
                                                 const QVariant &refsVal)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "evaluate",
    //      "body"        : ...
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }
    if (!d->evaluatingExpression.contains(sequence)) {
        //Update the locals
        foreach (int index, d->currentFrameScopes)
            d->scope(index);

    } else {
        QVariantMap bodyMap = bodyVal.toMap();
        if (bodyMap.contains(_(REF))) {
            bodyMap = valueFromRef(bodyMap.value(_(REF)).toInt(),
                                   refsVal).toMap();
        }

        QmlV8ObjectData body = d->extractData(QVariant(bodyMap));
        QString exp =  d->evaluatingExpression.take(sequence);
        if (d->watchedExpressions.contains(exp)) {
            QByteArray iname = d->engine->watchHandler()->watcherName(exp.toLatin1());
            SDEBUG(QString(iname));
            WatchData data;
            data.exp = exp.toLatin1();
            data.name = exp;
            data.iname = iname;
            data.id = bodyMap.value(_(HANDLE)).toInt();
            if (success) {
                data.type = body.type;
                data.value = body.value.toString();
                data.hasChildren = body.properties.toList().count();
            } else {
                //Do not set type since it is unknown
                data.setError(body.value.toString());
            }

            //Insert the newly evaluated expression to the Watchers Window
            d->engine->watchHandler()->beginCycle(false);
            d->engine->watchHandler()->insertData(data);
            d->engine->watchHandler()->endCycle();

        } else {
            d->engine->showMessage(body.value.toString(), ScriptConsoleOutput);
            //Update the locals
            foreach (int index, d->currentFrameScopes)
                d->scope(index);
        }
    }
}

void QmlV8DebuggerClient::updateBreakpoints(const QVariant &bodyVal)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "listbreakpoints",
    //      "body"        : { "breakpoints": [ { "type"             : <string: "scriptId"  or "scriptName".>,
    //                                           "script_id"        : <int: script id.  Only defined if type is scriptId.>,
    //                                           "script_name"      : <string: script name.  Only defined if type is scriptName.>,
    //                                           "number"           : <int: breakpoint number.  Starts from 1.>,
    //                                           "line"             : <int: line number of this breakpoint.  Starts from 0.>,
    //                                           "column"           : <int: column number of this breakpoint.  Starts from 0.>,
    //                                           "groupId"          : <int: group id of this breakpoint.>,
    //                                           "hit_count"        : <int: number of times this breakpoint has been hit.  Starts from 0.>,
    //                                           "active"           : <bool: true if this breakpoint is enabled.>,
    //                                           "ignoreCount"      : <int: remaining number of times to ignore breakpoint.  Starts from 0.>,
    //                                           "actual_locations" : <actual locations of the breakpoint.>,
    //                                         }
    //                                       ],
    //                        "breakOnExceptions"         : <true if break on all exceptions is enabled>,
    //                        "breakOnUncaughtExceptions" : <true if break on uncaught exceptions is enabled>
    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }

    const QVariantMap body = bodyVal.toMap();
    const QVariantList breakpoints = body.value(_("breakpoints")).toList();
    BreakHandler *handler = d->engine->breakHandler();

    foreach (const QVariant &breakpoint, breakpoints) {
        const QVariantMap breakpointData = breakpoint.toMap();

        int index = breakpointData.value(_("number")).toInt();
        BreakpointModelId id = d->breakpoints.key(index);
        BreakpointResponse br = handler->response(id);

        const QVariantList actualLocations = breakpointData.value(_("actual_locations")).toList();
        foreach (const QVariant &location, actualLocations) {
            const QVariantMap locationData = location.toMap();
            br.lineNumber = locationData.value(_("line")).toInt() + 1;;
            br.enabled = breakpointData.value(_("active")).toBool();
            br.hitCount = breakpointData.value(_("hit_count")).toInt();
            br.ignoreCount = breakpointData.value(_("ignoreCount")).toInt();

            handler->setResponse(id, br);
        }
    }
}

QVariant QmlV8DebuggerClient::valueFromRef(int handle, const QVariant &refsVal)
{
    QVariant variant;
    const QVariantList refs = refsVal.toList();
    foreach (const QVariant &ref, refs) {
        const QVariantMap refData = ref.toMap();
        if (refData.value(_(HANDLE)).toInt() == handle) {
            variant = refData;
            break;
        }
    }
    return variant;
}

void QmlV8DebuggerClient::expandLocalsAndWatchers(const QVariant &bodyVal, const QVariant &refsVal)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "lookup",
    //      "body"        : <array of serialized objects indexed using their handle>
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }
    const QVariantMap body = bodyVal.toMap();

    QList<WatchData> watchDataList;
    QStringList handlesList = body.keys();
    foreach (const QString &handle, handlesList) {
        QmlV8ObjectData bodyObjectData = d->extractData(
                    body.value(handle));
        QByteArray prepend = d->localsAndWatchers.take(handle.toInt());


        if (prepend.startsWith("local.") || prepend.startsWith("watch.")) {
            //Data for expanded local/watch
            if (bodyObjectData.properties.isValid()) {
                //Could be an object or function
                const WatchData *parent = d->engine->watchHandler()->findItem(prepend);
                const QVariantList properties = bodyObjectData.properties.toList();
                foreach (const QVariant &property, properties) {
                    QVariantMap propertyData = property.toMap();
                    WatchData data;
                    data.name = propertyData.value(_(NAME)).toString();

                    //Check for v8 specific local data
                    if (data.name.startsWith(QLatin1Char('.')) || data.name.isEmpty())
                        continue;
                    if (parent && parent->type == "object") {
                        if (parent->value == _("Array"))
                            data.exp = parent->exp + QByteArray("[") + data.name.toLatin1() + QByteArray("]");
                        else if (parent->value == _("Object"))
                            data.exp = parent->exp + QByteArray(".") + data.name.toLatin1();
                    } else {
                        data.exp = data.name.toLatin1();
                    }

                    data.iname = prepend + '.' + data.name.toLatin1();
                    propertyData = valueFromRef(propertyData.value(_(REF)).toInt(),
                                                refsVal).toMap();
                    data.id = propertyData.value(_(HANDLE)).toInt();

                    QmlV8ObjectData objectData = d->extractData(QVariant(propertyData));
                    data.type = objectData.type;
                    data.value = objectData.value.toString();

                    data.setHasChildren(objectData.properties.toList().count());
                    watchDataList << data;
                }
            }
        } else {
            //rest
            WatchData data;
            data.exp = prepend;
            data.name = QLatin1String(data.exp);
            data.iname = QByteArray("local.") + data.exp;
            data.id = handle.toInt();

            data.type = bodyObjectData.type;
            data.value = bodyObjectData.value.toString();

            data.setHasChildren(bodyObjectData.properties.toList().count());

            watchDataList << data;
        }
    }

    d->engine->watchHandler()->beginCycle(false);
    d->engine->watchHandler()->insertBulkData(watchDataList);
    d->engine->watchHandler()->endCycle();
}

void QmlV8DebuggerClient::highlightExceptionCode(int lineNumber,
                                                 const QString &filePath,
                                                 const QString &errorMessage)
{
    EditorManager *editorManager = EditorManager::instance();
    QList<IEditor *> openedEditors = editorManager->openedEditors();

    // set up the format for the errors
    QTextCharFormat errorFormat;
    errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    errorFormat.setUnderlineColor(Qt::red);

    foreach (IEditor *editor, openedEditors) {
        if (editor->file()->fileName() == filePath) {
            TextEditor::BaseTextEditorWidget *ed = qobject_cast<TextEditor::BaseTextEditorWidget *>(editor->widget());
            if (!ed)
                continue;

            QList<QTextEdit::ExtraSelection> selections;
            QTextEdit::ExtraSelection sel;
            sel.format = errorFormat;
            QTextCursor c(ed->document()->findBlockByNumber(lineNumber - 1));
            const QString text = c.block().text();
            for (int i = 0; i < text.size(); ++i) {
                if (! text.at(i).isSpace()) {
                    c.setPosition(c.position() + i);
                    break;
                }
            }
            c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            sel.cursor = c;

            sel.format.setToolTip(errorMessage);

            selections.append(sel);
            ed->setExtraSelections(TextEditor::BaseTextEditorWidget::DebuggerExceptionSelection, selections);

            QString message = QString(_("%1: %2: %3")).arg(filePath).arg(lineNumber)
                    .arg(errorMessage);
            d->engine->showMessage(message, ScriptConsoleOutput);
        }
    }
}

void QmlV8DebuggerClient::clearExceptionSelection()
{
    EditorManager *editorManager = EditorManager::instance();
    QList<IEditor *> openedEditors = editorManager->openedEditors();
    QList<QTextEdit::ExtraSelection> selections;

    foreach (IEditor *editor, openedEditors) {
        TextEditor::BaseTextEditorWidget *ed = qobject_cast<TextEditor::BaseTextEditorWidget *>(editor->widget());
        if (!ed)
            continue;

        ed->setExtraSelections(TextEditor::BaseTextEditorWidget::DebuggerExceptionSelection, selections);
    }

}

} // Internal
} // Debugger
