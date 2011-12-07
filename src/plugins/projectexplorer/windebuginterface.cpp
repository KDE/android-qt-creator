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

#include "windebuginterface.h"

#ifdef Q_OS_WIN

#include <windows.h>


/*!
    \class ProjectExplorer::Internal::WinDebugInterface
    \brief Windows: Captures output of the Windows API OutputDebugString() function.

    Emits output by process id.

    OutputDebugString puts its data into a shared memory segment named
    \c DBWIN_BUFFER which can be accessed via file mapping.
*/

namespace ProjectExplorer {
namespace Internal {

WinDebugInterface *WinDebugInterface::m_instance = 0;

WinDebugInterface *WinDebugInterface::instance()
{
    return m_instance;
}

WinDebugInterface::WinDebugInterface(QObject *parent) :
    QThread(parent)
{
    m_instance = this;
    setObjectName(QLatin1String("WinDebugInterfaceThread"));
    start();
}

WinDebugInterface::~WinDebugInterface()
{
    if (m_waitHandles[TerminateEventHandle]) {
        SetEvent(m_waitHandles[TerminateEventHandle]);
        wait(500);
    }
    m_instance = 0;
}

void WinDebugInterface::run()
{
    m_waitHandles[DataReadyEventHandle] = m_waitHandles[TerminateEventHandle] = 0;
    m_bufferReadyEvent = 0;
    m_sharedFile = 0;
    m_sharedMem  = 0;
    if (!runLoop())
        emit cannotRetrieveDebugOutput();
    if (m_sharedMem) {
        UnmapViewOfFile(m_sharedMem);
        m_sharedMem = 0;
    }
    if (m_sharedFile) {
        CloseHandle(m_sharedFile);
        m_sharedFile = 0;
    }
    if (m_waitHandles[TerminateEventHandle]) {
        CloseHandle(m_waitHandles[TerminateEventHandle]);
        m_waitHandles[TerminateEventHandle] = 0;
    }
    if (m_waitHandles[DataReadyEventHandle]) {
        CloseHandle(m_waitHandles[DataReadyEventHandle]);
        m_waitHandles[DataReadyEventHandle] = 0;
    }
    if (m_bufferReadyEvent) {
        CloseHandle(m_bufferReadyEvent);
        m_bufferReadyEvent = 0;
    }
}

bool WinDebugInterface::runLoop()
{
    m_waitHandles[TerminateEventHandle] = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        return false;
    m_waitHandles[DataReadyEventHandle] = CreateEvent(NULL, FALSE, FALSE, L"DBWIN_DATA_READY");
    if (!m_waitHandles[TerminateEventHandle] || !m_waitHandles[DataReadyEventHandle]
            || GetLastError() == ERROR_ALREADY_EXISTS)
        return false;
    m_bufferReadyEvent = CreateEvent(NULL, FALSE, FALSE, L"DBWIN_BUFFER_READY");
    if (!m_bufferReadyEvent
            || GetLastError() == ERROR_ALREADY_EXISTS)
        return false;
    m_sharedFile = CreateFileMapping((HANDLE)-1, NULL, PAGE_READWRITE, 0, 4096, L"DBWIN_BUFFER");
    if (!m_sharedFile || GetLastError() == ERROR_ALREADY_EXISTS)
        return false;
    m_sharedMem = MapViewOfFile(m_sharedFile, FILE_MAP_READ, 0, 0,  512);
    if (!m_sharedMem)
        return false;

    LPSTR  message = reinterpret_cast<LPSTR>(m_sharedMem) + sizeof(DWORD);
    LPDWORD processId = reinterpret_cast<LPDWORD>(m_sharedMem);

    SetEvent(m_bufferReadyEvent);

    while (true) {
        const DWORD ret = WaitForMultipleObjects(HandleCount, m_waitHandles, FALSE, INFINITE);
        if (ret == WAIT_FAILED || ret - WAIT_OBJECT_0 == TerminateEventHandle)
            break;
        if (ret - WAIT_OBJECT_0 == DataReadyEventHandle) {
            emit debugOutput(*processId, QString::fromLocal8Bit(message));
            SetEvent(m_bufferReadyEvent);
        }
    }
    return true;
}

} // namespace Internal
} // namespace ProjectExplorer

#else

namespace ProjectExplorer {
namespace Internal {

WinDebugInterface *WinDebugInterface::m_instance = 0;

WinDebugInterface *WinDebugInterface::instance() { return 0; }

WinDebugInterface::WinDebugInterface(QObject *) {}

WinDebugInterface::~WinDebugInterface() {}

void WinDebugInterface::run() {}

bool WinDebugInterface::runLoop() { return false; }

} // namespace Internal
} // namespace ProjectExplorer

#endif
