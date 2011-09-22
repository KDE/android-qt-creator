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

#include "qmljsplugindumper.h"
#include "qmljsmodelmanager.h"

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsinterpreter.h>
#include <projectexplorer/projectexplorer.h>
#include <coreplugin/messagemanager.h>
#include <utils/filesystemwatcher.h>
#include <utils/fileutils.h>

#include <QtCore/QMetaType>
#include <QtCore/QDir>

using namespace LanguageUtils;
using namespace QmlJS;
using namespace QmlJSTools;
using namespace QmlJSTools::Internal;

PluginDumper::PluginDumper(ModelManager *modelManager)
    : QObject(modelManager)
    , m_modelManager(modelManager)
    , m_pluginWatcher(0)
{
    qRegisterMetaType<QmlJS::ModelManagerInterface::ProjectInfo>("QmlJS::ModelManagerInterface::ProjectInfo");
}

Utils::FileSystemWatcher *PluginDumper::pluginWatcher()
{
    if (!m_pluginWatcher) {
        m_pluginWatcher = new Utils::FileSystemWatcher(this);
        m_pluginWatcher->setObjectName(QLatin1String("PluginDumperWatcher"));
        connect(m_pluginWatcher, SIGNAL(fileChanged(QString)),
                this, SLOT(pluginChanged(QString)));
    }
    return m_pluginWatcher;
}

void PluginDumper::loadBuiltinTypes(const QmlJS::ModelManagerInterface::ProjectInfo &info)
{
    // move to the owning thread
    metaObject()->invokeMethod(this, "onLoadBuiltinTypes",
                               Q_ARG(QmlJS::ModelManagerInterface::ProjectInfo, info));
}

void PluginDumper::loadPluginTypes(const QString &libraryPath, const QString &importPath, const QString &importUri, const QString &importVersion)
{
    // move to the owning thread
    metaObject()->invokeMethod(this, "onLoadPluginTypes",
                               Q_ARG(QString, libraryPath),
                               Q_ARG(QString, importPath),
                               Q_ARG(QString, importUri),
                               Q_ARG(QString, importVersion));
}

void PluginDumper::scheduleRedumpPlugins()
{
    // move to the owning thread
    metaObject()->invokeMethod(this, "dumpAllPlugins", Qt::QueuedConnection);
}

void PluginDumper::scheduleMaybeRedumpBuiltins(const QmlJS::ModelManagerInterface::ProjectInfo &info)
{
    // move to the owning thread
    metaObject()->invokeMethod(this, "dumpBuiltins", Qt::QueuedConnection,
                               Q_ARG(QmlJS::ModelManagerInterface::ProjectInfo, info));
}

void PluginDumper::onLoadBuiltinTypes(const QmlJS::ModelManagerInterface::ProjectInfo &info, bool force)
{
    if (info.qmlDumpPath.isEmpty() || info.qtImportsPath.isEmpty())
        return;

    const QString importsPath = QDir::cleanPath(info.qtImportsPath);
    if (m_runningQmldumps.values().contains(importsPath))
        return;

    LibraryInfo builtinInfo;
    if (!force) {
        const Snapshot snapshot = m_modelManager->snapshot();
        builtinInfo = snapshot.libraryInfo(info.qtImportsPath);
        if (builtinInfo.isValid())
            return;
    }
    builtinInfo = LibraryInfo(LibraryInfo::Found);
    m_modelManager->updateLibraryInfo(info.qtImportsPath, builtinInfo);

    // prefer QTDIR/imports/builtins.qmltypes if available
    const QString builtinQmltypesPath = info.qtImportsPath + QLatin1String("/builtins.qmltypes");
    if (QFile::exists(builtinQmltypesPath)) {
        loadQmltypesFile(QStringList(builtinQmltypesPath), info.qtImportsPath, builtinInfo);
        return;
    }

    // run qmldump
    QProcess *process = new QProcess(this);
    process->setEnvironment(info.qmlDumpEnvironment.toStringList());
    connect(process, SIGNAL(finished(int)), SLOT(qmlPluginTypeDumpDone(int)));
    connect(process, SIGNAL(error(QProcess::ProcessError)), SLOT(qmlPluginTypeDumpError(QProcess::ProcessError)));
    QStringList args(QLatin1String("--builtins"));
    process->start(info.qmlDumpPath, args);
    m_runningQmldumps.insert(process, info.qtImportsPath);
    m_qtToInfo.insert(info.qtImportsPath, info);
}

