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

#ifndef ABSTRACTPROCESSSTEP_H
#define ABSTRACTPROCESSSTEP_H

#include "buildstep.h"
#include "processparameters.h"

#include <utils/environment.h>

#include <utils/qtcprocess.h>

#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QEventLoop;
class QTimer;
QT_END_NAMESPACE

namespace ProjectExplorer {

class IOutputParser;

/*!
  AbstractProcessStep is a convenience class, which can be used as a base class instead of BuildStep.
  It should be used as a base class if your buildstep just needs to run a process.

  Usage:
    Use processParameters() to configure the process you want to run
    (you need to do that before calling AbstractProcessStep::init()).
    Inside YourBuildStep::init() call AbstractProcessStep::init().
    Inside YourBuildStep::run() call AbstractProcessStep::run(), which automatically starts the proces
    and by default adds the output on stdOut and stdErr to the OutputWindow.
    If you need to process the process output override stdOut() and/or stdErr.
    The two functions processStarted() and processFinished() are called after starting/finishing the process.
    By default they add a message to the output window.

    Use setEnabled() to control whether the BuildStep needs to run. (A disabled BuildStep immediately returns true,
    from the run function.)

*/

class PROJECTEXPLORER_EXPORT AbstractProcessStep : public BuildStep
{
    Q_OBJECT

public:
    virtual ~AbstractProcessStep();

    /// reimplemented from BuildStep::init()
    /// You need to call this from YourBuildStep::init()
    virtual bool init();
    /// reimplemented from BuildStep::init()
    /// You need to call this from YourBuildStep::run()
    virtual void run(QFutureInterface<bool> &);

    virtual BuildStepConfigWidget *createConfigWidget() = 0;
    virtual bool immutable() const = 0;

    /// enables or disables a BuildStep
    /// Disabled BuildSteps immediately return true from their run method
    /// should be called from init()
    void setEnabled(bool b) { m_enabled = b; }

    /// obtain a reference to the parameters for the actual process to run.
    /// should be used in init()
    ProcessParameters *processParameters() { return &m_param; }

    /// If ignoreReturnValue is set to true, then the abstractprocess step will
    /// return success even if the return value indicates otherwise
    /// should be called from init
    void setIgnoreReturnValue(bool b);

    // derived classes needs to call this function
    /// Delete all existing output parsers and start a new chain with the
    /// given parser.
    void setOutputParser(ProjectExplorer::IOutputParser *parser);
    /// Append the given output parser to the existing chain of parsers.
    void appendOutputParser(ProjectExplorer::IOutputParser *parser);
    ProjectExplorer::IOutputParser *outputParser() const;

protected:
    AbstractProcessStep(BuildStepList *bsl, const QString &id);
    AbstractProcessStep(BuildStepList *bsl, AbstractProcessStep *bs);

    /// Called after the process is started
    /// the default implementation adds a process started message to the output message
    virtual void processStarted();
    /// Called after the process Finished
    /// the default implementation adds a line to the output window
    virtual void processFinished(int exitCode, QProcess::ExitStatus status);
    /// Called if the process could not be started,
    /// by default adds a message to the output window
    virtual void processStartupFailed();
    /// Called to test whether a prcess succeeded or not.
    virtual bool processSucceeded(int exitCode, QProcess::ExitStatus status);
    /// Called for each line of output on stdOut()
    /// the default implementation adds the line to the
    /// application output window
    virtual void stdOutput(const QString &line);
    /// Called for each line of output on StdErrror()
    /// the default implementation adds the line to the
    /// application output window
    virtual void stdError(const QString &line);

private slots:
    void processReadyReadStdOutput();
    void processReadyReadStdError();
    void slotProcessFinished(int, QProcess::ExitStatus);
    void checkForCancel();

    void taskAdded(const ProjectExplorer::Task &task);

    void outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);

private:
    QTimer *m_timer;
    QFutureInterface<bool> *m_futureInterface;
    ProcessParameters m_param;
    bool m_enabled;
    bool m_ignoreReturnValue;
    Utils::QtcProcess *m_process;
    QEventLoop *m_eventLoop;
    ProjectExplorer::IOutputParser *m_outputParserChain;
};

} // namespace ProjectExplorer

#endif // ABSTRACTPROCESSSTEP_H
