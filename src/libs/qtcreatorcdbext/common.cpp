/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "common.h"
#include "iinterfacepointer.h"
#include <sstream>

std::string winErrorMessage(unsigned long error)
{
    char *lpMsgBuf;
    const int len = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, 0, (LPSTR)&lpMsgBuf, 0, NULL);
    if (len) {
        const std::string rc(lpMsgBuf, len);
        LocalFree(lpMsgBuf);
        return rc;
    }
    std::ostringstream str;
    str << "Unknown error " << error;
    return str.str();
}

std::string winErrorMessage()
{
    return winErrorMessage(GetLastError());
}

std::string msgDebugEngineComResult(HRESULT hr)
{
    switch (hr) {
    case S_OK:
        return std::string("S_OK");
    case S_FALSE:
        return std::string("S_FALSE");
    case E_FAIL:
        break;
    case E_INVALIDARG:
        return std::string("E_INVALIDARG");
    case E_NOINTERFACE:
        return std::string("E_NOINTERFACE");
    case E_OUTOFMEMORY:
        return std::string("E_OUTOFMEMORY");
    case E_UNEXPECTED:
        return std::string("E_UNEXPECTED");
    case E_NOTIMPL:
        return std::string("E_NOTIMPL");
    }
    if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
        return std::string("ERROR_ACCESS_DENIED");;
    if (hr == HRESULT_FROM_NT(STATUS_CONTROL_C_EXIT))
        return std::string("STATUS_CONTROL_C_EXIT");
    return std::string("E_FAIL ") + winErrorMessage(HRESULT_CODE(hr));
}

std::string msgDebugEngineComFailed(const char *func, HRESULT hr)
{
    std::string rc = func;
    rc += " failed: ";
    rc += msgDebugEngineComResult(hr);
    return rc;
}

ULONG currentThreadId(IDebugSystemObjects *sysObjects)
{
    ULONG id = 0;
    if (sysObjects->GetCurrentThreadId(&id) == S_OK)
        return id;
    return 0;
}

ULONG currentThreadId(CIDebugClient *client)
{
    IInterfacePointer<IDebugSystemObjects> sysObjects(client);
    if (sysObjects)
        return currentThreadId(sysObjects.data());
    return 0;
}

ULONG currentProcessId(IDebugSystemObjects *sysObjects)
{
    ULONG64 handle = 0;
    if (sysObjects->GetCurrentProcessHandle(&handle) == S_OK)
        return GetProcessId((HANDLE)handle);
    return 0;
}

ULONG currentProcessId(CIDebugClient *client)
{
    IInterfacePointer<IDebugSystemObjects> sysObjects(client);
    if (sysObjects)
        return currentProcessId(sysObjects.data());
    return 0;
}

std::string moduleNameByOffset(CIDebugSymbols *symbols, ULONG64 offset)
{
    enum { BufSize = 512 };
    ULONG index = 0;
    ULONG64 base = 0;
    // Convert module base address to module index
    HRESULT hr = symbols->GetModuleByOffset(offset, 0, &index, &base);
    if (FAILED(hr))
        return std::string();
    // Obtain module name
    char buf[BufSize];
    buf[0] = '\0';
    hr = symbols->GetModuleNameString(DEBUG_MODNAME_MODULE, index, base, buf, BufSize, 0);
    if (FAILED(hr))
        return std::string();
    return std::string(buf);
}
