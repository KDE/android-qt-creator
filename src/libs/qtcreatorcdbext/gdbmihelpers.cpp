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

#include "gdbmihelpers.h"
#include "stringutils.h"
#include "iinterfacepointer.h"
#include "base64.h"
#include "symbolgroupvalue.h"
#include "extensioncontext.h"

#include <vector>

StackFrame::StackFrame(ULONG64 a) : address(a), line(0) {}

std::wstring StackFrame::fileName() const
{
    std::wstring::size_type lastSlash = fullPathName.rfind(L'\\');
    if (lastSlash == std::wstring::npos)
        return fullPathName;
    return fullPathName.substr(lastSlash + 1, fullPathName.size() - lastSlash  - 1);
}

void StackFrame::formatGDBMI(std::ostream &str, unsigned level) const
{
    str << "frame={level=\"" << level << "\",addr=\"0x"
        << std::hex << address << std::dec << '"';
    if (!function.empty()) {
        // Split into module/function
        const std::wstring::size_type exclPos = function.find('!');
        if (exclPos == std::wstring::npos) {
            str << ",func=\"" << gdbmiWStringFormat(function) << '"';
        } else {
            const std::wstring module = function.substr(0, exclPos);
            const std::wstring fn = function.substr(exclPos + 1, function.size() - exclPos - 1);
            str << ",func=\"" << gdbmiWStringFormat(fn)
                << "\",from=\"" << gdbmiWStringFormat(module) << '"';
        }
    }
    if (!fullPathName.empty()) { // Creator/gdbmi expects 'clean paths'
        std::wstring cleanPath = fullPathName;
        replace(cleanPath, L'\\', L'/');
        str << ",fullname=\"" << gdbmiWStringFormat(fullPathName)
            << "\",file=\"" << gdbmiWStringFormat(fileName()) << "\",line=\"" << line << '"';
    }
    str << '}';
}

Thread::Thread(ULONG i, ULONG sysId) : id(i), systemId(sysId) {}

void Thread::formatGDBMI(std::ostream &str) const
{
    str << "{id=\"" << id << "\",target-id=\"" << systemId << "\",";
    frame.formatGDBMI(str);
    if (!name.empty())
        str << ",name=\"" << gdbmiWStringFormat(name) << '"';
    if (!state.empty())
        str << ",state=\"" << state << '"';
    str << '}';
}

static inline std::string msgGetThreadsFailed(const std::string &why)
{
    return std::string("Unable to determine the thread information: ") + why;
}

static inline bool setCurrentThread(CIDebugSystemObjects *debugSystemObjects,
                                    ULONG id, std::string *errorMessage)
{
    const HRESULT hr = debugSystemObjects->SetCurrentThreadId(id);
    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("SetCurrentThreadId", hr);
        return false;
    }
    return true;
}

// Fill in stack frame info
void getFrame(CIDebugSymbols *debugSymbols,
              const DEBUG_STACK_FRAME &s, StackFrame *f)
{
    WCHAR wBuf[MAX_PATH];
    f->address = s.InstructionOffset;
    HRESULT hr = debugSymbols->GetNameByOffsetWide(f->address, wBuf, MAX_PATH, 0, 0);
    if (SUCCEEDED(hr)) {
        f->function = wBuf;
    } else {
        f->function.clear();
    }
    ULONG64 ul64Displacement = 0;
    hr = debugSymbols->GetLineByOffsetWide(f->address, &(f->line), wBuf, MAX_PATH, 0, &ul64Displacement);
    if (SUCCEEDED(hr)) {
        f->fullPathName = wBuf;
    } else {
        f->fullPathName.clear();
        f->line = 0;
    }
}

