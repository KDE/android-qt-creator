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

#include "stacktracecontext.h"
#include "symbolgroupcontext.h"
#include "corebreakpoint.h"
#include "coreengine.h"

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QScopedArrayPointer>

enum { debug = 0 };

namespace CdbCore {

StackFrame::StackFrame() :
    line(0), address(0)
{
}

QString StackFrame::toString() const
{
    QString rc;
    QTextStream str(&rc);
    format(str);
    return rc;
}

QDebug operator<<(QDebug d, const StackFrame &f)
{
    d.nospace() << f.toString();
    return d;
}

void StackFrame::format(QTextStream &str) const
{
    // left-pad level
    if (hasFile())
        str << QDir::toNativeSeparators(fileName) << ':' << line << " (";
    if (!module.isEmpty())
        str << module << '!';
    str << function;
    if (hasFile())
        str << ')';
    str.setIntegerBase(16);
    str << " 0x" << address;
    str.setIntegerBase(10);
}

Thread::Thread(unsigned long i, unsigned long si) :
        id(i), systemId(si), dataOffset(0)
{
}

QString Thread::toString() const
{
    QString rc;
    QTextStream str(&rc);
    str << "Thread id " << id << " System id " << systemId
        << " name='" << name <<"' Data at 0x";
    str.setIntegerBase(16);
    str << dataOffset;
    return rc;
}

QDebug operator<<(QDebug d, const Thread &t)
{
    d.nospace() << t.toString();
    return d;
}

// Check for special functions
StackTraceContext::SpecialFunction StackTraceContext::specialFunction(const QString &module,
                                                                      const QString &function)
{
    if (module == QLatin1String("ntdll")) {
        if (function == QLatin1String("DbgBreakPoint"))
            return BreakPointFunction;
        if (function == QLatin1String("KiFastSystemCallRet"))
            return KiFastSystemCallRet;
        if (function.startsWith("ZwWaitFor"))
            return WaitFunction;
    }
    if (module == QLatin1String("kernel32")) {
        if (function == QLatin1String("MsgWaitForMultipleObjects"))
            return WaitFunction;
        if (function.startsWith(QLatin1String("WaitFor")))
            return WaitFunction;
    }
    return None;
}

StackTraceContext::StackTraceContext(const ComInterfaces *cif) :
        m_cif(cif),
        m_instructionOffset(0),
        m_lastIndex(-1)
{
}

StackTraceContext *StackTraceContext::create(const ComInterfaces *cif,
                                             unsigned long maxFramesIn,
                                             QString *errorMessage)
{    
    StackTraceContext *ctx = new StackTraceContext(cif);
    if (!ctx->init(maxFramesIn, errorMessage)) {
        delete ctx;
        return 0;

    }
    return ctx;
}

StackTraceContext::~StackTraceContext()
{
    qDeleteAll(m_frameContexts);
}

// Convert the DEBUG_STACK_FRAMEs to our StackFrame structure
StackFrame StackTraceContext::frameFromFRAME(const CdbCore::ComInterfaces &cif,
                                             const DEBUG_STACK_FRAME &s)
{
    static WCHAR wszBuf[MAX_PATH];
    StackFrame frame;
    frame.address = s.InstructionOffset;
    cif.debugSymbols->GetNameByOffsetWide(frame.address, wszBuf, MAX_PATH, 0, 0);
    // Determine function and module, if available ("Qt4Core!foo").
    const QString moduleFunction = QString::fromUtf16(reinterpret_cast<const ushort *>(wszBuf));
    const int moduleSepPos = moduleFunction.indexOf(QLatin1Char('!'));
    if (moduleSepPos == -1) {
        frame.function = moduleFunction;
    } else {
        frame.module = moduleFunction.left(moduleSepPos);
        frame.function = moduleFunction.mid(moduleSepPos + 1);
    }
    ULONG64 ul64Displacement;
    const HRESULT hr = cif.debugSymbols->GetLineByOffsetWide(frame.address, &frame.line, wszBuf, MAX_PATH, 0, &ul64Displacement);
    if (SUCCEEDED(hr)) {
        const QString rawName = QString::fromUtf16(reinterpret_cast<const ushort *>(wszBuf));
        if (!rawName.isEmpty())
            frame.fileName = BreakPoint::normalizeFileName(rawName);
    }
    return frame;
}

bool StackTraceContext::init(unsigned long maxFramesIn, QString *errorMessage)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << maxFramesIn;

