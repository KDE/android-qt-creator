/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#ifndef IANALYZERTOOL_H
#define IANALYZERTOOL_H

#include "analyzerbase_global.h"
#include "analyzerconstants.h"

#include <coreplugin/id.h>

#include <QtCore/QObject>

namespace ProjectExplorer {
class RunConfiguration;
}

namespace Analyzer {

class AnalyzerStartParameters;
class IAnalyzerOutputPaneAdapter;
class IAnalyzerEngine;


/**
 * This class represents an analyzation tool, e.g. "Valgrind Memcheck".
 *
 * Each tool can run in different run modes. The modes are specific to the mode.
 *
 * @code
 * bool YourPlugin::initialize(const QStringList &arguments, QString *errorString)
 * {
 *    AnalyzerManager::instance()->addTool(new MemcheckTool(this));
 *    return true;
 * }
 * @endcode
 */
class ANALYZER_EXPORT IAnalyzerTool : public QObject
{
    Q_OBJECT

public:
    explicit IAnalyzerTool(QObject *parent = 0);

    /// Returns a unique ID for this tool.
    virtual Core::Id id() const = 0;
    /// Returns a short user readable display name for this tool.
    virtual QString displayName() const = 0;
    /// Returns a user readable description name for this tool.
    virtual QString description() const = 0;
    /// Returns an id for the start action.
    virtual Core::Id actionId(StartMode mode) const
        { return defaultActionId(this, mode); }
    /// Returns the menu group the start action should go to.
    virtual Core::Id menuGroup(StartMode mode) const
        { return defaultMenuGroup(mode); }
    /// Returns a short user readable action name for this tool.
    virtual QString actionName(StartMode mode) const
        { return defaultActionName(this, mode); }

    /**
     * The mode in which this tool should preferably be run
     *
     * The memcheck tool, for example, requires debug symbols, hence DebugMode
     * is preferred. On the other hand, callgrind should look at optimized code,
     * hence ReleaseMode.
     */
    enum ToolMode {
        DebugMode,
        ReleaseMode,
        AnyMode
    };
    virtual ToolMode toolMode() const = 0;

    /// Convenience implementation.
    static Core::Id defaultMenuGroup(StartMode mode);
    static Core::Id defaultActionId(const IAnalyzerTool *tool, StartMode mode);
    static QString defaultActionName(const IAnalyzerTool *tool, StartMode mode);

    /// This gets called after all analyzation tools where initialized.
    virtual void extensionsInitialized() = 0;

    /// Creates all widgets used by the tool.
    /// Returns a control widget which will be shown in the status bar when
    /// this tool is selected. Must be non-zero.
    virtual QWidget *createWidgets() = 0;

    /// Returns a new engine for the given start parameters.
    /// Called each time the tool is launched.
    virtual IAnalyzerEngine *createEngine(const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration = 0) = 0;

    virtual void startTool(StartMode mode) = 0;

    /// Called when tools gets selected.
    virtual void toolSelected() const {}

    /// Called when tools gets deselected.
    virtual void toolDeselected() const {}
};

} // namespace Analyzer

#endif // IANALYZERTOOL_H
