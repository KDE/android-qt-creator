/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDTEMPLATESCREATOR_H
#define ANDROIDTEMPLATESCREATOR_H

#include "qt4projectmanager/qt4target.h"
#include "qt4projectmanager/qt4buildconfiguration.h"

#include <QtCore/QFile>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtGui/QIcon>
#include <QtXml/QDomDocument>

QT_FORWARD_DECLARE_CLASS(QFileSystemWatcher);

namespace ProjectExplorer {
class Project;
class ProjectNode;
class Target;
}

namespace Qt4ProjectManager {
class Qt4Project;
class Qt4Target;
class Qt4ProFileNode;
}


namespace Android {
namespace Internal {
class AndroidTargetFactory;

class AndroidTarget  : public Qt4ProjectManager::Qt4BaseTarget
{
    friend class AndroidTargetFactory;
    Q_OBJECT
    enum AndroidIconType
    {
        HighDPI,
        MediumDPI,
        LowDPI
    };

    struct Library
    {
        Library()
        {
            level = -1;
        }
        int level;
        QStringList dependencies;
        QString name;
    };
    typedef QMap<QString, Library>  LibrariesMap;
public:
    enum BuildType
    {
        DebugBuild,
        ReleaseBuildUnsigned,
        ReleaseBuildSigned
    };

    explicit AndroidTarget(Qt4ProjectManager::Qt4Project *parent, const QString &id);
    virtual ~AndroidTarget();

    Qt4ProjectManager::Qt4BuildConfigurationFactory *buildConfigurationFactory() const;

    void createApplicationProFiles();

    QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Node *n);

    static QString defaultDisplayName();


    QString packageName();
    bool setPackageName(const QString &name);

    QString intentName();
    QString activityName();

    QString applicationName();
    bool setApplicationName(const QString &name);

    QStringList availableTargetApplications();
    QString targetApplication();
    bool setTargetApplication(const QString &name);
    QString targetApplicationPath();

    QString targetSDK();
    bool setTargetSDK(const QString &target);

    int versionCode();
    bool setVersionCode(int version);

    QString versionName();
    bool setVersionName(const QString &version);

    QStringList permissions();
    bool setPermissions(const QStringList &permissions);

    QStringList availableQtLibs();
    QStringList qtLibs();
    bool setQtLibs(const QStringList &qtLibs);

    QStringList availablePrebundledLibs();
    QStringList prebundledLibs();
    bool setPrebundledLibs(const QStringList &qtLibs);

    QIcon highDpiIcon();
    bool setHighDpiIcon(const QString &iconFilePath);

    QIcon mediumDpiIcon();
    bool setMediumDpiIcon(const QString &iconFilePath);

    QIcon lowDpiIcon();
    bool setLowDpiIcon(const QString &iconFilePath);

    QString androidDirPath();
    QString androidManifestPath();
    QString androidLibsPath();
    QString androidStringsPath();
    QString androidDefaultPropertiesPath();
    QString androidSrcPath();
    QString apkPath(BuildType buildType);
    QString localLibsRulesFilePath();
    QString loadLocalLibs(int apiLevel);
    QString loadLocalJars(int apiLevel);

public slots:
    bool createAndroidTemplatesIfNecessary(bool forceJava = false);
    void updateProject(const QString &targetSDK, const QString &name = QString());

signals:
    void androidDirContentsChanged();

private slots:
    void handleTargetAdded(ProjectExplorer::Target *target);
    void handleTargetToBeRemoved(ProjectExplorer::Target *target);

private:
    enum ItemType
    {
        Lib,
        Jar
    };

    QString loadLocal(int apiLevel, ItemType item);
    void raiseError(const QString &reason);
    bool openXmlFile(QDomDocument &doc, const QString &fileName);
    bool saveXmlFile(QDomDocument &doc, const QString &fileName);
    bool openAndroidManifest(QDomDocument &doc);
    bool saveAndroidManifest(QDomDocument &doc);
    bool openLibsXml(QDomDocument &doc);
    bool saveLibsXml(QDomDocument &doc);

    QIcon androidIcon(AndroidIconType type);
    bool setAndroidIcon(AndroidIconType type, const QString &iconFileName);

    QStringList libsXml(const QString &tag);
    bool setLibsXml(const QStringList &qtLibs, const QString &tag);

    static bool QtLibrariesLessThan(const AndroidTarget::Library &a, const AndroidTarget::Library &b);
    QStringList getDependencies(const QString &readelfPath, const QString &lib);
    int setLibraryLevel(const QString &library, LibrariesMap &mapLibs);


    QFileSystemWatcher *const m_androidFilesWatcher;

    Qt4ProjectManager::Qt4BuildConfigurationFactory *m_buildConfigurationFactory;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDTEMPLATESCREATOR_H
