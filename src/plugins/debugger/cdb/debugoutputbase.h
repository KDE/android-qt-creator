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

#ifndef DEBUGOUTPUTBASE_H
#define DEBUGOUTPUTBASE_H

#include "cdbcom.h"

#include <QtCore/QString>
#include <QtCore/QSharedPointer>

namespace CdbCore {

class CoreEngine;

// CdbDebugOutputBase is a base class for output handlers
// that takes care of the Active X magic and conversion to QString.

class DebugOutputBase : public IDebugOutputCallbacksWide
{
public:
    virtual ~DebugOutputBase();
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

    // IDebugOutputCallbacks.
    STDMETHOD(Output)(
            THIS_
            IN ULONG mask,
            IN PCWSTR text
            );

    // Helpers to retrieve the output callbacks IF
    static IDebugOutputCallbacksWide *getOutputCallback(CIDebugClient *client);
    static const char *maskDescription(ULONG m);

protected:
    DebugOutputBase();
    virtual void output(ULONG mask, const QString &message) = 0;
};

// An output handler that adds lines to a string (to be
// used for cases in which linebreaks occur in-between calls
// to output).
class StringOutputHandler : public DebugOutputBase
{
public:
    StringOutputHandler() {}
    QString result() const { return m_result; }

protected:
    virtual void output(ULONG, const QString &message) { m_result += message; }

private:
    QString m_result;
};

// Utility class to temporarily redirect output to another handler
// as long as in scope
class OutputRedirector {
    Q_DISABLE_COPY(OutputRedirector)
public:
    typedef QSharedPointer<DebugOutputBase> DebugOutputBasePtr;

    explicit OutputRedirector(CoreEngine *engine, const DebugOutputBasePtr &o);
    ~OutputRedirector();

private:
    CoreEngine *m_engine;
    const DebugOutputBasePtr m_oldOutput;
};

} // namespace CdbCore

#endif // DEBUGOUTPUTBASE_H