    // fill the DEBUG_STACK_FRAME array
    ULONG frameCount;
    const unsigned long effectiveMaxFrames = qMin(maxFramesIn, unsigned long(StackTraceContext::maxFrames));
    const HRESULT hr = m_cif->debugControl->GetStackTrace(0, 0, 0, m_cdbFrames,
                                                        effectiveMaxFrames,
                                                        &frameCount);
    if (FAILED(hr)) {
         *errorMessage = CdbCore::msgComFailed("GetStackTrace", hr);
        return false;
    }

    // Adapt group cache.
    m_frameContexts.resize(frameCount);
    qFill(m_frameContexts, static_cast<SymbolGroupContext*>(0));    
    // Convert the DEBUG_STACK_FRAMEs to our StackFrame structure and populate the frames
    for (ULONG i=0; i < frameCount; ++i)
        m_frames.push_back(frameFromFRAME(*m_cif, m_cdbFrames[i]));
    m_instructionOffset = m_frames.empty() ? ULONG64(0) : m_frames.front().address;
    return true;
}

int StackTraceContext::indexOf(const QString &function,
                               const QString &module /* = QString() */) const
{    
    const bool noModuleMatch = module.isEmpty();
    const int count = m_frames.size();
    for (int i = 0; i < count; i++) {
        if (m_frames.at(i).function == function
            && (noModuleMatch || module == m_frames.at(i).module))
            return i;
    }
    return -1;
}

QString StackTraceContext::msgFrameContextFailed(int index, const StackFrame &f, const QString &why)
{
    return QString::fromLatin1("Unable to create stack frame context #%1, %2!%3:%4 (%5): %6").
            arg(index).arg(f.module).arg(f.function).arg(f.line).arg(f.fileName, why);
}

SymbolGroupContext *StackTraceContext::createSymbolGroup(const ComInterfaces &cif,
                                                         int /* index */,
                                                         const QString &prefix,
                                                         CIDebugSymbolGroup *comSymbolGroup,
                                                         QString *errorMessage)
{
    return SymbolGroupContext::create(prefix, comSymbolGroup, cif.debugDataSpaces,
                                      QStringList(), errorMessage);
}

SymbolGroupContext *StackTraceContext::symbolGroupContextAt(int index, QString *errorMessage)
{
    // Create a frame on demand
    if (debug)
        qDebug() << Q_FUNC_INFO << index;

    if (index < 0 || index >= m_frameContexts.size()) {
        *errorMessage = QString::fromLatin1("%1: Index %2 out of range %3.").
                        arg(QLatin1String(Q_FUNC_INFO)).arg(index).arg(m_frameContexts.size());
        return 0;
    }
    if (m_frameContexts.at(index)) {
        // Symbol group only functions correctly if IDebugSymbols has the right scope.
        if (m_lastIndex != index) {
            if (!setScope(index, errorMessage))
                return 0;
            m_lastIndex = index;
        }
        return m_frameContexts.at(index);
    }
    CIDebugSymbolGroup *comSymbolGroup  = createCOM_SymbolGroup(index, errorMessage);
    if (!comSymbolGroup) {
        *errorMessage = msgFrameContextFailed(index, m_frames.at(index), *errorMessage);
        return 0;
    }
    SymbolGroupContext *sc = createSymbolGroup(*m_cif, index, QLatin1String("local"),
                                               comSymbolGroup, errorMessage);
    if (!sc) {
        *errorMessage = msgFrameContextFailed(index, m_frames.at(index), *errorMessage);
        return 0;
    }
    m_frameContexts[index] = sc;
    return sc;
}

