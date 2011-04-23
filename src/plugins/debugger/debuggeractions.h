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

#ifndef DEBUGGER_ACTIONS_H
#define DEBUGGER_ACTIONS_H

#include <QtCore/QHash>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {
class SavedAction;
}

namespace Debugger {
namespace Internal {

// Global debugger options that are not stored as saved action.
class GlobalDebuggerOptions
{
public:
    typedef QMap<QString, QString> SourcePathMap;

    void toSettings(QSettings *) const;
    void fromSettings(QSettings *);
    bool equals(const GlobalDebuggerOptions &rhs) const { return sourcePathMap == rhs.sourcePathMap; }

    SourcePathMap sourcePathMap;
};

inline bool operator==(const GlobalDebuggerOptions &o1, const GlobalDebuggerOptions &o2) { return o1.equals(o2); }
inline bool operator!=(const GlobalDebuggerOptions &o1, const GlobalDebuggerOptions &o2) { return !o1.equals(o2); }

class DebuggerSettings : public QObject
{
    Q_OBJECT // For tr().
public:
    explicit DebuggerSettings(QSettings *setting);
    ~DebuggerSettings();

    void insertItem(int code, Utils::SavedAction *item);
    Utils::SavedAction *item(int code) const;

    QString dump() const;

    void readSettings();
    void writeSettings() const;

private:
    QHash<int, Utils::SavedAction *> m_items;
    QSettings *m_settings;
};

///////////////////////////////////////////////////////////

enum DebuggerActionCode
{
    // General
    SettingsDialog,
    UseAlternatingRowColors,
    UseMessageBoxForSignals,
    AutoQuit,
    LockView,
    LogTimeStamps,
    VerboseLog,
    OperateByInstruction,
    CloseBuffersOnExit,
    SwitchModeOnExit,

    UseDebuggingHelpers,

    UseCodeModel,
    ShowThreadNames,

    UseToolTipsInMainEditor,
    UseToolTipsInLocalsView,
    UseToolTipsInBreakpointsView,
    UseToolTipsInStackView,
    UseAddressInBreakpointsView,
    UseAddressInStackView,

    RegisterForPostMortem,

    // Gdb
    LoadGdbInit,
    GdbScriptFile,
    GdbWatchdogTimeout,
    TargetAsync,

    // Stack
    MaximalStackDepth,
    ExpandStack,
    CreateFullBacktrace,
    AlwaysAdjustStackColumnWidths,

    // Watchers & Locals
    ShowStdNamespace,
    ShowQtNamespace,
    SortStructMembers,
    AutoDerefPointers,
    AlwaysAdjustLocalsColumnWidths,

    // Source List
    ListSourceFiles,

    // Running
    SkipKnownFrames,
    EnableReverseDebugging,

    // Breakpoints
    SynchronizeBreakpoints,
    AllPluginBreakpoints,
    SelectedPluginBreakpoints,
    AdjustBreakpointLocations,
    AlwaysAdjustBreakpointsColumnWidths,
    NoPluginBreakpoints,
    SelectedPluginBreakpointsPattern,
    BreakOnThrow,
    BreakOnCatch,

    // Registers
    AlwaysAdjustRegistersColumnWidths,

    // Snapshots
    AlwaysAdjustSnapshotsColumnWidths,

    // Threads
    AlwaysAdjustThreadsColumnWidths,

    // Modules
    AlwaysAdjustModulesColumnWidths
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_WATCHWINDOW_H