// Determine the frames of the threads.
// Note: Current thread is switched as a side effect!
static bool getThreadFrames(CIDebugSymbols *debugSymbols,
                            CIDebugSystemObjects *debugSystemObjects,
                            CIDebugControl *debugControl,
                            Threads *threads, std::string *errorMessage)
{
    enum { MaxFrames = 1 };

    DEBUG_STACK_FRAME frames[MaxFrames];
    const Threads::iterator end = threads->end();
    for (Threads::iterator it = threads->begin(); it != end; ++it) {
        if (!setCurrentThread(debugSystemObjects, it->id, errorMessage))
            return false;
        ULONG frameCount;
        const HRESULT hr = debugControl->GetStackTrace(0, 0, 0, frames, MaxFrames, &frameCount);
        if (SUCCEEDED(hr))
            getFrame(debugSymbols, frames[0], &(it->frame));
    }
    return true;
}

bool getFrame(unsigned n, StackFrame *f, std::string *errorMessage)
{
    IInterfacePointer<CIDebugClient> client;
    if (!client.create()) {
        *errorMessage = "Cannot obtain client.";
        return false;
    }
    IInterfacePointer<CIDebugSymbols> symbols(client.data());
    IInterfacePointer<CIDebugControl> control(client.data());
    if (!symbols || !control) {
        *errorMessage = "Cannot obtain required objects.";
        return false;
    }
    return getFrame(symbols.data(), control.data(), n, f, errorMessage);
}

bool getFrame(CIDebugSymbols *debugSymbols,
              CIDebugControl *debugControl,
              unsigned n, StackFrame *f, std::string *errorMessage)
{
    DEBUG_STACK_FRAME *frames = new DEBUG_STACK_FRAME[n + 1];
    ULONG frameCount;
    const HRESULT hr = debugControl->GetStackTrace(0, 0, 0, frames, n + 1, &frameCount);
    if (FAILED(hr)) {
        delete [] frames;
        *errorMessage = msgDebugEngineComFailed("GetStackTrace", hr);
        return false;
    }
    getFrame(debugSymbols, frames[n], f);
    delete [] frames;
    return true;
}

bool threadList(CIDebugSystemObjects *debugSystemObjects,
                CIDebugSymbols *debugSymbols,
                CIDebugControl *debugControl,
                CIDebugAdvanced *debugAdvanced,
                Threads* threads, ULONG *currentThreadId,
                std::string *errorMessage)
{
    threads->clear();
    ULONG threadCount;
    *currentThreadId = 0;
    // Get count
    HRESULT hr= debugSystemObjects->GetNumberThreads(&threadCount);
    if (FAILED(hr)) {
        *errorMessage= msgGetThreadsFailed(msgDebugEngineComFailed("GetNumberThreads", hr));
        return false;
    }
    // Get index of current
    if (!threadCount)
        return true;
    hr = debugSystemObjects->GetCurrentThreadId(currentThreadId);
    if (FAILED(hr)) {
        *errorMessage= msgGetThreadsFailed(msgDebugEngineComFailed("GetCurrentThreadId", hr));
        return false;
    }
    // Get Identifiers
    threads->reserve(threadCount);
    ULONG *ids = new ULONG[threadCount];
    ULONG *systemIds = new ULONG[threadCount];
    hr = debugSystemObjects->GetThreadIdsByIndex(0, threadCount, ids, systemIds);
    if (FAILED(hr)) {
        *errorMessage= msgGetThreadsFailed(msgDebugEngineComFailed("GetThreadIdsByIndex", hr));
        return false;
    }
    // Create entries
    static WCHAR name[256];
    for (ULONG i= 0; i < threadCount ; i++) {
        const ULONG id = ids[i];
        Thread thread(id, systemIds[i]);
        // Thread name
        ULONG bytesReceived = 0;
        hr = debugAdvanced->GetSystemObjectInformation(DEBUG_SYSOBJINFO_THREAD_NAME_WIDE,
                                                       0, id, name,
                                                       sizeof(name), &bytesReceived);
        if (SUCCEEDED(hr) && bytesReceived)
            thread.name = name;
        threads->push_back(thread);
    }
    delete [] ids;
    delete [] systemIds;
    // Get frames and at all events,
    // restore current thread after switching frames.
    const bool framesOk = getThreadFrames(debugSymbols,
                                          debugSystemObjects,
                                          debugControl,
                                          threads, errorMessage);
    const bool restoreOk =setCurrentThread(debugSystemObjects, *currentThreadId, errorMessage);
    return framesOk && restoreOk;
}