static QString makeAbsolute(const QString &path, const QString &base)
{
    if (QFileInfo(path).isAbsolute())
        return path;
    return QString("%1%2%3").arg(base, QDir::separator(), path);
}

void PluginDumper::onLoadPluginTypes(const QString &libraryPath, const QString &importPath, const QString &importUri, const QString &importVersion)
{
    const QString canonicalLibraryPath = QDir::cleanPath(libraryPath);
    if (m_runningQmldumps.values().contains(canonicalLibraryPath))
        return;
    const Snapshot snapshot = m_modelManager->snapshot();
    const LibraryInfo libraryInfo = snapshot.libraryInfo(canonicalLibraryPath);
    if (libraryInfo.pluginTypeInfoStatus() != LibraryInfo::NoTypeInfo)
        return;

    // avoid inserting the same plugin twice
    int index;
    for (index = 0; index < m_plugins.size(); ++index) {
        if (m_plugins.at(index).qmldirPath == libraryPath)
            break;
    }
    if (index == m_plugins.size())
        m_plugins.append(Plugin());

    Plugin &plugin = m_plugins[index];
    plugin.qmldirPath = canonicalLibraryPath;
    plugin.importPath = importPath;
    plugin.importUri = importUri;
    plugin.importVersion = importVersion;

    // add default qmltypes file if it exists
    const QLatin1String defaultQmltypesFileName("plugins.qmltypes");
    const QString defaultQmltypesPath = makeAbsolute(defaultQmltypesFileName, canonicalLibraryPath);
    if (QFile::exists(defaultQmltypesPath))
        plugin.typeInfoPaths += defaultQmltypesPath;

    // add typeinfo files listed in qmldir
    foreach (const QmlDirParser::TypeInfo &typeInfo, libraryInfo.typeInfos())
        plugin.typeInfoPaths += makeAbsolute(typeInfo.fileName, canonicalLibraryPath);

    // watch plugin libraries
    foreach (const QmlDirParser::Plugin &plugin, snapshot.libraryInfo(canonicalLibraryPath).plugins()) {
        const QString pluginLibrary = resolvePlugin(canonicalLibraryPath, plugin.path, plugin.name);
        if (!pluginLibrary.isEmpty()) {
            if (!pluginWatcher()->watchesFile(pluginLibrary))
                pluginWatcher()->addFile(pluginLibrary, Utils::FileSystemWatcher::WatchModifiedDate);
            m_libraryToPluginIndex.insert(pluginLibrary, index);
        }
    }

    // watch library qmltypes file
    if (!plugin.typeInfoPaths.isEmpty()) {
        foreach (const QString &path, plugin.typeInfoPaths) {
            if (!QFile::exists(path))
                continue;
            if (!pluginWatcher()->watchesFile(path))
                pluginWatcher()->addFile(path, Utils::FileSystemWatcher::WatchModifiedDate);
            m_libraryToPluginIndex.insert(path, index);
        }
    }

    dump(plugin);
}

void PluginDumper::dumpBuiltins(const QmlJS::ModelManagerInterface::ProjectInfo &info)
{
    // if the builtin types were generated with a different qmldump, regenerate!
    if (m_qtToInfo.contains(info.qtImportsPath)) {
        QmlJS::ModelManagerInterface::ProjectInfo oldInfo = m_qtToInfo.value(info.qtImportsPath);
        if (oldInfo.qmlDumpPath != info.qmlDumpPath
                || oldInfo.qmlDumpEnvironment != info.qmlDumpEnvironment) {
            m_qtToInfo.remove(info.qtImportsPath);
            onLoadBuiltinTypes(info, true);
        }
    }
}

void PluginDumper::dumpAllPlugins()
{
    foreach (const Plugin &plugin, m_plugins) {
        dump(plugin);
    }
}

