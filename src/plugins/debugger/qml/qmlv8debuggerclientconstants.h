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

#ifndef QMLV8DEBUGGERCLIENTCONSTANTS_H
#define QMLV8DEBUGGERCLIENTCONSTANTS_H

namespace Debugger {
namespace Internal {

const char V8DEBUG[] = "V8DEBUG";
const char SEQ[] = "seq";
const char TYPE[] = "type";
const char COMMAND[] = "command";
const char ARGUMENTS[] = "arguments";
const char STEPACTION[] = "stepaction";
const char STEPCOUNT[] = "stepcount";
const char EXPRESSION[] = "expression";
const char FRAME[] = "frame";
const char GLOBAL[] = "global";
const char DISABLE_BREAK[] = "disable_break";
const char ADDITIONAL_CONTEXT[] = "additional_context";
const char HANDLES[] = "handles";
const char INCLUDESOURCE[] = "includeSource";
const char FROMFRAME[] = "fromFrame";
const char TOFRAME[] = "toFrame";
const char BOTTOM[] = "bottom";
const char NUMBER[] = "number";
const char FRAMENUMBER[] = "frameNumber";
const char TYPES[] = "types";
const char IDS[] = "ids";
const char FILTER[] = "filter";
const char FROMLINE[] = "fromLine";
const char TOLINE[] = "toLine";
const char TARGET[] = "target";
const char LINE[] = "line";
const char COLUMN[] = "column";
const char ENABLED[] = "enabled";
const char CONDITION[] = "condition";
const char IGNORECOUNT[] = "ignoreCount";
const char BREAKPOINT[] = "breakpoint";
const char FLAGS[] = "flags";

const char CONTINEDEBUGGING[] = "continue";
const char EVALUATE[] = "evaluate";
const char LOOKUP[] = "lookup";
const char BACKTRACE[] = "backtrace";
const char SCOPE[] = "scope";
const char SCOPES[] = "scopes";
const char SCRIPTS[] = "scripts";
const char SOURCE[] = "source";
const char SETBREAKPOINT[] = "setbreakpoint";
const char CHANGEBREAKPOINT[] = "changebreakpoint";
const char CLEARBREAKPOINT[] = "clearbreakpoint";
const char SETEXCEPTIONBREAK[] = "setexceptionbreak";
const char V8FLAGS[] = "v8flags";
const char VERSION[] = "version";
const char DISCONNECT[] = "disconnect";
const char LISTBREAKPOINTS[] = "listbreakpoints";
const char GARBAGECOLLECTOR[] = "gc";
//const char PROFILE[] = "profile";

const char CONNECT[] = "connect";
const char INTERRUPT[] = "interrupt";

const char REQUEST[] = "request";
const char IN[] = "in";
const char NEXT[] = "next";
const char OUT[] = "out";

const char FUNCTION[] = "function";
const char SCRIPT[] = "script";
const char EVENT[] = "event";

const char ALL[] = "all";
const char UNCAUGHT[] = "uncaught";

//const char PAUSE[] = "pause";
//const char RESUME[] = "resume";

const char HANDLE[] = "handle";
const char REF[] = "ref";
const char REFS[] = "refs";
const char BODY[] = "body";
const char NAME[] = "name";
const char VALUE[] = "value";

const char OBJECT[] = "{}";
const char ARRAY[] = "[]";

} //Internal
} //Debugger
#endif // QMLV8DEBUGGERCLIENTCONSTANTS_H
