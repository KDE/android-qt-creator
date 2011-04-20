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

#ifndef THREADLIST_H
#define THREADLIST_H

#include "common.h"
#include <vector>

struct SymbolGroupValueContext;

/* Various helpers to the extension commands to retrieve debuggee information
 * in suitable formats for the debugger engine. */

/* Helpers to retrieve threads and their top stack frame
 * in one roundtrip, conveniently in GDBMI format. */

struct StackFrame
{
    StackFrame(ULONG64 a = 0);
    void formatGDBMI(std::ostream &str, unsigned level = 0) const;
    std::wstring fileName() const;

    ULONG64 address;
    std::wstring function;
    std::wstring fullPathName;
    ULONG line;
};

typedef std::vector<StackFrame> StackFrames;

bool getFrame(unsigned n, StackFrame *f, std::string *errorMessage);
bool getFrame(CIDebugSymbols *debugSymbols, CIDebugControl *debugControl,
              unsigned n, StackFrame *f, std::string *errorMessage);

struct Thread
{
    Thread(ULONG i = 0, ULONG sysId = 0);
    void formatGDBMI(std::ostream &str) const;

    ULONG id;
    ULONG systemId;
    ULONG64 address;
    std::string state;
    std::wstring name;
    StackFrame frame;
};

typedef std::vector<Thread> Threads;

// Get list of threads.
bool threadList(CIDebugSystemObjects *debugSystemObjects,
                CIDebugSymbols *debugSymbols,
                CIDebugControl *debugControl,
                CIDebugAdvanced *debugAdvanced,
                Threads* threads, ULONG *currentThreadId,
                std::string *errorMessage);

// Convenience formatting as GDBMI
std::string gdbmiThreadList(CIDebugSystemObjects *debugSystemObjects,
                            CIDebugSymbols *debugSymbols,
                            CIDebugControl *debugControl,
                            CIDebugAdvanced *debugAdvanced,
                            std::string *errorMessage);

/* Helpers for retrieving module lists */

struct Module {
    Module();

    std::string name;
    std::string image;
    bool deferred;
    ULONG64 base;
    ULONG64 size;
};

typedef std::vector<Module> Modules;

// Retrieve modules info
Modules getModules(CIDebugSymbols *syms, std::string *errorMessage);

// Format modules as GDBMI
std::string gdbmiModules(CIDebugSymbols *syms, bool humanReadable, std::string *errorMessage);

// Format breakpoints as GDBMI
std::string gdbmiBreakpoints(CIDebugControl *ctrl,
                             CIDebugSymbols *symbols /* = 0 */,
                             bool humanReadable,
                             unsigned verbose,
                             std::string *errorMessage);

/* Helpers for registers */
struct Register
{
    Register();

    std::wstring name;
    std::wstring description;
    bool subRegister;
    bool pseudoRegister;
    DEBUG_VALUE value;
};

typedef std::vector<Register> Registers;

// Description of a DEBUG_VALUE type field
const wchar_t *valueType(ULONG type);
// Helper to format a DEBUG_VALUE
void formatDebugValue(std::ostream &str, const DEBUG_VALUE &dv, CIDebugControl *ctl = 0);

enum RegisterFlags {
    IncludePseudoRegisters = 0x1,
    IncludeSubRegisters = 0x2
};

// Retrieve current register snapshot using RegisterFlags
Registers getRegisters(CIDebugRegisters *regs,
                       unsigned flags,
                       std::string *errorMessage);

// Format current register snapshot using RegisterFlags as GDBMI
// This is not exactly using the actual GDB formatting as this is
// to verbose (names and values separated)
std::string gdbmiRegisters(CIDebugRegisters *regs,
                           CIDebugControl *control,
                           bool humanReadable,
                           unsigned flags,
                           std::string *errorMessage);

std::string memoryToBase64(CIDebugDataSpaces *ds, ULONG64 address, ULONG length, std::string *errorMessage);

// Stack helpers
StackFrames getStackTrace(CIDebugControl *debugControl, CIDebugSymbols *debugSymbols,
                                 unsigned maxFrames, std::string *errorMessage);

std::string gdbmiStack(CIDebugControl *debugControl, CIDebugSymbols *debugSymbols,
                       unsigned maxFrames, bool humanReadable,
                       std::string *errorMessage);

// Find the widget of the application at (x,y) by calling QApplication::widgetAt().
// Return a string of "Qualified_ClassName:Address"
std::string widgetAt(const SymbolGroupValueContext &ctx,
                     int x, int y, std::string *errorMessage);

#endif // THREADLIST_H