static QString noTypeinfoError(const QString &libraryPath)
{
    return PluginDumper::tr("QML module at:\n"
                            "%1\n"
                            "does not contain information about components contained in plugins.\n"
                            "See \"Using QML Modules with Plugins\" in the documentation.").arg(
                libraryPath);
}

static QString qmldumpErrorMessage(const QString &libraryPath, const QString &error)
{
    return noTypeinfoError(libraryPath) + QLatin1String("\n\n") +
            PluginDumper::tr("Automatic type dump of QML module failed.\nErrors:\n%1\n").
            arg(error);
}

static QString qmldumpFailedMessage(const QString &libraryPath, const QString &error)
{
    QString firstLines =
            QStringList(error.split(QLatin1Char('\n')).mid(0, 10)).join(QLatin1String("\n"));
    return noTypeinfoError(libraryPath) + QLatin1String("\n\n") +
            PluginDumper::tr("Automatic type dump of QML module failed.\n"
                             "First 10 lines or errors:\n"
                             "\n"
                             "%1"
                             "\n"
                             "Check 'General Messages' output pane for details."
                             ).arg(firstLines);
}

static void printParseWarnings(const QString &libraryPath, const QString &warning)
{
    Core::MessageManager *messageManager = Core::MessageManager::instance();
    messageManager->printToOutputPane(
                PluginDumper::tr("Warnings while parsing qmltypes information of %1:\n"
                                 "%2").arg(libraryPath, warning));
}

static QList<FakeMetaObject::ConstPtr> parseHelper(const QByteArray &qmlTypeDescriptions,
                                                   QString *error,
                                                   QString *warning)
{
    QList<FakeMetaObject::ConstPtr> ret;
    QHash<QString, FakeMetaObject::ConstPtr> newObjects;
    CppQmlTypesLoader::parseQmlTypeDescriptions(qmlTypeDescriptions, &newObjects,
                                                             error, warning);

    if (error->isEmpty()) {
        ret = newObjects.values();
    }
    return ret;
}

void PluginDumper::qmlPluginTypeDumpDone(int exitCode)
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process)
        return;
    process->deleteLater();

    const QString libraryPath = m_runningQmldumps.take(process);
    if (libraryPath.isEmpty())
        return;
    const Snapshot snapshot = m_modelManager->snapshot();
    LibraryInfo libraryInfo = snapshot.libraryInfo(libraryPath);

    if (exitCode != 0) {
        Core::MessageManager *messageManager = Core::MessageManager::instance();
        const QString errorMessages = process->readAllStandardError();
        messageManager->printToOutputPane(qmldumpErrorMessage(libraryPath, errorMessages));
        libraryInfo.setPluginTypeInfoStatus(LibraryInfo::DumpError, qmldumpFailedMessage(libraryPath, errorMessages));
    }

    const QByteArray output = process->readAllStandardOutput();
    QString error;
    QString warning;
    QList<FakeMetaObject::ConstPtr> objectsList = parseHelper(output, &error, &warning);
    if (exitCode == 0) {
        if (!error.isEmpty()) {
            libraryInfo.setPluginTypeInfoStatus(LibraryInfo::DumpError,
                                                qmldumpErrorMessage(libraryPath, error));
        } else {
            libraryInfo.setMetaObjects(objectsList);
            libraryInfo.setPluginTypeInfoStatus(LibraryInfo::DumpDone);
        }

        if (!warning.isEmpty())
            printParseWarnings(libraryPath, warning);
    }

    m_modelManager->updateLibraryInfo(libraryPath, libraryInfo);
}

void PluginDumper::qmlPluginTypeDumpError(QProcess::ProcessError)
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process)
        return;
    process->deleteLater();

    const QString libraryPath = m_runningQmldumps.take(process);
    if (libraryPath.isEmpty())
        return;

    Core::MessageManager *messageManager = Core::MessageManager::instance();
    const QString errorMessages = process->readAllStandardError();
    messageManager->printToOutputPane(qmldumpErrorMessage(libraryPath, errorMessages));

    if (!libraryPath.isEmpty()) {
        const Snapshot snapshot = m_modelManager->snapshot();
        LibraryInfo libraryInfo = snapshot.libraryInfo(libraryPath);
        libraryInfo.setPluginTypeInfoStatus(LibraryInfo::DumpError, qmldumpFailedMessage(libraryPath, errorMessages));
        m_modelManager->updateLibraryInfo(libraryPath, libraryInfo);
    }
}

