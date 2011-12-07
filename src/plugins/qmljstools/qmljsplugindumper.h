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

#ifndef QMLJSPLUGINDUMPER_H
#define QMLJSPLUGINDUMPER_H

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QProcess>

QT_BEGIN_NAMESPACE
class QDir;
QT_END_NAMESPACE

namespace Utils {
class FileSystemWatcher;
}

namespace QmlJSTools {
namespace Internal {

class ModelManager;

class PluginDumper : public QObject
{
    Q_OBJECT
public:
    explicit PluginDumper(ModelManager *modelManager);

public:
    void loadBuiltinTypes(const QmlJS::ModelManagerInterface::ProjectInfo &info);
    void loadPluginTypes(const QString &libraryPath, const QString &importPath,
                         const QString &importUri, const QString &importVersion);
    void scheduleRedumpPlugins();
    void scheduleMaybeRedumpBuiltins(const QmlJS::ModelManagerInterface::ProjectInfo &info);

private slots:
    void onLoadBuiltinTypes(const QmlJS::ModelManagerInterface::ProjectInfo &info,
                            bool force = false);
    void onLoadPluginTypes(const QString &libraryPath, const QString &importPath,
                           const QString &importUri, const QString &importVersion);
    void dumpBuiltins(const QmlJS::ModelManagerInterface::ProjectInfo &info);
    void dumpAllPlugins();
    void qmlPluginTypeDumpDone(int exitCode);
    void qmlPluginTypeDumpError(QProcess::ProcessError error);
    void pluginChanged(const QString &pluginLibrary);

private:
    class Plugin {
    public:
        QString qmldirPath;
        QString importPath;
        QString importUri;
        QString importVersion;
        QStringList typeInfoPaths;
    };

    void dump(const Plugin &plugin);
    void loadQmltypesFile(const QStringList &qmltypesFilePaths,
                          const QString &libraryPath,
                          QmlJS::LibraryInfo libraryInfo);
    QString resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath,
                          const QString &baseName);
    QString resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath,
                          const QString &baseName, const QStringList &suffixes,
                          const QString &prefix = QString());

private:
    Utils::FileSystemWatcher *pluginWatcher();

    ModelManager *m_modelManager;
    Utils::FileSystemWatcher *m_pluginWatcher;
    QHash<QProcess *, QString> m_runningQmldumps;
    QList<Plugin> m_plugins;
    QHash<QString, int> m_libraryToPluginIndex;
    QHash<QString, QmlJS::ModelManagerInterface::ProjectInfo> m_qtToInfo;
};

} // namespace Internal
} // namespace QmlJSTools

#endif // QMLJSPLUGINDUMPER_H
