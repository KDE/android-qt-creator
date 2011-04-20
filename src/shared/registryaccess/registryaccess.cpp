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

#include "registryaccess.h"

#include <QtGui/QApplication>

#include <QtCore/QDir>
#include <QtCore/QTextStream>

namespace RegistryAccess {

static QString winErrorMessage(unsigned long error)
{
    QString rc = QString::fromLatin1("#%1: ").arg(error);
    ushort *lpMsgBuf;

    const int len = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
    if (len) {
       rc = QString::fromUtf16(lpMsgBuf, len);
        LocalFree(lpMsgBuf);
    } else {
        rc += QString::fromLatin1("<unknown error>");
    }
    return rc;
}

QString msgFunctionFailed(const char *f, unsigned long error)
{
    return QString::fromLatin1("'%1' failed: %2").arg(QLatin1String(f), winErrorMessage(error));
}

static bool registryReadBinaryKey(HKEY handle, // HKEY_LOCAL_MACHINE, etc.
                                  const WCHAR *valueName,
                                  QByteArray *data,
                                  QString *errorMessage)
{
    data->clear();
    DWORD type;
    DWORD size;
    // get size and retrieve
    LONG rc = RegQueryValueEx(handle, valueName, 0, &type, 0, &size);
    if (rc != ERROR_SUCCESS) {
        *errorMessage = msgRegistryOperationFailed("read", valueName, msgFunctionFailed("RegQueryValueEx1", rc));
        return false;
    }
    BYTE *dataC = new BYTE[size + 1];
    // Will be Utf16 in case of a string
    rc = RegQueryValueEx(handle, valueName, 0, &type, dataC, &size);
    if (rc != ERROR_SUCCESS) {
        *errorMessage = msgRegistryOperationFailed("read", valueName, msgFunctionFailed("RegQueryValueEx2", rc));
        return false;
    }
    *data = QByteArray(reinterpret_cast<const char*>(dataC), size);
    delete [] dataC;
    return true;
}

bool registryReadStringKey(HKEY handle, // HKEY_LOCAL_MACHINE, etc.
                           const WCHAR *valueName,
                           QString *s,
                           QString *errorMessage)
{
    QByteArray data;
    if (!registryReadBinaryKey(handle, valueName, &data, errorMessage))
        return false;
    data += '\0';
    data += '\0';
    *s = QString::fromUtf16(reinterpret_cast<const unsigned short*>(data.data()));
    return true;
}

bool openRegistryKey(HKEY category, // HKEY_LOCAL_MACHINE, etc.
                     const WCHAR *key,
                     bool readWrite,
                     HKEY *keyHandle,
                     QString *errorMessage)
{
    Q_UNUSED(debuggerRegistryKeyC);  // avoid warning from MinGW

    REGSAM accessRights = KEY_READ;
    if (readWrite)
         accessRights |= KEY_SET_VALUE;
    const LONG rc = RegOpenKeyEx(category, key, 0, accessRights, keyHandle);
    if (rc != ERROR_SUCCESS) {
        *errorMessage = msgFunctionFailed("RegOpenKeyEx", rc);
        if (readWrite)
            *errorMessage += "You need administrator privileges to edit the registry.";
        return false;
    }
    return true;
}

// Installation helpers: Format the debugger call with placeholders for PID and event
// '"[path]\qtcdebugger" [-wow] %ld %ld'.

QString debuggerCall(const QString &additionalOption)
{
    QString rc;
    QTextStream str(&rc);
    str << '"' << QDir::toNativeSeparators(QApplication::applicationDirPath() + QLatin1Char('/')
                                           + debuggerApplicationFileC + QLatin1String(".exe")) << '"';
    if (!additionalOption.isEmpty())
        str << ' ' << additionalOption;
    str << " %ld %ld";
    return rc;
}

bool isRegistered(HKEY handle, const QString &call, QString *errorMessage, QString *oldDebugger)
{
    QString registeredDebugger;
    registryReadStringKey(handle, debuggerRegistryValueNameC, &registeredDebugger, errorMessage);
    if (oldDebugger)
        *oldDebugger = registeredDebugger;
    return !registeredDebugger.compare(call, Qt::CaseInsensitive);
}

} // namespace RegistryAccess