/* Format as: \code

52^done,threads=[{id="1",target-id="Thread 4740.0xb30",
frame={level="0",addr="0x00403487",func="foo",
args=[{name="this",value="0x27fee4"}],file="mainwindow.cpp",fullname="c:\\qt\\projects\\gitguim\\app/mainwindow.cpp",line="298"},state="stopped"}],
current-thread-id="1"
*/

std::string gdbmiThreadList(CIDebugSystemObjects *debugSystemObjects,
                            CIDebugSymbols *debugSymbols,
                            CIDebugControl *debugControl,
                            CIDebugAdvanced *debugAdvanced,
                            std::string *errorMessage)
{
    Threads threads;
    ULONG currentThreadId;
    if (!threadList(debugSystemObjects, debugSymbols, debugControl,
                    debugAdvanced, &threads, &currentThreadId, errorMessage))
        return std::string();
    std::ostringstream str;
    str << "{threads=[";
    const Threads::const_iterator cend = threads.end();
    for (Threads::const_iterator it = threads.begin(); it != cend; ++it)
        it->formatGDBMI(str);
    str << "],current-thread-id=\"" << currentThreadId << "\"}";
    return str.str();
}

Module::Module() : deferred(false), base(0), size(0)
{
}

Modules getModules(CIDebugSymbols *syms, std::string *errorMessage)
{
    enum { BufSize = 1024 };

    char nameBuf[BufSize];
    char fileBuf[BufSize];

    ULONG Loaded;
    ULONG Unloaded;
    HRESULT hr = syms->GetNumberModules(&Loaded, &Unloaded);
    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("GetNumberModules", hr);
        return Modules();
    }
    const ULONG count = Loaded + Unloaded;
    Modules rc;
    rc.reserve(count);
    DEBUG_MODULE_PARAMETERS *parameters = new DEBUG_MODULE_PARAMETERS[count];
    hr = syms->GetModuleParameters(count, NULL, 0, parameters);
    if (FAILED(hr)) {
        delete [] parameters;
        *errorMessage = msgDebugEngineComFailed("GetModuleParameters", hr);
        return Modules();
    }

    for (ULONG m = 0; m < count; m++) {
        Module module;
        module.base = parameters[m].Base;
        module.size = parameters[m].Size;
        module.deferred = parameters[m].Flags == DEBUG_SYMTYPE_DEFERRED;
        hr = syms->GetModuleNames(m, 0, fileBuf, BufSize, NULL,
                                  nameBuf, BufSize, NULL, NULL, NULL, NULL);
        if (FAILED(hr))
            break; // Fail silently should unloaded modules not work.
        module.name = nameBuf;
        module.image = fileBuf;
        rc.push_back(module);
    }
    return rc;
}

std::string gdbmiModules(CIDebugSymbols *syms, bool humanReadable, std::string *errorMessage)
{
    const Modules modules = getModules(syms, errorMessage);
    if (modules.empty())
        return std::string();

    std::ostringstream str;
    str << '[' << std::hex << std::showbase;
    const Modules::size_type size = modules.size();
    for (Modules::size_type m = 0; m < size; m++) {
        const Module &module = modules.at(m);
        if (m)
            str << ',';
        str << "{name=\"" << module.name
            << "\",image=\"" << gdbmiStringFormat(module.image)
            << "\",start=\"" << module.base << "\",end=\""
            << (module.base + module.size - 1) << '"';
        if (module.deferred)
            str << "{deferred=\"true\"";
        str << '}';
        if (humanReadable)
            str << '\n';
    }
    str << ']';
    return str.str();
}