bool StackTraceContext::setScope(int index, QString *errorMessage)
{
    if (debug)
        qDebug() << "setScope" << index;
    const HRESULT hr = m_cif->debugSymbols->SetScope(0, m_cdbFrames + index, NULL, 0);
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Cannot set scope %1: %2").
                        arg(index).arg(CdbCore::msgComFailed("SetScope", hr));
        return false;
    }
    return true;
}

CIDebugSymbolGroup *StackTraceContext::createCOM_SymbolGroup(int index, QString *errorMessage)
{
    CIDebugSymbolGroup *sg = 0;
    HRESULT hr = m_cif->debugSymbols->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, NULL, &sg);
    if (FAILED(hr)) {
        *errorMessage = CdbCore::msgComFailed("GetScopeSymbolGroup", hr);
        return 0;
    }
    // Set debugSymbols's scope.
    if (!setScope(index, errorMessage)) {
        sg->Release();
        return 0;
    }
    // refresh with current frame
    hr = m_cif->debugSymbols->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, sg, &sg);
    if (FAILED(hr)) {
        *errorMessage = CdbCore::msgComFailed("GetScopeSymbolGroup", hr);
        sg->Release();
        return 0;
    }
    m_lastIndex = index;
    return sg;
}

QString StackTraceContext::toString() const
{
    QString rc;
    QTextStream str(&rc);
    format(str);
    return rc;
}

void StackTraceContext::format(QTextStream &str) const
{
    const int count = m_frames.count();
    const int defaultFieldWidth = str.fieldWidth();
    const QTextStream::FieldAlignment defaultAlignment = str.fieldAlignment();
    for (int f = 0; f < count; f++) {
        // left-pad level
        str << qSetFieldWidth(6) << left << f;
        str.setFieldWidth(defaultFieldWidth);
        str.setFieldAlignment(defaultAlignment);
        m_frames.at(f).format(str);
        str << '\n';
    }
}

// Thread state helper
static inline QString msgGetThreadStateFailed(unsigned long threadId, const QString &why)
{
    return QString::fromLatin1("Unable to determine the state of thread %1: %2").arg(threadId).arg(why);
}

// Determine information about thread. This changes the
// current thread to thread->id.
bool StackTraceContext::getStoppedThreadState(const CdbCore::ComInterfaces &cif,
                                              unsigned long id,
                                              StackFrame *topFrame,
                                              QString *errorMessage)
{
    enum { MaxFrames = 2 };
    ULONG currentThread;
    HRESULT hr = cif.debugSystemObjects->GetCurrentThreadId(&currentThread);
    if (FAILED(hr)) {
        *errorMessage = msgGetThreadStateFailed(id, CdbCore::msgComFailed("GetCurrentThreadId", hr));
        return false;
    }
    if (currentThread != id) {
        hr = cif.debugSystemObjects->SetCurrentThreadId(id);
        if (FAILED(hr)) {
            *errorMessage = msgGetThreadStateFailed(id, CdbCore::msgComFailed("SetCurrentThreadId", hr));
            return false;
        }
    }
    ULONG frameCount;
    // Ignore the top frame if it is "ntdll!KiFastSystemCallRet", which is
    // not interesting for display.
    DEBUG_STACK_FRAME frames[MaxFrames];
    hr = cif.debugControl->GetStackTrace(0, 0, 0, frames, MaxFrames, &frameCount);
    if (FAILED(hr)) {
        *errorMessage = msgGetThreadStateFailed(id, CdbCore::msgComFailed("GetStackTrace", hr));
        return false;
    }
    // Ignore the top frame if it is "ntdll!KiFastSystemCallRet", which is
    // not interesting for display.
    *topFrame = frameFromFRAME(cif, frames[0]);
    if (frameCount > 1
        && StackTraceContext::specialFunction(topFrame->module, topFrame->function) == KiFastSystemCallRet)
            *topFrame = frameFromFRAME(cif, frames[1]);
    return true;
}

