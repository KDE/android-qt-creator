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

#include "debugeventcallbackbase.h"
#include "coreengine.h"

namespace CdbCore {

// DebugEventCallbackBase
DebugEventCallbackBase::DebugEventCallbackBase()
{
}

DebugEventCallbackBase::~DebugEventCallbackBase()
{
}

STDMETHODIMP DebugEventCallbackBase::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface)
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
        IsEqualIID(InterfaceId, __uuidof(IDebugOutputCallbacks)))  {
        *Interface = (IDebugOutputCallbacks *)this;
        AddRef();
        return S_OK;
    } else {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) DebugEventCallbackBase::AddRef(THIS)
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG) DebugEventCallbackBase::Release(THIS)
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP DebugEventCallbackBase::Breakpoint(THIS_ __in PDEBUG_BREAKPOINT2)
{
    return S_OK;
}
STDMETHODIMP DebugEventCallbackBase::Exception(
    THIS_
    __in PEXCEPTION_RECORD64,
    __in ULONG /* FirstChance */
    )
{
    return S_OK;
}

STDMETHODIMP DebugEventCallbackBase::CreateThread(
    THIS_
    __in ULONG64 /* Handle */,
    __in ULONG64 /* DataOffset */,
    __in ULONG64 /* StartOffset */
    )
{
    return S_OK;
}

STDMETHODIMP DebugEventCallbackBase::ExitThread(
    THIS_
    __in ULONG /* ExitCode */
    )
{
    return S_OK;
}

STDMETHODIMP DebugEventCallbackBase::CreateProcess(
    THIS_
    __in ULONG64 /* ImageFileHandle */,
    __in ULONG64 /* Handle */,
    __in ULONG64 /* BaseOffset */,
    __in ULONG /* ModuleSize */,
    __in_opt PCWSTR /* ModuleName */,
    __in_opt PCWSTR /* ImageName */,
    __in ULONG /* CheckSum */,
    __in ULONG /* TimeDateStamp */,
    __in ULONG64 /* InitialThreadHandle */,
    __in ULONG64 /* ThreadDataOffset */,
    __in ULONG64 /* StartOffset */
    )
{
    return S_OK;
}

STDMETHODIMP DebugEventCallbackBase::ExitProcess(
    THIS_
    __in ULONG /* ExitCode */
    )
{
    return S_OK;
}

STDMETHODIMP DebugEventCallbackBase::LoadModule(
    THIS_
    __in ULONG64 /* ImageFileHandle */,
    __in ULONG64 /* BaseOffset */,
    __in ULONG /* ModuleSize */,
    __in_opt PCWSTR /* ModuleName */,
    __in_opt PCWSTR /* ImageName */,
    __in ULONG /* CheckSum */,
    __in ULONG /* TimeDateStamp */
    )
{
    handleModuleLoad();
    return S_OK;
}

STDMETHODIMP DebugEventCallbackBase::UnloadModule(
    THIS_
    __in_opt PCWSTR /* ImageBaseName */,
    __in ULONG64 /* BaseOffset */
    )
{
    handleModuleUnload();
    return S_OK;
}

STDMETHODIMP DebugEventCallbackBase::SystemError(
    THIS_
    __in ULONG /* Error */,
    __in ULONG /* Level */
    )
{
    return S_OK;
}

STDMETHODIMP DebugEventCallbackBase::SessionStatus(
    THIS_
    __in ULONG /* Status */
    )
{
    return S_OK;
}

STDMETHODIMP DebugEventCallbackBase::ChangeDebuggeeState(
    THIS_
    __in ULONG /* Flags */,
    __in ULONG64 /* Argument */
    )
{
    return S_OK;
}

STDMETHODIMP DebugEventCallbackBase::ChangeEngineState(
    THIS_
    __in ULONG /* Flags */,
    __in ULONG64 /* Argument */
    )
{
    return S_OK;
}

STDMETHODIMP DebugEventCallbackBase::ChangeSymbolState(
    THIS_
    __in ULONG /* Flags */,
    __in ULONG64 /* Argument */
    )
{
    return S_OK;
}

IDebugEventCallbacksWide *DebugEventCallbackBase::getEventCallback(CIDebugClient *clnt)
{
    IDebugEventCallbacksWide *rc = 0;
    if (SUCCEEDED(clnt->GetEventCallbacksWide(&rc)))
        return rc;
    return 0;
}

void DebugEventCallbackBase::handleModuleLoad()
{
    m_moduleCount++;
}

void DebugEventCallbackBase::handleModuleUnload()
{
    m_moduleCount--;
}

unsigned DebugEventCallbackBase::moduleCount() const
{
    return m_moduleCount;
}

void DebugEventCallbackBase::setModuleCount(unsigned m)
{
    m_moduleCount = m;
}

ULONG DebugEventCallbackBase::baseInterestMask() const
{
    return DEBUG_EVENT_LOAD_MODULE | DEBUG_EVENT_UNLOAD_MODULE;
}

// ----------- EventCallbackRedirector

EventCallbackRedirector::EventCallbackRedirector(CoreEngine *engine,
                                                 const DebugEventCallbackBasePtr &cb) :
    m_engine(engine),
    m_oldCallback(engine->setDebugEventCallback(cb))
{
}

EventCallbackRedirector::~EventCallbackRedirector()
{
    m_engine->setDebugEventCallback(m_oldCallback);
}


} // namespace CdbCore