// Description of a DEBUG_VALUE type field
const wchar_t *valueType(ULONG type)
{
    switch (type) {
    case DEBUG_VALUE_INT8:
        return L"I8";
    case DEBUG_VALUE_INT16:
        return L"I16";
    case DEBUG_VALUE_INT32:
        return L"I32";
    case DEBUG_VALUE_INT64:
        return  L"I64";
    case DEBUG_VALUE_FLOAT32:
        return L"F32";
    case DEBUG_VALUE_FLOAT64:
        return L"F64";
    case DEBUG_VALUE_FLOAT80:
        return L"F80";
    case DEBUG_VALUE_FLOAT128:
        return L"F128";
    case DEBUG_VALUE_VECTOR64:
        return L"V64";
    case DEBUG_VALUE_VECTOR128:
        return L"V128";
    }
    return L"";
}

// Format a 128bit vector register by adding digits in reverse order
void formatVectorRegister(std::ostream &str, const unsigned char *array, int size)
{
    const char oldFill = str.fill('0');

    str << "0x" << std::hex;
    for (int i = size - 1; i >= 0; i--) {
        str.width(2);
        str << unsigned(array[i]);
    }
    str << std::dec;

    str.fill(oldFill);
}

void formatDebugValue(std::ostream &str, const DEBUG_VALUE &dv, CIDebugControl *ctl)
{
    const std::ios::fmtflags savedState = str.flags();
    switch (dv.Type) {
    // Do not use showbase to get the hex prefix as this omits it for '0'. Grmpf.
    case DEBUG_VALUE_INT8:
        str << std::hex << "0x" << unsigned(dv.I8);
        break;
    case DEBUG_VALUE_INT16:
        str << std::hex << "0x" << dv.I16;
        break;
    case DEBUG_VALUE_INT32:
        str << std::hex << "0x" << dv.I32;
        break;
    case DEBUG_VALUE_INT64:
        str << std::hex << "0x" << dv.I64;
        break;
    case DEBUG_VALUE_FLOAT32:
        str << dv.F32;
        break;
    case DEBUG_VALUE_FLOAT64:
        str << dv.F64;
        break;
    case DEBUG_VALUE_FLOAT80:
    case DEBUG_VALUE_FLOAT128: { // Convert to double
            DEBUG_VALUE doubleValue;
            if (ctl && SUCCEEDED(ctl->CoerceValue(const_cast<DEBUG_VALUE*>(&dv), DEBUG_VALUE_FLOAT64, &doubleValue)))
                str << dv.F64;
            }
        break;
    case DEBUG_VALUE_VECTOR64:
        formatVectorRegister(str, dv.VI8, 8);
        break;
    case DEBUG_VALUE_VECTOR128:
        formatVectorRegister(str, dv.VI8, 16);
        break;
    }
    str.flags(savedState);
}

Register::Register() : subRegister(false), pseudoRegister(false)
{
    value.Type = DEBUG_VALUE_INT32;
    value.I32 = 0;
}

static inline std::wstring registerDescription(const DEBUG_REGISTER_DESCRIPTION &d)
{
    std::wostringstream str;
    str << valueType(d.Type);
    if (d.Flags & DEBUG_REGISTER_SUB_REGISTER)
        str << ", sub-register of " << d.SubregMaster;
    return str.str();
}

