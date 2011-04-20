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

#ifndef DEBUGGERRUNNER_H
#define DEBUGGERRUNNER_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/runconfiguration.h>

#include <QtCore/QScopedPointer>
#include <QtCore/QPair>
#include <QtCore/QStringList>

namespace Utils {
class Environment;
}

namespace Debugger {
class DebuggerEngine;
class DebuggerRunControl;
class DebuggerStartParameters;

namespace Internal {
class DebuggerRunControlPrivate;
class DebuggerRunControlFactory;
} // namespace Internal


class DEBUGGER_EXPORT ConfigurationCheck
{
public:
    ConfigurationCheck();
    operator bool() const;
    QString errorDetailsString() const;

    QString errorMessage;
    QStringList errorDetails;
    QString settingsCategory;
    QString settingsPage;
    QPair<DebuggerEngineType, DebuggerEngineType> masterSlaveEngineTypes;
};

DEBUGGER_EXPORT ConfigurationCheck checkDebugConfiguration(const DebuggerStartParameters &sp);

// This is a job description containing all data "local" to the jobs, including
// the models of the individual debugger views.
class DEBUGGER_EXPORT DebuggerRunControl
    : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    typedef ProjectExplorer::RunConfiguration RunConfiguration;
    explicit DebuggerRunControl(RunConfiguration *runConfiguration,
                                const DebuggerStartParameters &sp,
                                const QPair<DebuggerEngineType, DebuggerEngineType> &masterSlaveEngineTypes);

    ~DebuggerRunControl();

    // ProjectExplorer::RunControl
    void start();
    bool promptToStop(bool *prompt = 0) const;
    StopResult stop(); // Called from SnapshotWindow.
    bool isRunning() const;
    QString displayName() const;

    void setCustomEnvironment(Utils::Environment env);
    void startFailed();
    void debuggingFinished();
    RunConfiguration *runConfiguration() const;
    DebuggerEngine *engine(); // FIXME: Remove. Only used by Maemo support.

    void showMessage(const QString &msg, int channel);

signals:
    void engineRequestSetup();

private slots:
    void handleFinished();

protected:
    const DebuggerStartParameters &startParameters() const;

private:
    friend class Internal::DebuggerRunControlFactory;
    QScopedPointer<Internal::DebuggerRunControlPrivate> d;
};

} // namespace Debugger

#endif // DEBUGGERRUNNER_H
