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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_BREAKPOINT_H
#define DEBUGGER_BREAKPOINT_H

#include <QtCore/QDebug>
#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QString>

namespace Debugger {
namespace Internal {

//////////////////////////////////////////////////////////////////
//
// BreakpointModelId
//
//////////////////////////////////////////////////////////////////

class BreakpointModelId
{
public:
    BreakpointModelId() { m_majorPart = m_minorPart = 0; }
    explicit BreakpointModelId(quint16 ma) { m_majorPart = ma; m_minorPart = 0; }
    BreakpointModelId(quint16 ma, quint16 mi) { m_majorPart = ma; m_minorPart = mi; }
    explicit BreakpointModelId(const QByteArray &ba); // "21.2"

    bool isValid() const { return m_majorPart != 0; }
    bool isMajor() const { return m_majorPart != 0 && m_minorPart == 0; }
    bool isMinor() const { return m_majorPart != 0 && m_minorPart != 0; }
    bool operator!() const { return !isValid(); }
    operator const void*() const { return isValid() ? this : 0; }
    quint32 toInternalId() const { return m_majorPart | (m_minorPart << 16); }
    QByteArray toByteArray() const;
    QString toString() const;
    bool operator==(const BreakpointModelId &id) const
        { return m_majorPart == id.m_majorPart && m_minorPart == id.m_minorPart; }
    quint16 majorPart() const { return m_majorPart; }
    quint16 minorPart() const { return m_minorPart; }
    BreakpointModelId parent() const;
    BreakpointModelId child(int row) const;

    static BreakpointModelId fromInternalId(quint32 id)
        { return BreakpointModelId(id & 0xff, id >> 16); }

private:
    quint16 m_majorPart;
    quint16 m_minorPart;
};

QDebug operator<<(QDebug d, const BreakpointModelId &id);


//////////////////////////////////////////////////////////////////
//
// BreakpointResponseId
//
//////////////////////////////////////////////////////////////////

class BreakpointResponseId
{
public:
    BreakpointResponseId() { m_majorPart = m_minorPart = 0; }
    explicit BreakpointResponseId(quint16 ma) { m_majorPart = ma; m_minorPart = 0; }
    BreakpointResponseId(quint16 ma, quint16 mi) { m_majorPart = ma; m_minorPart = mi; }
    explicit BreakpointResponseId(const QByteArray &ba); // "21.2"

    bool isValid() const { return m_majorPart != 0; }
    bool isMajor() const { return m_majorPart != 0 && m_minorPart == 0; }
    bool isMinor() const { return m_majorPart != 0 && m_minorPart != 0; }
    bool operator!() const { return !isValid(); }
    operator const void*() const { return isValid() ? this : 0; }
    quint32 toInternalId() const { return m_majorPart | (m_minorPart << 16); }
    QByteArray toByteArray() const;
    QString toString() const;
    bool operator==(const BreakpointResponseId &id) const
        { return m_majorPart == id.m_majorPart && m_minorPart == id.m_minorPart; }
    quint16 majorPart() const { return m_majorPart; }
    quint16 minorPart() const { return m_minorPart; }
    BreakpointResponseId parent() const;
    BreakpointResponseId child(int row) const;

private:
    quint16 m_majorPart;
    quint16 m_minorPart;
};

QDebug operator<<(QDebug d, const BreakpointModelId &id);
QDebug operator<<(QDebug d, const BreakpointResponseId &id);

//////////////////////////////////////////////////////////////////
//
// BreakpointData
//
//////////////////////////////////////////////////////////////////

//! \enum Debugger::Internal::BreakpointType
enum BreakpointType
{
    UnknownType,
    BreakpointByFileAndLine,
    BreakpointByFunction,
    BreakpointByAddress,
    BreakpointAtThrow,
    BreakpointAtCatch,
    BreakpointAtMain,
    BreakpointAtFork,
    BreakpointAtExec,
    //BreakpointAtVFork,
    BreakpointAtSysCall,
    WatchpointAtAddress,
    WatchpointAtExpression,
    BreakpointOnSignalHandler
};

//! \enum Debugger::Internal::BreakpointState
enum BreakpointState
{
    BreakpointNew,
    BreakpointInsertRequested,  //!< Inferior was told about bp, not ack'ed.
    BreakpointInsertProceeding,
    BreakpointChangeRequested,
    BreakpointChangeProceeding,
    BreakpointInserted,
    BreakpointRemoveRequested,
    BreakpointRemoveProceeding,
    BreakpointDead
};

//! \enum Debugger::Internal::BreakpointPathUsage
enum BreakpointPathUsage
{
    BreakpointPathUsageEngineDefault, //!< Default value that suits the engine.
    BreakpointUseFullPath,            //!< Use full path to avoid ambiguities. Slow with gdb.
    BreakpointUseShortPath            //!< Use filename only, in case source files are relocated.
};

enum BreakpointParts
{
    NoParts = 0,
    FileAndLinePart = 0x1,
    FunctionPart = 0x2,
    AddressPart = 0x4,
    ExpressionPart = 0x8,
    ConditionPart = 0x10,
    IgnoreCountPart = 0x20,
    ThreadSpecPart = 0x40,
    AllConditionParts = ConditionPart|IgnoreCountPart|ThreadSpecPart,
    ModulePart = 0x80,
    TracePointPart = 0x100,