Registers getRegisters(CIDebugRegisters *regs,
                       unsigned flags,
                       std::string *errorMessage)
{
    enum { bufSize= 128 };
    WCHAR buf[bufSize];

    ULONG registerCount = 0;
    HRESULT hr = regs->GetNumberRegisters(&registerCount);
    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("GetNumberRegisters", hr);
        return Registers();
    }
    ULONG pseudoRegisterCount = 0;
    if (flags & IncludePseudoRegisters) {
        hr = regs->GetNumberPseudoRegisters(&pseudoRegisterCount);
        if (FAILED(hr)) {
            *errorMessage = msgDebugEngineComFailed("GetNumberPseudoRegisters", hr);
            return Registers();
        }
    }

    Registers rc;
    rc.reserve(registerCount + pseudoRegisterCount);

    // Standard registers
    DEBUG_REGISTER_DESCRIPTION description;
    DEBUG_VALUE value;
    for (ULONG r = 0; r < registerCount; r++) {
        hr = regs->GetDescriptionWide(r, buf, bufSize, NULL, &description);
        if (FAILED(hr)) {
            *errorMessage = msgDebugEngineComFailed("GetDescription", hr);
            return Registers();
        }
        hr = regs->GetValue(r, &value);
        if (FAILED(hr)) {
            *errorMessage = msgDebugEngineComFailed("GetValue", hr);
            return Registers();
        }
        const bool isSubRegister = (description.Flags & DEBUG_REGISTER_SUB_REGISTER);
        if (!isSubRegister || (flags & IncludeSubRegisters)) {
            Register reg;
            reg.name = buf;
            reg.description = registerDescription(description);
            reg.subRegister = isSubRegister;
            reg.value = value;
            rc.push_back(reg);
        }
    }

    // Pseudo
    for (ULONG r = 0; r < pseudoRegisterCount; r++) {
        ULONG type;
        hr = regs->GetPseudoDescriptionWide(r, buf, bufSize, NULL, NULL, &type);
        if (FAILED(hr))
            continue; // Fails for some pseudo registers
        hr = regs->GetValue(r, &value);
        if (FAILED(hr))
            continue; // Fails for some pseudo registers
        Register reg;
        reg.pseudoRegister = true;
        reg.name = buf;
        reg.description = valueType(type);
        reg.value = value;
        rc.push_back(reg);
    }
    return rc;
}

std::string gdbmiRegisters(CIDebugRegisters *regs,
                           CIDebugControl *control,
                           bool humanReadable,
                           unsigned flags,
                           std::string *errorMessage)
{
    if (regs == 0 || control == 0) {
        *errorMessage = "Required interfaces missing for registers dump.";
        return std::string();
    }

    const Registers registers = getRegisters(regs, flags, errorMessage);
    if (registers.empty())
        return std::string();

    std::ostringstream str;
    str << '[';
    if (humanReadable)
        str << '\n';
    const Registers::size_type size = registers.size();
    for (Registers::size_type r = 0; r < size; r++) {
        const Register &reg = registers.at(r);
        if (r)
            str << ',';
        str << "{number=\"" << r << "\",name=\"" << gdbmiWStringFormat(reg.name) << '"';
        if (!reg.description.empty())
            str << ",description=\"" << gdbmiWStringFormat(reg.description) << '"';
        if (reg.subRegister)
            str << ",subregister=\"true\"";
        if (reg.pseudoRegister)
            str << ",pseudoregister=\"true\"";
        str << ",value=\"";
        formatDebugValue(str, reg.value, control);
        str << "\"}";
        if (humanReadable)
            str << '\n';
    }
    str << ']';
    if (humanReadable)
        str << '\n';
    return str.str();
}

std::string memoryToBase64(CIDebugDataSpaces *ds, ULONG64 address, ULONG length, std::string *errorMessage)
{
    unsigned char *buffer = new unsigned char[length];
    std::fill(buffer, buffer + length, 0);
    ULONG received = 0;
    const HRESULT hr = ds->ReadVirtual(address, buffer, length, &received);
    if (FAILED(hr)) {
        delete [] buffer;
        std::ostringstream estr;
        estr << "Cannot read " << length << " bytes from " << address << ": "
                << msgDebugEngineComFailed("ReadVirtual", hr);
        *errorMessage = estr.str();
        return std::string();
    }
    if (received < length) {
        std::ostringstream estr;
        estr << "Warning: Received only " << received << " bytes of " << length << " requested at " << address << '.';
        *errorMessage = estr.str();
    }

    std::ostringstream str;
    base64Encode(str, buffer, length);
    delete [] buffer;
    return str.str();
}

