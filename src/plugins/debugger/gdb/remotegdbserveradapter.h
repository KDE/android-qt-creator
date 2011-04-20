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

#ifndef DEBUGGER_REMOTEGDBADAPTER_H
#define DEBUGGER_REMOTEGDBADAPTER_H

#include "abstractgdbadapter.h"
#include "localgdbprocess.h"

#include <projectexplorer/abi.h>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// RemoteGdbAdapter
//
///////////////////////////////////////////////////////////////////////

class RemoteGdbServerAdapter : public AbstractGdbAdapter
{
    Q_OBJECT

public:
    RemoteGdbServerAdapter(GdbEngine *engine, const ProjectExplorer::Abi &abi, QObject *parent = 0);

private:
    DumperHandling dumperHandling() const;

    void startAdapter();
    void setupInferior();
    void runEngine();
    void interruptInferior();
    void shutdownInferior();
    void shutdownAdapter();

    AbstractGdbProcess *gdbProc() { return &m_gdbProc; }

    void handleSetupDone();

signals:
    /*
     * For "external" clients of a debugger run control that need to do
     * further setup before the debugger is started (e.g. Maemo).
     * Afterwards, handleSetupDone() or handleSetupFailed() must be called
     * to continue or abort debugging, respectively.
     * This signal is only emitted if the start parameters indicate that
     * a server start script should be used, but none is given.
     */
    void requestSetup();

private:
    Q_SLOT void readUploadStandardOutput();
    Q_SLOT void readUploadStandardError();
    Q_SLOT void uploadProcError(QProcess::ProcessError error);
    Q_SLOT void uploadProcFinished();

    virtual void handleRemoteSetupDone(int gdbServerPort, int qmlPort);
    virtual void handleRemoteSetupFailed(const QString &reason);

    void handleSetTargetAsync(const GdbResponse &response);
    void handleFileExecAndSymbols(const GdbResponse &response);
    void callTargetRemote();
    void handleTargetRemote(const GdbResponse &response);
    void handleInterruptInferior(const GdbResponse &response);

    const ProjectExplorer::Abi m_abi;

    QProcess m_uploadProc;
    LocalGdbProcess m_gdbProc;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_PLAINGDBADAPTER_H