void PluginDumper::pluginChanged(const QString &pluginLibrary)
{
    const int pluginIndex = m_libraryToPluginIndex.value(pluginLibrary, -1);
    if (pluginIndex == -1)
        return;

    const Plugin &plugin = m_plugins.at(pluginIndex);
    dump(plugin);
}

void PluginDumper::loadQmltypesFile(const QStringList &qmltypesFilePaths,
                                    const QString &libraryPath,
                                    QmlJS::LibraryInfo libraryInfo)
{
    QStringList errors;
    QStringList warnings;
    QList<FakeMetaObject::ConstPtr> objects;

    foreach (const QString &qmltypesFilePath, qmltypesFilePaths) {
        Utils::FileReader reader;
        if (!reader.fetch(qmltypesFilePath, QFile::Text)) {
            errors += reader.errorString();
            continue;
        }

        QString error;
        QString warning;
        objects += parseHelper(reader.data(), &error, &warning);
        if (!error.isEmpty())
            errors += tr("Failed to parse '%1'.\nError: %2").arg(qmltypesFilePath, error);
        if (!warning.isEmpty())
            warnings += warning;
    }

    libraryInfo.setMetaObjects(objects);
    if (errors.isEmpty()) {
        libraryInfo.setPluginTypeInfoStatus(LibraryInfo::TypeInfoFileDone);
    } else {
        errors.prepend(tr("Errors while reading typeinfo files:"));
        libraryInfo.setPluginTypeInfoStatus(LibraryInfo::TypeInfoFileError, errors.join(QLatin1String("\n")));
    }

    if (!warnings.isEmpty())
        printParseWarnings(libraryPath, warnings.join(QLatin1String("\n")));

    m_modelManager->updateLibraryInfo(libraryPath, libraryInfo);
}

void PluginDumper::dump(const Plugin &plugin)
{
    // if there are type infos, don't dump!
    if (!plugin.typeInfoPaths.isEmpty()) {
        const Snapshot snapshot = m_modelManager->snapshot();
        LibraryInfo libraryInfo = snapshot.libraryInfo(plugin.qmldirPath);
        if (!libraryInfo.isValid())
            return;

        loadQmltypesFile(plugin.typeInfoPaths, plugin.qmldirPath, libraryInfo);
        return;
    }

    ProjectExplorer::Project *activeProject = ProjectExplorer::ProjectExplorerPlugin::instance()->startupProject();
    if (!activeProject)
        return;

    ModelManagerInterface::ProjectInfo info = m_modelManager->projectInfo(activeProject);

    if (!info.tryQmlDump || info.qmlDumpPath.isEmpty()) {
        const Snapshot snapshot = m_modelManager->snapshot();
        LibraryInfo libraryInfo = snapshot.libraryInfo(plugin.qmldirPath);
        if (!libraryInfo.isValid())
            return;

        QString errorMessage;
        if (!info.tryQmlDump) {
            errorMessage = noTypeinfoError(plugin.qmldirPath);
        } else {
            errorMessage = qmldumpErrorMessage(plugin.qmldirPath,
                    tr("Could not locate the helper application for dumping type information from C++ plugins.\n"
                       "Please build the qmldump applcation on the Qt version options page."));
        }

        libraryInfo.setPluginTypeInfoStatus(LibraryInfo::DumpError, errorMessage);
        m_modelManager->updateLibraryInfo(plugin.qmldirPath, libraryInfo);
        return;
    }

    QProcess *process = new QProcess(this);
    process->setEnvironment(info.qmlDumpEnvironment.toStringList());
    connect(process, SIGNAL(finished(int)), SLOT(qmlPluginTypeDumpDone(int)));
    connect(process, SIGNAL(error(QProcess::ProcessError)), SLOT(qmlPluginTypeDumpError(QProcess::ProcessError)));
    QStringList args;
    if (plugin.importUri.isEmpty()) {
        args << QLatin1String("--path");
        args << plugin.importPath;
        if (ComponentVersion(plugin.importVersion).isValid())
            args << plugin.importVersion;
    } else {
        args << plugin.importUri;
        args << plugin.importVersion;
        args << plugin.importPath;
    }
    process->start(info.qmlDumpPath, args);
    m_runningQmldumps.insert(process, plugin.qmldirPath);
}