// Format stack as GDBMI
static StackFrames getStackTrace(CIDebugControl *debugControl,
                                 CIDebugSymbols *debugSymbols,
                                 unsigned maxFrames,
                                 std::string *errorMessage)
{

    if (!maxFrames)
        return StackFrames();
    DEBUG_STACK_FRAME *frames = new DEBUG_STACK_FRAME[maxFrames];
    ULONG frameCount = 0;
    const HRESULT hr = debugControl->GetStackTrace(0, 0, 0, frames, maxFrames, &frameCount);
    if (FAILED(hr)) {
        delete [] frames;
        *errorMessage = msgDebugEngineComFailed("GetStackTrace", hr);
    }
    StackFrames rc(frameCount, StackFrame());
    for (ULONG f = 0; f < frameCount; f++)
        getFrame(debugSymbols, frames[f], &(rc[f]));
    delete [] frames;
    return rc;
}

std::string gdbmiStack(CIDebugControl *debugControl,
                       CIDebugSymbols *debugSymbols,
                       unsigned maxFrames,
                       bool humanReadable, std::string *errorMessage)
{
    const StackFrames frames = getStackTrace(debugControl, debugSymbols,
                                        maxFrames, errorMessage);
    if (frames.empty() && maxFrames > 0)
        return std::string();

    std::ostringstream str;
    str << '[';
    const StackFrames::size_type size = frames.size();
    for (StackFrames::size_type i = 0; i < size; i++) {
        if (i)
            str << ',';
        frames.at(i).formatGDBMI(str, (int)i);
        if (humanReadable)
            str << '\n';
    }
    str << ']';
    return str.str();
}

// Find the widget of the application by calling QApplication::widgetAt().
// Return "Qualified_ClassName:Address"

static inline std::string msgWidgetParseError(std::wstring wo)
{
    replace(wo, L'\n', L';');
    return "Output parse error :" + wStringToString(wo);
}

std::string widgetAt(const SymbolGroupValueContext &ctx, int x, int y, std::string *errorMessage)
{
    typedef SymbolGroupValue::SymbolList SymbolList;
    // First, resolve symbol since there are ambiguities. Take the first one which is the
    // overload for (int,int) and call by address instead off name to overcome that.
    const std::string func = QtInfo::get(ctx).prependQtGuiModule("QApplication::widgetAt");
    const SymbolList symbols = SymbolGroupValue::resolveSymbol(func.c_str(), ctx, errorMessage);
    if (symbols.empty())
        return std::string(); // Not a gui application, likely
    std::ostringstream callStr;
    callStr << std::showbase << std::hex << symbols.front().second
            << std::noshowbase << std::dec << '(' << x << ',' << y << ')';
    std::wstring wOutput;
    if (!ExtensionContext::instance().call(callStr.str(), &wOutput, errorMessage))
        return std::string();
    // Returns: ".call returns\nclass QWidget * 0x00000000`022bf100\nbla...".
    // Chop lines in front and after 'class ...' and convert first line.
    const std::wstring::size_type classPos = wOutput.find(L"class ");
    if (classPos == std::wstring::npos) {
        *errorMessage = msgWidgetParseError(wOutput);
        return std::string();
    }
    wOutput.erase(0, classPos + 6);
    const std::wstring::size_type nlPos = wOutput.find(L'\n');
    if (nlPos != std::wstring::npos)
        wOutput.erase(nlPos, wOutput.size() - nlPos);
    const std::string::size_type addressPos = wOutput.find(L" * 0x");
    if (addressPos == std::string::npos) {
        *errorMessage = msgWidgetParseError(wOutput);
        return std::string();
    }
    // "QWidget * 0x00000000`022bf100" -> "QWidget:0x00000000022bf100"
    wOutput.replace(addressPos, 3, L":");
    const std::string::size_type sepPos = wOutput.find(L'`');
    if (sepPos != std::string::npos)
        wOutput.erase(sepPos, 1);
    return wStringToString(wOutput);
}