static inline QString msgGetThreadsFailed(const QString &why)
{
    return QString::fromLatin1("Unable to determine the thread information: %1").arg(why);
}

bool StackTraceContext::getThreadList(const CdbCore::ComInterfaces &cif,
                                      QVector<Thread> *threads,
                                      ULONG *currentThreadId,
                                      QString *errorMessage)
{
    threads->clear();
    ULONG threadCount;
    *currentThreadId = 0;
    // Get count
    HRESULT hr= cif.debugSystemObjects->GetNumberThreads(&threadCount);
    if (FAILED(hr)) {
        *errorMessage= msgGetThreadsFailed(CdbCore::msgComFailed("GetNumberThreads", hr));
        return false;
    }
    // Get index of current
    if (!threadCount)
        return true;
    hr = cif.debugSystemObjects->GetCurrentThreadId(currentThreadId);
    if (FAILED(hr)) {
        *errorMessage= msgGetThreadsFailed(CdbCore::msgComFailed("GetCurrentThreadId", hr));
        return false;
    }
    // Get Identifiers
    threads->reserve(threadCount);
    QScopedArrayPointer<ULONG> ids(new ULONG[threadCount]);
    QScopedArrayPointer<ULONG> systemIds(new ULONG[threadCount]);
    hr = cif.debugSystemObjects->GetThreadIdsByIndex(0, threadCount, ids.data(), systemIds.data());
    if (FAILED(hr)) {
        *errorMessage= msgGetThreadsFailed(CdbCore::msgComFailed("GetThreadIdsByIndex", hr));
        return false;
    }
    // Create entries
    static WCHAR name[256];
    for (ULONG i= 0; i < threadCount ; i++) {
        const ULONG id = ids[i];
        Thread thread(id, systemIds[i]);
        if (ids[i] ==  *currentThreadId) { // More info for current
            ULONG64 offset;
            if (SUCCEEDED(cif.debugSystemObjects->GetCurrentThreadDataOffset(&offset)))
                thread.dataOffset = offset;
        }
        // Name
        ULONG bytesReceived = 0;
        hr = cif.debugAdvanced->GetSystemObjectInformation(DEBUG_SYSOBJINFO_THREAD_NAME_WIDE,
                                                           0, id, name,
                                                           sizeof(name), &bytesReceived);
        if (SUCCEEDED(hr) && bytesReceived)
            thread.name = QString::fromWCharArray(name);
        threads->push_back(thread);
    }
    return true;
}

bool StackTraceContext::getStoppedThreadFrames(const CdbCore::ComInterfaces &cif,
                                               ULONG currentThreadId,
                                               const QVector<Thread> &threads,
                                               QVector<StackFrame> *frames,
                                               QString *errorMessage)
{
    frames->clear();
    if (threads.isEmpty())
        return true;
    frames->reserve(threads.size());

    const int threadCount = threads.size();
    for (int i = 0; i < threadCount; i++) {
        StackFrame frame;
        if (!getStoppedThreadState(cif, threads.at(i).id, &frame, errorMessage)) {
            qWarning("%s\n", qPrintable(*errorMessage));
            errorMessage->clear();
        }
        frames->append(frame);
    }
    // Restore thread id
    if (threads.back().id != currentThreadId) {
        const HRESULT hr = cif.debugSystemObjects->SetCurrentThreadId(currentThreadId);
        if (FAILED(hr)) {
            *errorMessage= msgGetThreadsFailed(CdbCore::msgComFailed("SetCurrentThreadId", hr));
            return false;
        }
    }
    return true;
}

QDebug operator<<(QDebug d, const StackTraceContext &t)
{
    d.nospace() << t.toString();
    return d;
}

}
