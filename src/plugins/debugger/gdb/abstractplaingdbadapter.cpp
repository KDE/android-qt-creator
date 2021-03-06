/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "abstractplaingdbadapter.h"
#include "gdbmi.h"
#include "gdbengine.h"
#include "debuggerstartparameters.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"

#include <QDir>
#include <QFile>
#include <QTemporaryFile>

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::GdbCommandCallback>(&GdbAbstractPlainEngine::callback), \
    STRINGIFY(callback)

GdbAbstractPlainEngine::GdbAbstractPlainEngine(const DebuggerStartParameters &startParameters,
    DebuggerEngine *masterEngine)
    : GdbEngine(startParameters, masterEngine)
{}

void GdbAbstractPlainEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (!startParameters().processArgs.isEmpty()) {
        QString args = startParameters().processArgs;
        postCommand("-exec-arguments " + toLocalEncoding(args));
    }
    postCommand("-file-exec-and-symbols \"" + execFilePath() + '"',
        CB(handleFileExecAndSymbols));
}

void GdbAbstractPlainEngine::handleFileExecAndSymbols(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        handleInferiorPrepared();
    } else {
        QByteArray ba = response.data.findChild("msg").data();
        QString msg = fromLocalEncoding(ba);
        // Extend the message a bit in unknown cases.
        if (!ba.endsWith("File format not recognized"))
            msg = tr("Starting executable failed:\n") + msg;
        notifyInferiorSetupFailed(msg);
    }
}

void GdbAbstractPlainEngine::runEngine()
{
    postCommand("-exec-run", GdbEngine::RunRequest, CB(handleExecRun));
}

void GdbAbstractPlainEngine::handleExecRun(const GdbResponse &response)
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    if (response.resultClass == GdbResultRunning) {
        notifyEngineRunAndInferiorRunOk();
        //showStatusMessage(tr("Running..."));
        showMessage(_("INFERIOR STARTED"));
        showMessage(msgInferiorSetupOk(), StatusBar);
        // FIXME: That's the wrong place for it.
        if (debuggerCore()->boolSetting(EnableReverseDebugging))
            postCommand("target record");
    } else {
        QString msg = fromLocalEncoding(response.data.findChild("msg").data());
        //QTC_CHECK(status() == InferiorRunOk);
        //interruptInferior();
        showMessage(msg);
        notifyEngineRunFailed();
    }
}

} // namespace Debugger
} // namespace Internal