/*!
  Returns the result of the merge of \a baseName with \a path, \a suffixes, and \a prefix.
  The \a prefix must contain the dot.

  \a qmldirPath is the location of the qmldir file.

  Adapted from QDeclarativeImportDatabase::resolvePlugin.
*/
QString PluginDumper::resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath,
                                    const QString &baseName, const QStringList &suffixes,
                                    const QString &prefix)
{
    QStringList searchPaths;
    searchPaths.append(QLatin1String("."));

    bool qmldirPluginPathIsRelative = QDir::isRelativePath(qmldirPluginPath);
    if (!qmldirPluginPathIsRelative)
        searchPaths.prepend(qmldirPluginPath);

    foreach (const QString &pluginPath, searchPaths) {

        QString resolvedPath;

        if (pluginPath == QLatin1String(".")) {
            if (qmldirPluginPathIsRelative)
                resolvedPath = qmldirPath.absoluteFilePath(qmldirPluginPath);
            else
                resolvedPath = qmldirPath.absolutePath();
        } else {
            resolvedPath = pluginPath;
        }

        QDir dir(resolvedPath);
        foreach (const QString &suffix, suffixes) {
            QString pluginFileName = prefix;

            pluginFileName += baseName;
            pluginFileName += suffix;

            QFileInfo fileInfo(dir, pluginFileName);

            if (fileInfo.exists())
                return fileInfo.absoluteFilePath();
        }
    }

    return QString();
}

/*!
  Returns the result of the merge of \a baseName with \a dir and the platform suffix.

  Adapted from QDeclarativeImportDatabase::resolvePlugin.

  \table
  \header \i Platform \i Valid suffixes
  \row \i Windows     \i \c .dll
  \row \i Unix/Linux  \i \c .so
  \row \i AIX  \i \c .a
  \row \i HP-UX       \i \c .sl, \c .so (HP-UXi)
  \row \i Mac OS X    \i \c .dylib, \c .bundle, \c .so
  \row \i Symbian     \i \c .dll
  \endtable

  Version number on unix are ignored.
*/
QString PluginDumper::resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath,
                                    const QString &baseName)
{
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    return resolvePlugin(qmldirPath, qmldirPluginPath, baseName,
                         QStringList()
                         << QLatin1String("d.dll") // try a qmake-style debug build first
                         << QLatin1String(".dll"));
#elif defined(Q_OS_DARWIN)
    return resolvePlugin(qmldirPath, qmldirPluginPath, baseName,
                         QStringList()
                         << QLatin1String("_debug.dylib") // try a qmake-style debug build first
                         << QLatin1String(".dylib")
                         << QLatin1String(".so")
                         << QLatin1String(".bundle"),
                         QLatin1String("lib"));
#else  // Generic Unix
    QStringList validSuffixList;

#  if defined(Q_OS_HPUX)
/*
    See "HP-UX Linker and Libraries User's Guide", section "Link-time Differences between PA-RISC and IPF":
    "In PA-RISC (PA-32 and PA-64) shared libraries are suffixed with .sl. In IPF (32-bit and 64-bit),
    the shared libraries are suffixed with .so. For compatibility, the IPF linker also supports the .sl suffix."
 */
    validSuffixList << QLatin1String(".sl");
#   if defined __ia64
    validSuffixList << QLatin1String(".so");
#   endif
#  elif defined(Q_OS_AIX)
    validSuffixList << QLatin1String(".a") << QLatin1String(".so");
#  elif defined(Q_OS_UNIX)
    validSuffixList << QLatin1String(".so");
#  endif

    // Examples of valid library names:
    //  libfoo.so

    return resolvePlugin(qmldirPath, qmldirPluginPath, baseName, validSuffixList, QLatin1String("lib"));
#endif
}
