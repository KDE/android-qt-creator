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

#include "qmldumptool.h"
#include "qtsupportconstants.h"
#include "qtversionmanager.h"
#include "debugginghelperbuildtask.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runconfiguration.h>
#include <qtconcurrent/runextensions.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <utils/qtcassert.h>
#include <QtGui/QDesktopServices>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QHash>

namespace {

using namespace QtSupport;
using QtSupport::DebuggingHelperBuildTask;


class QmlDumpBuildTask;

typedef QHash<int, QmlDumpBuildTask *> QmlDumpByVersion;
Q_GLOBAL_STATIC(QmlDumpByVersion, qmlDumpBuilds)

// A task suitable to be run by QtConcurrent to build qmldump.
class QmlDumpBuildTask : public QObject {
    Q_DISABLE_COPY(QmlDumpBuildTask)
    Q_OBJECT
public:
    explicit QmlDumpBuildTask(BaseQtVersion *version)
        : m_buildTask(new DebuggingHelperBuildTask(version, DebuggingHelperBuildTask::QmlDump))
        , m_failed(false)
    {
        qmlDumpBuilds()->insert(version->uniqueId(), this);
        // Don't open General Messages pane with errors
        m_buildTask->showOutputOnError(false);
        connect(m_buildTask, SIGNAL(finished(int,QString,DebuggingHelperBuildTask::Tools)),
                this, SLOT(finish(int,QString,DebuggingHelperBuildTask::Tools)),
                Qt::QueuedConnection);
    }

    void run(QFutureInterface<void> &future)
    {
        m_buildTask->run(future);
    }

    void updateProjectWhenDone(QPointer<ProjectExplorer::Project> project, bool preferDebug)
    {
        foreach (const ProjectToUpdate &update, m_projectsToUpdate) {
            if (update.project == project)
                return;
        }

        ProjectToUpdate update;
        update.project = project;
        update.preferDebug = preferDebug;
        m_projectsToUpdate += update;
    }

    bool hasFailed() const
    {
        return m_failed;
    }

private slots:
    void finish(int qtId, const QString &output, DebuggingHelperBuildTask::Tools tools)
    {
        BaseQtVersion *version = QtVersionManager::instance()->version(qtId);

        QTC_ASSERT(tools == DebuggingHelperBuildTask::QmlDump, return);
        QString errorMessage;
        if (!version) {
            m_failed = true;
            errorMessage = QString::fromLatin1("Qt version became invalid");
        } else {
            if (!version->hasQmlDump()) {
                m_failed = true;
                errorMessage = QString::fromLatin1("Could not build QML plugin dumping helper for %1\n"
                                                   "Output:\n%2").
                        arg(version->displayName(), output);
            }
        }

        if (m_failed) {
            qWarning("%s", qPrintable(errorMessage));
        }

        // update qmldump path for all the project
        QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
        if (!modelManager)
            return;

        foreach (const ProjectToUpdate &update, m_projectsToUpdate) {
            if (!update.project)
                continue;
            QmlJS::ModelManagerInterface::ProjectInfo projectInfo = modelManager->projectInfo(update.project);
            projectInfo.qmlDumpPath = version->qmlDumpTool(update.preferDebug);
            if (projectInfo.qmlDumpPath.isEmpty())
                projectInfo.qmlDumpPath = version->qmlDumpTool(!update.preferDebug);
            projectInfo.qmlDumpEnvironment = version->qmlToolsEnvironment();
            modelManager->updateProjectInfo(projectInfo);
        }

        // clean up
        qmlDumpBuilds()->remove(qtId);
        deleteLater();
    }

private:
    class ProjectToUpdate {
    public:
        QPointer<ProjectExplorer::Project> project;
        bool preferDebug;
    };

    QList<ProjectToUpdate> m_projectsToUpdate;
    DebuggingHelperBuildTask *m_buildTask; // deletes itself after run()
    bool m_failed;
};
} // end of anonymous namespace


