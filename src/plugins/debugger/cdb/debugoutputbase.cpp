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

#include "debugoutputbase.h"
#include "coreengine.h"
#include <QtCore/QDebug>

namespace CdbCore {

DebugOutputBase::DebugOutputBase()
{
}

DebugOutputBase::~DebugOutputBase() // must be present to avoid exit crashes
{
}

STDMETHODIMP DebugOutputBase::QueryInterface(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        )
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
        IsEqualIID(InterfaceId, __uuidof(IDebugOutputCallbacksWide)))
    {
        *Interface = (IDebugOutputCallbacksWide*)this;
        AddRef();
        return S_OK;
    } else {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) DebugOutputBase::AddRef(THIS)
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG) DebugOutputBase::Release(THIS)
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP DebugOutputBase::Output(
        THIS_
        IN ULONG mask,
        IN PCWSTR text
        )
{
    const QString msg = QString::fromUtf16(reinterpret_cast<const ushort *>(text));
    output(mask, msg);
    return S_OK;
}

IDebugOutputCallbacksWide *DebugOutputBase::getOutputCallback(CIDebugClient *client)
{
    IDebugOutputCallbacksWide *rc;
    if (FAILED(client->GetOutputCallbacksWide(&rc)))
        return 0;
    return rc;
}

const char *DebugOutputBase::maskDescription(ULONG m)
{
    switch (m) {
    case DEBUG_OUTPUT_NORMAL:
        break;
    case DEBUG_OUTPUT_ERROR:
        return "error";
    case DEBUG_OUTPUT_WARNING:
        return "warn";
    case DEBUG_OUTPUT_VERBOSE:
        return "verbose";
    case DEBUG_OUTPUT_PROMPT_REGISTERS:
        return "register";
    case DEBUG_OUTPUT_EXTENSION_WARNING:
        return "extwarn";
    case DEBUG_OUTPUT_DEBUGGEE:
        return "target";
    case DEBUG_OUTPUT_DEBUGGEE_PROMPT:
        return "input";
    case DEBUG_OUTPUT_SYMBOLS:
        return "symbol";
    default:
        break;
    }
    return "misc";
}

OutputRedirector::OutputRedirector(CoreEngine *engine, const DebugOutputBasePtr &o) :
    m_engine(engine),
    m_oldOutput(engine->setDebugOutput(o))
{
}

OutputRedirector::~OutputRedirector()
{
    m_engine->setDebugOutput(m_oldOutput);
}

} // namespace CdbCore
