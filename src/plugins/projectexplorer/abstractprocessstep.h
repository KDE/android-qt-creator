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
// Documentation inside.
class PROJECTEXPLORER_EXPORT AbstractProcessStep : public BuildStep
{
    Q_OBJECT

public:
    virtual ~AbstractProcessStep();

    virtual bool init();
    virtual void run(QFutureInterface<bool> &);

    virtual BuildStepConfigWidget *createConfigWidget() = 0;
    virtual bool immutable() const = 0;

    void setEnabled(bool b) { m_enabled = b; }

    ProcessParameters *processParameters() { return &m_param; }

    void setIgnoreReturnValue(bool b);

    void setOutputParser(ProjectExplorer::IOutputParser *parser);
    void appendOutputParser(ProjectExplorer::IOutputParser *parser);
    ProjectExplorer::IOutputParser *outputParser() const;

protected:
    AbstractProcessStep(BuildStepList *bsl, const QString &id);
    AbstractProcessStep(BuildStepList *bsl, AbstractProcessStep *bs);

    virtual void processStarted();
    virtual void processFinished(int exitCode, QProcess::ExitStatus status);
    virtual void processStartupFailed();
    virtual bool processSucceeded(int exitCode, QProcess::ExitStatus status);
    virtual void stdOutput(const QString &line);
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
