/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#ifndef ANDROIDMANAGER_H
#define ANDROIDMANAGER_H

#include <utils/fileutils.h>

#include <QDomDocument>
#include <QObject>
#include <QStringList>

namespace ProjectExplorer { class Target; }

namespace Android {
class AndroidPlugin;

namespace Internal {

class AndroidManager : public QObject
{
    Q_OBJECT

public:
    enum BuildType
    {
        DebugBuild,
        ReleaseBuildUnsigned,
        ReleaseBuildSigned
    };

    static AndroidManager *instance();

    ~AndroidManager();

    static bool supportsAndroid(ProjectExplorer::Target *target);

    static QString packageName(ProjectExplorer::Target *target);
    static bool setPackageName(ProjectExplorer::Target *target, const QString &name);

    static QString applicationName(ProjectExplorer::Target *target);
    static bool setApplicationName(ProjectExplorer::Target *target, const QString &name);

    static QStringList permissions(ProjectExplorer::Target *target);
    static bool setPermissions(ProjectExplorer::Target *target, const QStringList &permissions);

    static QString intentName(ProjectExplorer::Target *target);
    static QString activityName(ProjectExplorer::Target *target);

    static int versionCode(ProjectExplorer::Target *target);
    static bool setVersionCode(ProjectExplorer::Target *target, int version);
    static QString versionName(ProjectExplorer::Target *target);
    static bool setVersionName(ProjectExplorer::Target *target, const QString &version);

    static QIcon highDpiIcon(ProjectExplorer::Target *target);
    static bool setHighDpiIcon(ProjectExplorer::Target *target, const QString &iconFilePath);
    static QIcon mediumDpiIcon(ProjectExplorer::Target *target);
    static bool setMediumDpiIcon(ProjectExplorer::Target *target, const QString &iconFilePath);
    static QIcon lowDpiIcon(ProjectExplorer::Target *target);
    static bool setLowDpiIcon(ProjectExplorer::Target *target, const QString &iconFilePath);

    static QStringList availableTargetApplications(ProjectExplorer::Target *target);
    static QString targetApplication(ProjectExplorer::Target *target);
    static bool setTargetApplication(ProjectExplorer::Target *target, const QString &name);
    static QString targetApplicationPath(ProjectExplorer::Target *target);

    static QString targetSDK(ProjectExplorer::Target *target);
    static bool setTargetSDK(ProjectExplorer::Target *target, const QString &sdk);

    static Utils::FileName dirPath(ProjectExplorer::Target *target);
    static Utils::FileName manifestPath(ProjectExplorer::Target *target);
    static Utils::FileName libsPath(ProjectExplorer::Target *target);
    static Utils::FileName stringsPath(ProjectExplorer::Target *target);
    static Utils::FileName defaultPropertiesPath(ProjectExplorer::Target *target);
    static Utils::FileName srcPath(ProjectExplorer::Target *target);
    static Utils::FileName apkPath(ProjectExplorer::Target *target, BuildType buildType);

    static bool createAndroidTemplatesIfNecessary(ProjectExplorer::Target *target);
    static void updateTarget(ProjectExplorer::Target *target, const QString &targetSDK,
                             const QString &name = QString());

    static Utils::FileName localLibsRulesFilePath(ProjectExplorer::Target *target);
    static QString loadLocalLibs(ProjectExplorer::Target *target, int apiLevel);
    static QString loadLocalJars(ProjectExplorer::Target *target, int apiLevel);

    static QStringList availableQtLibs(ProjectExplorer::Target *target);
    static QStringList qtLibs(ProjectExplorer::Target *target);
    static bool setQtLibs(ProjectExplorer::Target *target, const QStringList &libs);

    static QStringList availablePrebundledLibs(ProjectExplorer::Target *target);
    static QStringList prebundledLibs(ProjectExplorer::Target *target);
    static bool setPrebundledLibs(ProjectExplorer::Target *target, const QStringList &libs);

private:
    explicit AndroidManager(QObject *parent = 0);

    static void raiseError(const QString &reason);
    static bool openXmlFile(ProjectExplorer::Target *target, QDomDocument &doc,
                            const Utils::FileName &fileName, bool createAndroidTemplates = false);
    static bool saveXmlFile(ProjectExplorer::Target *target, QDomDocument &doc, const Utils::FileName &fileName);
    static bool openManifest(ProjectExplorer::Target *target, QDomDocument &doc);
    static bool saveManifest(ProjectExplorer::Target *target, QDomDocument &doc);
    static bool openLibsXml(ProjectExplorer::Target *target, QDomDocument &doc);
    static bool saveLibsXml(ProjectExplorer::Target *target, QDomDocument &doc);
    static QStringList libsXml(ProjectExplorer::Target *target, const QString &tag);
    static bool setLibsXml(ProjectExplorer::Target *target, const QStringList &libs, const QString &tag);

    enum ItemType
    {
        Lib,
        Jar
    };
    static QString loadLocal(ProjectExplorer::Target *target, int apiLevel, ItemType item);

    class Library
    {
    public:
        Library()
        { level = -1; }
        int level;
        QStringList dependencies;
        QString name;
    };
    typedef QMap<QString, Library> LibrariesMap;

    enum IconType
    {
        HighDPI,
        MediumDPI,
        LowDPI
    };
    static QString iconPath(ProjectExplorer::Target *target, IconType type);
    static QIcon icon(ProjectExplorer::Target *target, IconType type);
    static bool setIcon(ProjectExplorer::Target *target, IconType type, const QString &iconFileName);

    static QStringList dependencies(const Utils::FileName &readelfPath, const QString &lib);
    static int setLibraryLevel(const QString &library, LibrariesMap &mapLibs);
    static bool qtLibrariesLessThan(const Library &a, const Library &b);

    static AndroidManager *m_instance;

    friend class Android::AndroidPlugin;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDMANAGER_H