static inline void formatGdbmiFlag(std::ostream &str, const char *name, bool v)
{
    str << name << "=\"" << (v ? "true" : "false") << '"';
}

static bool gdbmiFormatBreakpoint(std::ostream &str,
                                  IDebugBreakpoint *bp,
                                  CIDebugSymbols *symbols  /* = 0 */,
                                  unsigned verbose, std::string *errorMessage)
{
    enum { BufSize = 512 };
    ULONG64 offset = 0;
    ULONG flags = 0;
    ULONG id = 0;
    if (SUCCEEDED(bp->GetId(&id)))
        str << ",id=\"" << id << '"';
    HRESULT hr = bp->GetFlags(&flags);
    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("GetFlags", hr);
        return false;
    }
    const bool deferred = (flags & DEBUG_BREAKPOINT_DEFERRED) != 0;
    formatGdbmiFlag(str, ",deferred", deferred);
    formatGdbmiFlag(str, ",enabled", (flags & DEBUG_BREAKPOINT_ENABLED) != 0);
    if (verbose) {
        formatGdbmiFlag(str, ",oneshot", (flags & DEBUG_BREAKPOINT_ONE_SHOT) != 0);
        str << ",flags=\"" << flags << '"';
        ULONG threadId = 0;
        if (SUCCEEDED(bp->GetMatchThreadId(&threadId))) // Fails if none set
            str << ",thread=\"" << threadId << '"';
        ULONG passCount = 0;
        if (SUCCEEDED(bp->GetPassCount(&passCount)))
            str << ",passcount=\"" << passCount << '"';
    }
    // Offset: Fails for deferred ones
    if (!deferred && SUCCEEDED(bp->GetOffset(&offset))) {
        str << ",address=\"" << std::hex << std::showbase << offset
            << std::dec << std::noshowbase << '"';
        if (symbols) {
            const std::string module = moduleNameByOffset(symbols, offset);
            if (!module.empty())
                str << ",module=\"" << module << '"';
        }
    }
    // Expression
    if (verbose > 1) {
        char buf[BufSize];
        if (SUCCEEDED(bp->GetOffsetExpression(buf, BUFSIZ, 0)))
            str << ",expression=\"" << gdbmiStringFormat(buf) << '"';
    }
    return true;
}

// Format breakpoints as GDBMI
std::string gdbmiBreakpoints(CIDebugControl *ctrl,
                             CIDebugSymbols *symbols /* = 0 */,
                             bool humanReadable, unsigned verbose, std::string *errorMessage)
{
    ULONG breakPointCount = 0;
    HRESULT hr = ctrl->GetNumberBreakpoints(&breakPointCount);
    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("GetNumberBreakpoints", hr);
        return std::string();
    }
    std::ostringstream str;
    str << '[';
    if (humanReadable)
        str << '\n';
    for (ULONG i = 0; i < breakPointCount; i++) {
        str << "{number=\"" << i << '"';
        IDebugBreakpoint *bp = 0;
        hr = ctrl->GetBreakpointByIndex(i, &bp);
        if (FAILED(hr) || !bp) {
            *errorMessage = msgDebugEngineComFailed("GetBreakpointByIndex", hr);
            return std::string();
        }
        if (!gdbmiFormatBreakpoint(str, bp, symbols, verbose, errorMessage))
            return std::string();
        str << '}';
        if (humanReadable)
            str << '\n';
    }
    str << ']';
    return str.str();
}
