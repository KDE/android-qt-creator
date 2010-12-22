/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef ANDROIDDEBUGSUPPORT_H
#define ANDROIDDEBUGSUPPORT_H

#include "androidrunconfiguration.h"

#include <coreplugin/ssh/sftpdefs.h>
#include <utils/environment.h>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>

namespace Core { class SftpChannel; }

namespace Debugger {
class DebuggerRunControl;
}

namespace ProjectExplorer { class RunControl; }

namespace Qt4ProjectManager {
namespace Internal {

class AndroidRunConfiguration;
class AndroidRunner;

class AndroidDebugSupport : public QObject
{
    Q_OBJECT
public:
    static ProjectExplorer::RunControl *createDebugRunControl(AndroidRunConfiguration *runConfig);

    AndroidDebugSupport(AndroidRunConfiguration *runConfig,
        Debugger::DebuggerRunControl *runControl);
    ~AndroidDebugSupport();

private slots:
    void handleRemoteProcessStarted(int gdbServerPort=-1, int qmlPort=-1);
    void handleRemoteProcessFinished(const QString & errorMsg);

    void handleRemoteOutput(const QByteArray &output);
    void handleRemoteErrorOutput(const QByteArray &output);

private:

    void handleAdapterSetupDone();

    const QPointer<Debugger::DebuggerRunControl> m_runControl;
    const QPointer<AndroidRunConfiguration> m_runConfig;
    AndroidRunner * const m_runner;
    const AndroidRunConfiguration::DebuggingType m_debuggingType;
    const QString m_dumperLib;

    int m_gdbServerPort;
    int m_qmlPort;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDDEBUGSUPPORT_H
