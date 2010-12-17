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

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include "registerpostmortemaction.h"

#include "registryaccess.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QString>

#include <Windows.h>
#include <Objbase.h>
#include <Shellapi.h>

using namespace RegistryAccess;

namespace Debugger {
namespace Internal {

void RegisterPostMortemAction::registerNow(const QVariant &value)
{
    const bool boolValue = value.toBool();
    const QString debuggerExe = QCoreApplication::applicationDirPath() + QLatin1Char('/')
                                + debuggerApplicationFileC + QLatin1String(".exe");
    const std::wstring debuggerWString = QDir::toNativeSeparators(debuggerExe).toStdWString();

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    SHELLEXECUTEINFO shExecInfo;
    shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    shExecInfo.fMask  = SEE_MASK_NOCLOSEPROCESS;
    shExecInfo.hwnd   = NULL;
    shExecInfo.lpVerb = L"runas";
    shExecInfo.lpFile = debuggerWString.data();
    shExecInfo.lpParameters = boolValue ? L"-register" : L"-unregister";
    shExecInfo.lpDirectory  = NULL;
    shExecInfo.nShow        = SW_SHOWNORMAL;
    shExecInfo.hProcess     = NULL;
    if (ShellExecuteEx(&shExecInfo) && shExecInfo.hProcess)
        WaitForSingleObject(shExecInfo.hProcess, INFINITE);
    CoUninitialize();
    readSettings();
}

RegisterPostMortemAction::RegisterPostMortemAction(QObject *parent) : Utils::SavedAction(parent)
{
    connect(this, SIGNAL(valueChanged(QVariant)), SLOT(registerNow(QVariant)));
}

void RegisterPostMortemAction::readSettings(const QSettings *)
{
    Q_UNUSED(debuggerRegistryValueNameC); // avoid warning from MinGW

    bool registered = false;
    HKEY handle = 0;
    QString errorMessage;
    if (openRegistryKey(HKEY_LOCAL_MACHINE, debuggerRegistryKeyC, false, &handle, &errorMessage))
        registered = isRegistered(handle, debuggerCall(), &errorMessage);
    if (handle)
        RegCloseKey(handle);
    setValue(registered, false);
}

} // namespace Internal
} // namespace Debugger