namespace QtSupport {

static inline QStringList validBinaryFilenames(bool debugBuild)
{
    QStringList list = QStringList()
            << QLatin1String("qmldump.exe")
            << QLatin1String("qmldump")
            << QLatin1String("qmldump.app/Contents/MacOS/qmldump");
    if (debugBuild)
        list.prepend(QLatin1String("debug/qmldump.exe"));
    else
        list.prepend(QLatin1String("release/qmldump.exe"));
    return list;
}

static inline QStringList validPrebuiltFilenames(bool debugBuild)
{
    QStringList list = QStringList(QLatin1String("qmlplugindump"));
    list.append(QLatin1String("qmlplugindump.app/Contents/MacOS/qmlplugindump"));
    if (debugBuild)
        list.prepend(QLatin1String("qmlplugindumpd.exe"));
    else
        list.prepend(QLatin1String("qmlplugindump.exe"));
    return list;
}

static bool hasPrivateHeaders(const QString &qtInstallHeaders) {
    const QString header = qtInstallHeaders
            + QLatin1String("/QtDeclarative/private/qdeclarativemetatype_p.h");
    return QFile::exists(header);
}

bool QmlDumpTool::canBuild(const BaseQtVersion *qtVersion, QString *reason)
{
    const QString installHeaders = qtVersion->versionInfo().value("QT_INSTALL_HEADERS");

    if (qtVersion->type() != Constants::DESKTOPQT
            && qtVersion->type() != Constants::SIMULATORQT) {
        if (reason)
            *reason = QCoreApplication::translate("Qt4ProjectManager::QmlDumpTool", "Only available for Qt for Desktop and Qt for Qt Simulator.");
        return false;
    }
    if (qtVersion->qtVersion() < QtVersionNumber(4, 7, 1)) {
        if (reason)
            *reason = QCoreApplication::translate("Qt4ProjectManager::QmlDumpTool", "Only available for Qt 4.7.1 or newer.");
        return false;
    }

    if (!hasPrivateHeaders(installHeaders)) {
        if (reason)
            *reason = QCoreApplication::translate("Qt4ProjectManager::QmlDumpTool", "Private headers are missing for this Qt version.");
        return false;
    }
    return true;
}

QString QmlDumpTool::toolForVersion(BaseQtVersion *version, bool debugDump)
{
    if (version) {
        QString qtInstallData = version->versionInfo().value("QT_INSTALL_DATA");
        QString qtInstallHeaders = version->versionInfo().value("QT_INSTALL_HEADERS");
        QString toolPath = toolByInstallData(qtInstallData, qtInstallHeaders, debugDump);
        return toolPath;
    }

    return QString();
}

static QString sourcePath()
{
    return Core::ICore::instance()->resourcePath() + QLatin1String("/qml/qmldump/");
}

static QStringList sourceFileNames()
{
    QStringList files;
    files << QLatin1String("main.cpp") << QLatin1String("qmldump.pro")
          << QLatin1String("qmlstreamwriter.cpp") << QLatin1String("qmlstreamwriter.h")
          << QLatin1String("LICENSE.LGPL") << QLatin1String("LGPL_EXCEPTION.TXT");
#ifdef Q_OS_MAC
    files << QLatin1String("Info.plist");
#endif
    return files;
}

QString QmlDumpTool::toolByInstallData(const QString &qtInstallData, const QString &qtInstallHeaders,
                                       bool debugDump)
{
    if (!Core::ICore::instance())
        return QString();

    // check for prebuilt binary first
    QFileInfo fileInfo;
    if (getHelperFileInfoFor(validPrebuiltFilenames(debugDump), qtInstallData + QLatin1String("/bin/"), &fileInfo))
        return fileInfo.absoluteFilePath();

    const QStringList directories = installDirectories(qtInstallData);
    const QStringList binFilenames = validBinaryFilenames(debugDump);

    return byInstallDataHelper(sourcePath(), sourceFileNames(), directories, binFilenames,
                               !hasPrivateHeaders(qtInstallHeaders));
}

QStringList QmlDumpTool::locationsByInstallData(const QString &qtInstallData, bool debugDump)
{
    QStringList result;
    QFileInfo fileInfo;
    const QStringList binFilenames = validBinaryFilenames(debugDump);
    foreach(const QString &directory, installDirectories(qtInstallData)) {
        if (getHelperFileInfoFor(binFilenames, directory, &fileInfo))
            result << fileInfo.filePath();
    }
    return result;
}

bool QmlDumpTool::build(BuildHelperArguments arguments, QString *log, QString *errorMessage)
{
    arguments.helperName = QCoreApplication::translate("Qt4ProjectManager::QmlDumpTool", "qmldump");
    arguments.proFilename = QLatin1String("qmldump.pro");
    return buildHelper(arguments, log, errorMessage);
}

QString QmlDumpTool::copy(const QString &qtInstallData, QString *errorMessage)
{
    const QStringList directories = QmlDumpTool::installDirectories(qtInstallData);

    // Try to find a writeable directory.
    foreach(const QString &directory, directories) {
        if (copyFiles(sourcePath(), sourceFileNames(), directory, errorMessage)) {
            return directory;
        }
    }
    *errorMessage = QCoreApplication::translate("ProjectExplorer::QmlDumpTool",
                                                "qmldump could not be built in any of the directories:\n- %1\n\nReason: %2")
                    .arg(directories.join(QLatin1String("\n- ")), *errorMessage);
    return QString();
}

QStringList QmlDumpTool::installDirectories(const QString &qtInstallData)
{
    const QChar slash = QLatin1Char('/');
    const uint hash = qHash(qtInstallData);
    QStringList directories;
    directories
            << (qtInstallData + QLatin1String("/qtc-qmldump/"))
            << QDir::cleanPath((QCoreApplication::applicationDirPath() + QLatin1String("/../qtc-qmldump/") + QString::number(hash))) + slash
            << (QDesktopServices::storageLocation(QDesktopServices::DataLocation) + QLatin1String("/qtc-qmldump/") + QString::number(hash)) + slash;
    return directories;
}

void QmlDumpTool::pathAndEnvironment(ProjectExplorer::Project *project, BaseQtVersion *version,
                                     bool preferDebug, QString *dumperPath, Utils::Environment *env)
{
    QString path;
    if (version && !version->hasQmlDump() && QmlDumpTool::canBuild(version)) {
        QmlDumpBuildTask *qmlDumpBuildTask = qmlDumpBuilds()->value(version->uniqueId());
        if (qmlDumpBuildTask) {
            if (!qmlDumpBuildTask->hasFailed())
                qmlDumpBuildTask->updateProjectWhenDone(project, preferDebug);
        } else {
            QmlDumpBuildTask *buildTask = new QmlDumpBuildTask(version);
            buildTask->updateProjectWhenDone(project, preferDebug);
            QFuture<void> task = QtConcurrent::run(&QmlDumpBuildTask::run, buildTask);
            const QString taskName = QmlDumpBuildTask::tr("Building helper");
            Core::ICore::instance()->progressManager()->addTask(task, taskName,
                                                                QLatin1String("Qt4ProjectManager::BuildHelpers"));
        }
        return;
    }

    path = toolForVersion(version, preferDebug);
    if (path.isEmpty())
        path = toolForVersion(version, !preferDebug);

    if (!path.isEmpty()) {
        QFileInfo qmldumpFileInfo(path);
        if (!qmldumpFileInfo.exists()) {
            qWarning() << "QmlDumpTool::qmlDumpPath: qmldump executable does not exist at" << path;
            path.clear();
        } else if (!qmldumpFileInfo.isFile()) {
            qWarning() << "QmlDumpTool::qmlDumpPath: " << path << " is not a file";
            path.clear();
        }
    }

    if (!path.isEmpty() && version && dumperPath && env) {
        *dumperPath = path;
        *env = version->qmlToolsEnvironment();
    }
}

} // namespace Qt4ProjectManager

#include "qmldumptool.moc"
