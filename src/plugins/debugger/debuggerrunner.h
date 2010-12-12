/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGERRUNNER_H
#define DEBUGGERRUNNER_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <projectexplorer/runconfiguration.h>

#include <QtCore/QScopedPointer>

namespace Utils {
class Environment;
}

namespace Debugger {
class DebuggerEngine;
class DebuggerRunControl;
class DebuggerStartParameters;

namespace Internal {

class DebuggerRunControlPrivate;

class DebuggerRunControlFactory
    : public ProjectExplorer::IRunControlFactory
{
public:
    DebuggerRunControlFactory(QObject *parent, unsigned enabledEngines);

    // This is used by the "Non-Standard" scenarios, e.g. Attach to Core.
    // FIXME: What to do in case of a 0 runConfiguration?
    typedef ProjectExplorer::RunConfiguration RunConfiguration;
    typedef ProjectExplorer::RunControl RunControl;
    DebuggerRunControl *create(const DebuggerStartParameters &sp,
        RunConfiguration *runConfiguration = 0);

    // ProjectExplorer::IRunControlFactory
    // FIXME: Used by qmljsinspector.cpp:469
    RunControl *create(RunConfiguration *runConfiguration, const QString &mode);
    bool canRun(RunConfiguration *runConfiguration, const QString &mode) const;

private:
    QString displayName() const;
    QWidget *createConfigurationWidget(RunConfiguration *runConfiguration);

    unsigned m_enabledEngines;
};

} // namespace Internal


// This is a job description containing all data "local" to the jobs, including
// the models of the individual debugger views.
class DEBUGGER_EXPORT DebuggerRunControl
    : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    typedef ProjectExplorer::RunConfiguration RunConfiguration;
    DebuggerRunControl(RunConfiguration *runConfiguration,
        unsigned enabledEngines, const DebuggerStartParameters &sp);
    ~DebuggerRunControl();

    // ProjectExplorer::RunControl
    void start();
    bool aboutToStop() const;
    StopResult stop(); // Called from SnapshotWindow.
    bool isRunning() const;
    QString displayName() const;

    void setCustomEnvironment(Utils::Environment env);
    void startFailed();
    void debuggingFinished();
    RunConfiguration *runConfiguration() const;
    DebuggerEngine *engine();

    void showMessage(const QString &msg, int channel);

    static bool checkDebugConfiguration(int toolChain,
                                 QString *errorMessage,
                                 QString *settingsCategory = 0,
                                 QString *settingsPage = 0);
signals:
    void engineRequestSetup();

private slots:
    void handleFinished();

protected:
    const DebuggerStartParameters &startParameters() const;

private:
    QScopedPointer<Internal::DebuggerRunControlPrivate> d;
};

} // namespace Debugger

#endif // DEBUGGERRUNNER_H
