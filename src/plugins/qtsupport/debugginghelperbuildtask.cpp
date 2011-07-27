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

#include "debugginghelperbuildtask.h"
#include "qmldumptool.h"
#include "qmlobservertool.h"
#include "qmldebugginglibrary.h"
#include "baseqtversion.h"
#include "qtversionmanager.h"
#include <coreplugin/messagemanager.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/abi.h>
#include <utils/qtcassert.h>

#include <QtCore/QCoreApplication>

using namespace QtSupport;
using namespace QtSupport::Internal;
using ProjectExplorer::DebuggingHelperLibrary;

DebuggingHelperBuildTask::DebuggingHelperBuildTask(const BaseQtVersion *version,
                                                   ProjectExplorer::ToolChain *toolChain,
                                                   Tools tools) :
    m_tools(tools & availableTools(version)),
    m_invalidQt(false),
    m_showErrors(true)
{
    if (!version || !version->isValid() || !toolChain) {
        m_invalidQt = true;
        return;
    }
    // allow type to be used in queued connections.
    qRegisterMetaType<DebuggingHelperBuildTask::Tools>("DebuggingHelperBuildTask::Tools");

    // Print result in application ouptut
    Core::MessageManager *messageManager = Core::MessageManager::instance();
    connect(this, SIGNAL(logOutput(QString,bool)),
            messageManager, SLOT(printToOutputPane(QString,bool)),
            Qt::QueuedConnection);

    //
    // Extract all information we need from version, such that we don't depend on the existence
    // of the version pointer while compiling
    //
    m_qtId = version->uniqueId();
    m_qtInstallData = version->versionInfo().value("QT_INSTALL_DATA");
    if (m_qtInstallData.isEmpty()) {
        const QString error
                = QCoreApplication::translate(
                    "QtVersion",
                    "Cannot determine the installation path for Qt version '%1'."
                    ).arg(version->displayName());
        log(QString(), error);
        m_invalidQt = true;
        return;
    }

    m_environment = Utils::Environment::systemEnvironment();
    version->addToEnvironment(m_environment);

    toolChain->addToEnvironment(m_environment);

    log(QCoreApplication::translate("QtVersion", "Building helper(s) with toolchain '%1' ...\n"
                                    ).arg(toolChain->displayName()), QString());

    if (toolChain->targetAbi().os() == ProjectExplorer::Abi::LinuxOS
        && ProjectExplorer::Abi::hostAbi().os() == ProjectExplorer::Abi::WindowsOS)
        m_target = QLatin1String("-unix");
    m_qmakeCommand = version->qmakeCommand();
    m_makeCommand = toolChain->makeCommand();
    m_mkspec = version->mkspec();

    // Make sure QtVersion cache is invalidated
    connect(this, SIGNAL(finished(int,QString,DebuggingHelperBuildTask::Tools)),
            QtVersionManager::instance(), SLOT(updateQtVersion(int)),
            Qt::QueuedConnection);
}

DebuggingHelperBuildTask::~DebuggingHelperBuildTask()
{
}

DebuggingHelperBuildTask::Tools DebuggingHelperBuildTask::availableTools(const BaseQtVersion *version)
{
    QTC_ASSERT(version, return 0; )
    // Check the build requirements of the tools
    DebuggingHelperBuildTask::Tools tools = 0;
    // Gdb helpers are needed on Mac/gdb only.
    foreach (const ProjectExplorer::Abi &abi, version->qtAbis()) {
        if (abi.os() == ProjectExplorer::Abi::MacOS) {
            tools |= DebuggingHelperBuildTask::GdbDebugging;
            break;
        }
    }
    if (QmlDumpTool::canBuild(version))
        tools |= QmlDump;
    if (QmlDebuggingLibrary::canBuild(version)) {
        tools |= QmlDebugging;
        if (QmlObserverTool::canBuild(version))
            tools |= QmlObserver; // requires QML debugging.
    }
    return tools;
}

