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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGEVENTHANDLER_H
#define DEBUGEVENTHANDLER_H

#include "common.h"
#include "extensioncontext.h"

class EventCallback : public IDebugEventCallbacks
{
public:
    explicit EventCallback(IDebugEventCallbacks *wrapped);
    virtual ~EventCallback();
    // IUnknown.
    STDMETHOD(QueryInterface)(
            THIS_
            IN REFIID InterfaceId,
            OUT PVOID* Interface
            );
    STDMETHOD_(ULONG, AddRef)(
            THIS
            );
    STDMETHOD_(ULONG, Release)(
            THIS
            );

    // IDebugEventCallbacks.

    STDMETHOD(GetInterestMask)(
        THIS_
        __out PULONG mask
        );


    STDMETHOD(Breakpoint)(
            THIS_
            __in PDEBUG_BREAKPOINT Bp
            );

    STDMETHOD(Exception)(
            THIS_
            __in PEXCEPTION_RECORD64 Exception,
            __in ULONG FirstChance
            );

    STDMETHOD(CreateThread)(
            THIS_
            __in ULONG64 Handle,
            __in ULONG64 DataOffset,
            __in ULONG64 StartOffset
            );
    STDMETHOD(ExitThread)(
            THIS_
            __in ULONG ExitCode
            );

    STDMETHOD(CreateProcess)(
            THIS_
            __in ULONG64 ImageFileHandle,
            __in ULONG64 Handle,
            __in ULONG64 BaseOffset,
            __in ULONG ModuleSize,
            __in_opt PCSTR ModuleName,
            __in_opt PCSTR ImageName,
            __in ULONG CheckSum,
            __in ULONG TimeDateStamp,
            __in ULONG64 InitialThreadHandle,
            __in ULONG64 ThreadDataOffset,
            __in ULONG64 StartOffset
            );

    STDMETHOD(ExitProcess)(
            THIS_
            __in ULONG ExitCode
            );

    // Call handleModuleLoad() when reimplementing this
    STDMETHOD(LoadModule)(
            THIS_
            __in ULONG64 ImageFileHandle,
            __in ULONG64 BaseOffset,
            __in ULONG ModuleSize,
            __in_opt PCSTR ModuleName,
            __in_opt PCSTR ImageName,
            __in ULONG CheckSum,
            __in ULONG TimeDateStamp
            );

    // Call handleModuleUnload() when reimplementing this
    STDMETHOD(UnloadModule)(
            THIS_
            __in_opt PCSTR ImageBaseName,
            __in ULONG64 BaseOffset
            );

    STDMETHOD(SystemError)(
            THIS_
            __in ULONG Error,
            __in ULONG Level
            );

    STDMETHOD(SessionStatus)(
            THIS_
            __in ULONG Status
            );

    STDMETHOD(ChangeDebuggeeState)(
            THIS_
            __in ULONG Flags,
            __in ULONG64 Argument
            );

    STDMETHOD(ChangeEngineState)(
            THIS_
            __in ULONG Flags,
            __in ULONG64 Argument
            );

    STDMETHOD(ChangeSymbolState)(
            THIS_
            __in ULONG Flags,
            __in ULONG64 Argument
            );

    private:
        IDebugEventCallbacks *m_wrapped;
};
#endif // DEBUGEVENTHANDLER_H