    EnabledPart = 0x200,
    TypePart = 0x400,
    PathUsagePart = 0x800,
    CommandPart = 0x1000,
    MessagePart = 0x2000,

    AllParts = FileAndLinePart|FunctionPart
               |ExpressionPart|AddressPart|ConditionPart
               |IgnoreCountPart|ThreadSpecPart|ModulePart|TracePointPart
               |EnabledPart|TypePart|PathUsagePart|CommandPart|MessagePart
};

inline void operator|=(BreakpointParts &p, BreakpointParts r)
{
    p = BreakpointParts(int(p) | int(r));
}


class BreakpointParameters
{
public:
    explicit BreakpointParameters(BreakpointType = UnknownType);
    BreakpointParts differencesTo(const BreakpointParameters &rhs) const;
    bool equals(const BreakpointParameters &rhs) const;
    bool conditionsMatch(const QByteArray &other) const;
    bool isWatchpoint() const
        { return type == WatchpointAtAddress || type == WatchpointAtExpression; }
    // Enough for now.
    bool isBreakpoint() const { return !isWatchpoint() && !isTracepoint(); }
    bool isTracepoint() const { return tracepoint; }
    QString toString() const;
    void updateLocation(const QByteArray &location); // file.cpp:42

    bool operator==(const BreakpointParameters &p) const { return equals(p); }
    bool operator!=(const BreakpointParameters &p) const { return !equals(p); }

    BreakpointType type;     //!< Type of breakpoint.
    bool enabled;            //!< Should we talk to the debugger engine?
    BreakpointPathUsage pathUsage;  //!< Should we use the full path when setting the bp?
    QString fileName;        //!< Short name of source file.
    QByteArray condition;    //!< Condition associated with breakpoint.
    int ignoreCount;         //!< Ignore count associated with breakpoint.
    int lineNumber;          //!< Line in source file.
    quint64 address;         //!< Address for address based data breakpoints.
    QString expression;      //!< Expression for expression based data breakpoints.
    uint size;               //!< Size of watched area for data breakpoints.
    uint bitpos;             //!< Location of watched bitfield within watched area.
    uint bitsize;            //!< Size of watched bitfield within watched area.
    int threadSpec;          //!< Thread specification.
    QString functionName;
    QString module;          //!< module for file name
    QString command;         //!< command to execute
    QString message;         //!< message
    bool tracepoint;
};

class BreakpointResponse : public BreakpointParameters
{
public:
    BreakpointResponse();
    QString toString() const;

public:
    void fromParameters(const BreakpointParameters &p);

    BreakpointResponseId id; //!< Breakpoint number assigned by the debugger engine.
    bool pending;            //!< Breakpoint not fully resolved.
    int hitCount;            //!< Number of times this has been hit.
    bool multiple;           //!< Happens in constructors/gdb.
    int correctedLineNumber; //!< Line number as seen by gdb.
};

typedef QList<BreakpointModelId> BreakpointModelIds;

inline uint qHash(const Debugger::Internal::BreakpointModelId &id)
{
    return id.toInternalId();
}

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::BreakpointModelId)
Q_DECLARE_METATYPE(Debugger::Internal::BreakpointResponseId)


#endif // DEBUGGER_BREAKPOINT_H