void DebuggingHelperBuildTask::showOutputOnError(bool show)
{
    m_showErrors = show;
}

void DebuggingHelperBuildTask::run(QFutureInterface<void> &future)
{
    future.setProgressRange(0, 5);
    future.setProgressValue(1);

    if (m_invalidQt || !buildDebuggingHelper(future)) {
        const QString error
                = QCoreApplication::translate(
                    "QtVersion",
                    "Build failed.");
        log(QString(), error);
    } else {
        const QString result
                = QCoreApplication::translate(
                    "QtVersion",
                    "Build succeeded.");
        log(result, QString());
    }

    emit finished(m_qtId, m_log, m_tools);
    deleteLater();
}

bool DebuggingHelperBuildTask::buildDebuggingHelper(QFutureInterface<void> &future)
{
    Utils::BuildableHelperLibrary::BuildHelperArguments arguments;
    arguments.makeCommand = m_makeCommand;
    arguments.qmakeCommand = m_qmakeCommand;
    arguments.qmakeArguments = QStringList() << QLatin1String("-nocache");
    arguments.targetMode = m_target;
    arguments.mkspec = m_mkspec;
    arguments.environment = m_environment;

    if (m_tools & GdbDebugging) {
        QString output, error;
        bool success = true;

        arguments.directory = DebuggingHelperLibrary::copy(m_qtInstallData, &error);
        if (arguments.directory.isEmpty()
                || !DebuggingHelperLibrary::build(arguments, &output, &error))
            success = false;
        log(output, error);
        if (!success)
            return false;
    }
    future.setProgressValue(2);

    if (m_tools & QmlDump) {
        QString output, error;
        bool success = true;

        arguments.directory = QmlDumpTool::copy(m_qtInstallData, &error);
        if (arguments.directory.isEmpty()
                || !QmlDumpTool::build(arguments, &output, &error))
            success = false;
        log(output, error);
        if (!success)
            return false;
    }
    future.setProgressValue(3);

    QString qmlDebuggingDirectory;
    if (m_tools & QmlDebugging) {
        QString output, error;

        qmlDebuggingDirectory = QmlDebuggingLibrary::copy(m_qtInstallData, &error);

        bool success = true;
        arguments.directory = qmlDebuggingDirectory;
        arguments.makeArguments += QLatin1String("all"); // build debug and release
        arguments.makeArguments += QLatin1String("-k"); // don't stop if one fails
        if (arguments.directory.isEmpty()
                || !QmlDebuggingLibrary::build(arguments, &output, &error)) {
            success = false;
        }

        log(output, error);
        if (!success) {
            return false;
        }
        arguments.makeArguments.clear();
    }
    future.setProgressValue(4);

    if (m_tools & QmlObserver) {
        QString output, error;
        bool success = true;

        arguments.directory = QmlObserverTool::copy(m_qtInstallData, &error);
        arguments.qmakeArguments << QLatin1String("INCLUDEPATH+=\"\\\"") + qmlDebuggingDirectory + "include\\\"\"";
        arguments.qmakeArguments << QLatin1String("LIBS+=-L\"\\\"") + qmlDebuggingDirectory + QLatin1String("\\\"\"");

        if (arguments.directory.isEmpty()
                || !QmlObserverTool::build(arguments, &output, &error)) {
            success = false;
        }
        log(output, error);
        if (!success) {
            return false;
        }
    }
    future.setProgressValue(5);
    return true;
}

void DebuggingHelperBuildTask::log(const QString &output, const QString &error)
{
    if (output.isEmpty() && error.isEmpty())
        return;

    QString logEntry;
    if (!output.isEmpty())
        logEntry.append(output);
    if (!error.isEmpty())
        logEntry.append(error);
    m_log.append(logEntry);

    emit logOutput(logEntry, m_showErrors && !error.isEmpty());
}
