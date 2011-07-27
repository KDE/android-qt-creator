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

#include "callgrindengine.h"

#include "valgrindsettings.h"

#include <valgrind/callgrind/callgrindcontroller.h>
#include <valgrind/callgrind/callgrindparser.h>

#include <analyzerbase/analyzermanager.h>

#include <utils/qtcassert.h>

using namespace Analyzer;
using namespace Valgrind;
using namespace Valgrind::Internal;

CallgrindEngine::CallgrindEngine(IAnalyzerTool *tool, const AnalyzerStartParameters &sp,
         ProjectExplorer::RunConfiguration *runConfiguration)
    : ValgrindEngine(tool, sp, runConfiguration)
    , m_markAsPaused(false)
{
    connect(&m_runner, SIGNAL(finished()), this, SLOT(slotFinished()));
    connect(&m_runner, SIGNAL(started()), this, SLOT(slotStarted()));
    connect(m_runner.parser(), SIGNAL(parserDataReady()), this, SLOT(slotFinished()));
    connect(&m_runner, SIGNAL(statusMessage(QString)), SLOT(showStatusMessage(QString)));

    m_progress->setProgressRange(0, 2);
}

void CallgrindEngine::showStatusMessage(const QString &msg)
{
    AnalyzerManager::showStatusMessage(msg);
}

QStringList CallgrindEngine::toolArguments() const
{
    QStringList arguments;

    ValgrindBaseSettings *callgrindSettings = m_settings->subConfig<ValgrindBaseSettings>();
    QTC_ASSERT(callgrindSettings, return arguments);

    if (callgrindSettings->enableCacheSim())
        arguments << "--cache-sim=yes";

    if (callgrindSettings->enableBranchSim())
        arguments << "--branch-sim=yes";

    if (callgrindSettings->collectBusEvents())
        arguments << "--collect-bus=yes";

    if (callgrindSettings->collectSystime())
        arguments << "--collect-systime=yes";

    if (m_markAsPaused)
        arguments << "--instr-atstart=no";

    // add extra arguments
    arguments << extraArguments();

    return arguments;
}

QString CallgrindEngine::progressTitle() const
{
    return tr("Profiling");
}

Valgrind::ValgrindRunner * CallgrindEngine::runner()
{
    return &m_runner;
}

bool CallgrindEngine::start()
{
    emit outputReceived(tr("Profiling %1\n").arg(executable()), Utils::NormalMessageFormat);
    return ValgrindEngine::start();
}

void CallgrindEngine::dump()
{
    m_runner.controller()->run(Valgrind::Callgrind::CallgrindController::Dump);
}

void CallgrindEngine::setExtraArguments(const QStringList &extraArguments)
{
    m_extraArguments = extraArguments;
}

void CallgrindEngine::setPaused(bool paused)
{
    if (m_markAsPaused == paused)
        return;

    m_markAsPaused = paused;

    // call controller only if it is attached to a valgrind process
    if (m_runner.controller()->valgrindProcess()) {
        if (paused)
            pause();
        else
            unpause();
    }
}

void CallgrindEngine::setToggleCollectFunction(const QString &toggleCollectFunction)
{
    if (toggleCollectFunction.isEmpty())
        return;

    m_extraArguments << QString("--toggle-collect=%1").arg(toggleCollectFunction);
}

void CallgrindEngine::reset()
{
    m_runner.controller()->run(Valgrind::Callgrind::CallgrindController::ResetEventCounters);
}

void CallgrindEngine::pause()
{
    m_runner.controller()->run(Valgrind::Callgrind::CallgrindController::Pause);
}

void CallgrindEngine::unpause()
{
    m_runner.controller()->run(Valgrind::Callgrind::CallgrindController::UnPause);
}

Valgrind::Callgrind::ParseData *CallgrindEngine::takeParserData()
{
    return m_runner.parser()->takeData();
}

void CallgrindEngine::slotFinished()
{
    emit parserDataReady(this);
}

void CallgrindEngine::slotStarted()
{
    m_progress->setProgressValue(1);;
}
