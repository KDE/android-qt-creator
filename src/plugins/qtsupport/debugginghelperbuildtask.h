/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DEBUGGINGHELPERBUILDTASK_H
#define DEBUGGINGHELPERBUILDTASK_H

#include "qtsupport_global.h"
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <projectexplorer/toolchain.h>

#include <QtCore/QObject>
#include <QtCore/QFutureInterface>
#include <QtCore/QMetaType>

namespace ProjectExplorer {
class ToolChain;
} // namespace ProjectExplorer

namespace QtSupport {
class BaseQtVersion;

class QTSUPPORT_EXPORT DebuggingHelperBuildTask : public QObject
{
    Q_OBJECT

public:
    enum DebuggingHelper {
        GdbDebugging = 0x01,
        QmlDebugging = 0x02,
        QmlObserver = 0x04,
        QmlDump = 0x08,
        AllTools = GdbDebugging | QmlDebugging | QmlObserver | QmlDump
    };
    Q_DECLARE_FLAGS(Tools, DebuggingHelper)

    explicit DebuggingHelperBuildTask(const BaseQtVersion *version,
                                      ProjectExplorer::ToolChain *toolChain,
                                      Tools tools = AllTools);
    virtual ~DebuggingHelperBuildTask();

    void showOutputOnError(bool show);
    void run(QFutureInterface<void> &future);

    static Tools availableTools(const BaseQtVersion *version);

signals:
    void finished(int qtVersionId, const QString &output, DebuggingHelperBuildTask::Tools tools);

    // used internally
    void logOutput(const QString &output, bool bringToForeground);
    void updateQtVersions(const Utils::FileName &qmakeCommand);

private:
    bool buildDebuggingHelper(QFutureInterface<void> &future);
    void log(const QString &output, const QString &error);

    const Tools m_tools;

    int m_qtId;
    QString m_qtInstallData;
    QString m_target;
    Utils::FileName m_qmakeCommand;
    QString m_makeCommand;
    QStringList m_makeArguments;
    Utils::FileName m_mkspec;
    Utils::Environment m_environment;
    QString m_log;
    bool m_invalidQt;
    bool m_showErrors;
};

} // namespace Qt4ProjectManager

Q_DECLARE_METATYPE(QtSupport::DebuggingHelperBuildTask::Tools)

#endif // DEBUGGINGHELPERBUILDTASK_H
