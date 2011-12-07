/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidtarget.h"
#include "androiddeployconfiguration.h"
#include "androidconfigurations.h"
#include "androidrunconfiguration.h"
#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidpackagecreationstep.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qt4projectmanager/qt4target.h>

#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QList>
#include <QtGui/QPixmap>
#include <QtCore/QProcess>
#include <QtGui/QMessageBox>
#include <QtGui/QApplication>
#include <QtXml/QDomDocument>

#include <cctype>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace {
    const QLatin1String AndroidDirName("android");
    const QLatin1String AndroidManifestName("AndroidManifest.xml");
    const QLatin1String AndroidLibsFileName("/res/values/libs.xml");
    const QLatin1String AndroidStringsFileName("/res/values/strings.xml");
    const QLatin1String AndroidDefaultPropertiesName("project.properties");
} // anonymous namespace

namespace Android {
namespace Internal {

AndroidTarget::AndroidTarget(Qt4Project *parent, const QString &id) :
    Qt4BaseTarget(parent, id)
  , m_androidFilesWatcher(new QFileSystemWatcher(this))
  , m_buildConfigurationFactory(new Qt4BuildConfigurationFactory(this))
{
    setDisplayName(defaultDisplayName());
    setDefaultDisplayName(defaultDisplayName());
    setIcon(QIcon(Constants::ANDROID_SETTINGS_CATEGORY_ICON));
    connect(parent, SIGNAL(addedTarget(ProjectExplorer::Target*)),
        this, SLOT(handleTargetAdded(ProjectExplorer::Target*)));
}

AndroidTarget::~AndroidTarget()
{

}

Qt4BuildConfigurationFactory *AndroidTarget::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

void AndroidTarget::createApplicationProFiles()
{
    removeUnconfiguredCustomExectutableRunConfigurations();

    QList<Qt4ProFileNode *> profiles = qt4Project()->applicationProFiles();
    QSet<QString> paths;
    foreach (Qt4ProFileNode *pro, profiles)
        paths << pro->path();

    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
        if (AndroidRunConfiguration *qt4rc = qobject_cast<AndroidRunConfiguration *>(rc))
            paths.remove(qt4rc->proFilePath());

    // Only add new runconfigurations if there are none.
    foreach (const QString &path, paths)
        addRunConfiguration(new AndroidRunConfiguration(this, path));

    // Oh still none? Add a custom executable runconfiguration
    if (runConfigurations().isEmpty()) {
        addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(this));
    }
}

QList<ProjectExplorer::RunConfiguration *> AndroidTarget::runConfigurationsForNode(ProjectExplorer::Node *n)
{
    QList<ProjectExplorer::RunConfiguration *> result;
    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
        if (AndroidRunConfiguration *qt4c = qobject_cast<AndroidRunConfiguration *>(rc))
                if (qt4c->proFilePath() == n->path())
                    result << rc;
    return result;
}

QString AndroidTarget::defaultDisplayName()
{
    return QApplication::translate("Qt4ProjectManager::Qt4Target", "Android", "Qt4 Android target display name");
}

void AndroidTarget::handleTargetAdded(ProjectExplorer::Target *target)
{
    if (target != this)
        return;

    disconnect(project(), SIGNAL(addedTarget(ProjectExplorer::Target*)),
        this, SLOT(handleTargetAdded(ProjectExplorer::Target*)));
    connect(project(), SIGNAL(aboutToRemoveTarget(ProjectExplorer::Target*)),
        SLOT(handleTargetToBeRemoved(ProjectExplorer::Target*)));

    if (!createAndroidTemplatesIfNecessary())
        return;

    m_androidFilesWatcher->addPath(androidDirPath());
    m_androidFilesWatcher->addPath(androidManifestPath());
    m_androidFilesWatcher->addPath(androidSrcPath());
    connect(m_androidFilesWatcher, SIGNAL(directoryChanged(QString)), this,
        SIGNAL(androidDirContentsChanged()));
    connect(m_androidFilesWatcher, SIGNAL(fileChanged(QString)), this,
        SIGNAL(androidDirContentsChanged()));
}

void AndroidTarget::handleTargetToBeRemoved(ProjectExplorer::Target *target)
{
    if (target != this)
        return;

// I don't think is a good idea to remove android directory

//    const QString debianPath = debianDirPath();
//    if (!QFileInfo(debianPath).exists())
//        return;
//    const int answer = QMessageBox::warning(0, tr("Qt Creator"),
//        tr("Do you want to remove the packaging directory\n"
//           "associated with the target '%1'?").arg(displayName()),
//        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
//    if (answer == QMessageBox::No)
//        return;
//    QString error;
//    if (!MaemoGlobal::removeRecursively(debianPath, error))
//        qDebug("%s", qPrintable(error));
//    const QString packagingPath = project()->projectDirectory()
//        + QLatin1Char('/') + PackagingDirName;
//    const QStringList otherContents = QDir(packagingPath).entryList(QDir::Dirs
//        | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
//    if (otherContents.isEmpty()) {
//        if (!MaemoGlobal::removeRecursively(packagingPath, error))
//            qDebug("%s", qPrintable(error));
//    }
}

QString AndroidTarget::androidDirPath()
{
    return project()->projectDirectory()+QLatin1Char('/')+AndroidDirName;
}

QString AndroidTarget::androidManifestPath()
{
    return androidDirPath()+QLatin1Char('/')+AndroidManifestName;
}

QString AndroidTarget::androidLibsPath()
{
    return androidDirPath()+AndroidLibsFileName;
}

QString AndroidTarget::androidStringsPath()
{
    return androidDirPath()+AndroidStringsFileName;
}

QString AndroidTarget::androidDefaultPropertiesPath()
{
    return androidDirPath()+QLatin1Char('/')+AndroidDefaultPropertiesName;
}

QString AndroidTarget::androidSrcPath()
{
    return androidDirPath()+QLatin1String("/src");
}

QString AndroidTarget::apkPath(BuildType buildType)
{
    return project()->projectDirectory()+QLatin1Char('/')
            + AndroidDirName
            + QString("/bin/%1-%2.apk")
            .arg(applicationName())
            .arg(buildType == DebugBuild ? "debug" : (buildType == ReleaseBuildUnsigned) ? "unsigned" : "signed");
}

QString AndroidTarget::localLibsRulesFilePath()
{
    const Qt4Project *const qt4Project = qobject_cast<const Qt4Project *>(project());
    if (!qt4Project)
        return "";
    return qt4Project->activeTarget()->activeBuildConfiguration()->qtVersion()->versionInfo()["QT_INSTALL_LIBS"] + "/rules.xml";
}

QString AndroidTarget::loadLocal(int apiLevel, ItemType item)
{
    QString itemType;
    if (item==Lib)
        itemType="lib";
    else
        itemType="jar";

    QString localLibs;

    QDomDocument doc;
    if (!openXmlFile(doc, localLibsRulesFilePath()))
        return localLibs;

    QStringList libs;
    libs << qtLibs() << prebundledLibs();
    QDomElement element = doc.documentElement().firstChildElement("platforms").firstChildElement(itemType+"s").firstChildElement("version");
    while (!element.isNull()) {
        if (element.attribute("value").toInt() == apiLevel) {
            if (element.hasAttribute("symlink"))
                apiLevel = element.attribute("symlink").toInt();
            break;
        }
        element=element.nextSiblingElement("version");
    }

    element = doc.documentElement().firstChildElement("dependencies").firstChildElement("lib");
    while (!element.isNull()) {
        if (libs.contains(element.attribute("name"))) {
            QDomElement libElement = element.firstChildElement("depends").firstChildElement(itemType);
            while (!libElement.isNull()) {
                localLibs += libElement.attribute("file").arg(apiLevel)+":";
                libElement = libElement.nextSiblingElement(itemType);
            }

            libElement = element.firstChildElement("replaces").firstChildElement(itemType);
            while (!libElement.isNull()) {
                localLibs.replace(libElement.attribute("file").arg(apiLevel) + ":","");
                libElement = libElement.nextSiblingElement(itemType);
            }
        }
        element = element.nextSiblingElement("lib");
    }
    return localLibs;
}

QString AndroidTarget::loadLocalLibs(int apiLevel)
{
    return loadLocal(apiLevel, Lib);
}

QString AndroidTarget::loadLocalJars(int apiLevel)
{
    return loadLocal(apiLevel, Jar);
}

void AndroidTarget::updateProject(const QString &targetSDK, const QString &name)
{
    QString androidDir = androidDirPath();

    // clean previous build
    QProcess androidProc;
    androidProc.setWorkingDirectory(androidDir);
    androidProc.start(AndroidConfigurations::instance().antToolPath(), QStringList() << "clean");
    if (!androidProc.waitForFinished(-1))
        androidProc.terminate();
    // clean previous build

    int targetSDKNumber = targetSDK.mid(targetSDK.lastIndexOf('-') + 1).toInt();
    bool commentLines = false;
    QDirIterator it(androidDir,QStringList() << "*.java", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFile file(it.filePath());
        if (!file.open(QIODevice::ReadWrite))
            continue;
        QList<QByteArray> lines = file.readAll().trimmed().split('\n');

        bool modified = false;
        bool comment = false;
        for (int i = 0; i < lines.size(); i++) {
            if (lines[i].contains("@ANDROID-")) {
                commentLines = targetSDKNumber < lines[i].mid(lines[i].lastIndexOf('-')+1).toInt();
                comment = !comment;
                continue;
            }
            if (!comment)
                continue;
            if (commentLines) {
                if (!lines[i].trimmed().startsWith("//QtCreator")) {
                    lines[i] = "//QtCreator "+lines[i];
                    modified =  true;
                }
            } else { if (lines[i].trimmed().startsWith("//QtCreator")) {
                    lines[i] = lines[i].mid(12);
                    modified =  true;
                }
            }
        }
        if (modified) {
            file.resize(0);
            foreach (const QByteArray & line, lines) {
                file.write(line);
                file.write("\n");
            }
        }
        file.close();
    }

    QStringList params;
    params << "update" << "project" << "-p" << androidDir;
    if (!targetSDK.isEmpty())
        params << "-t" << targetSDK;
    if (!name.isEmpty())
        params << "-n" << name;
    androidProc.start(AndroidConfigurations::instance().androidToolPath(), params);
    if (!androidProc.waitForFinished(-1))
        androidProc.terminate();
}

bool AndroidTarget::createAndroidTemplatesIfNecessary(bool forceJava)
{
    const Qt4Project *qt4Project = qobject_cast<Qt4Project*>(project());
    if (!qt4Project || !qt4Project->rootProjectNode())
        return false;
    QDir projectDir(project()->projectDirectory());
    QString androidPath=androidDirPath();

    if (!forceJava && QFileInfo(androidPath).exists()
            && QFileInfo(androidManifestPath()).exists()
            && QFileInfo(androidPath+QLatin1String("/src")).exists()
            && QFileInfo(androidPath+QLatin1String("/res")).exists()) {
        return true;
    }

    if (!QFileInfo(androidDirPath()).exists())
        if (!projectDir.mkdir(AndroidDirName) && !forceJava) {
            raiseError(tr("Error creating Android directory '%1'.")
                .arg(AndroidDirName));
            return false;
        }

    if (forceJava)
        AndroidPackageCreationStep::removeDirectory(AndroidDirName+QLatin1String("/src"));

    QStringList androidFiles;
    QDirIterator it(qt4Project->activeTarget()->activeBuildConfiguration()->qtVersion()->versionInfo()["QT_INSTALL_PREFIX"]+QLatin1String("/src/android/java"),QDirIterator::Subdirectories);
    int pos=it.path().size();
    while (it.hasNext()) {
        it.next();
        if (it.fileInfo().isDir()) {
            projectDir.mkpath(AndroidDirName+it.filePath().mid(pos));
        } else {
            QFile::copy(it.filePath(), androidPath+it.filePath().mid(pos));
            androidFiles << androidPath + it.filePath().mid(pos);
        }
    }
    qt4Project->rootProjectNode()->addFiles(UnknownFileType, androidFiles);

    QStringList sdks = AndroidConfigurations::instance().sdkTargets();
    if (sdks.isEmpty()) {
        raiseError(tr("No Qt for Android SDKs were found.\nPlease install at least one SDK."));
        return false;
    }
    updateProject(AndroidConfigurations::instance().sdkTargets().at(0));
    if (availableTargetApplications().length())
        setTargetApplication(availableTargetApplications()[0]);
    return true;
}

bool AndroidTarget::openXmlFile(QDomDocument & doc, const QString & fileName)
{
    if (!createAndroidTemplatesIfNecessary())
        return false;

    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    if (!doc.setContent(f.readAll())) {
        raiseError(tr("Can't parse '%1'").arg(fileName));
        return false;
    }
    return true;
}

bool AndroidTarget::saveXmlFile(QDomDocument & doc, const QString & fileName)
{
    if (!createAndroidTemplatesIfNecessary())
        return false;

    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly)) {
        raiseError(tr("Can't open '%1'").arg(fileName));
        return false;
    }
    return f.write(doc.toByteArray(4)) >= 0;
}

bool AndroidTarget::openAndroidManifest(QDomDocument & doc)
{
    return openXmlFile(doc, androidManifestPath());
}

bool AndroidTarget::saveAndroidManifest(QDomDocument & doc)
{
    return saveXmlFile(doc, androidManifestPath());
}

bool AndroidTarget::openLibsXml(QDomDocument & doc)
{
    return openXmlFile(doc, androidLibsPath());
}

bool AndroidTarget::saveLibsXml(QDomDocument & doc)
{
    return saveXmlFile(doc, androidLibsPath());
}

QString AndroidTarget::activityName()
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return QString();
    QDomElement activityElem = doc.documentElement().firstChildElement("application").firstChildElement("activity");
    return activityElem.attribute(QLatin1String("android:name"));
}

QString AndroidTarget::intentName()
{
    return packageName() + QLatin1Char('/') + activityName();
}

QString AndroidTarget::packageName()
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return QString();
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("package"));
}

bool AndroidTarget::setPackageName(const QString & name)
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("package"), name);
    return saveAndroidManifest(doc);
}

QString AndroidTarget::applicationName()
{
    QDomDocument doc;
    if (!openXmlFile(doc, androidStringsPath()))
        return QString();
    QDomElement metadataElem = doc.documentElement().firstChildElement("string");
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute("name") == "app_name")
            return metadataElem.text();
        metadataElem = metadataElem.nextSiblingElement("string");
    }
    return QString();
}

bool AndroidTarget::setApplicationName(const QString & name)
{
    QDomDocument doc;
    if (!openXmlFile(doc, androidStringsPath()))
        return false;
    QDomElement metadataElem =doc.documentElement().firstChildElement("string");
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute("name") == "app_name") {
            metadataElem.removeChild(metadataElem.firstChild());
            metadataElem.appendChild(doc.createTextNode(name));
            break;
        }
        metadataElem = metadataElem.nextSiblingElement("string");
    }
    return saveXmlFile(doc, androidStringsPath());
}

QStringList AndroidTarget::availableTargetApplications()
{
    QStringList apps;
    Qt4Project * qt4Project = qobject_cast<Qt4Project *>(project());
    foreach (Qt4ProFileNode * proFile, qt4Project->applicationProFiles()) {
        if (proFile->projectType() == ApplicationTemplate) {
            if (proFile->targetInformation().target.startsWith(QLatin1String("lib"))
                    && proFile->targetInformation().target.endsWith(QLatin1String(".so")))
                apps<<proFile->targetInformation().target.mid(3, proFile->targetInformation().target.lastIndexOf(QChar('.'))-3);
            else
                apps<<proFile->targetInformation().target;
        }
    }
    apps.sort();
    return apps;
}

QString AndroidTarget::targetApplication()
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return QString();
    QDomElement metadataElem = doc.documentElement().firstChildElement("application").firstChildElement("activity").firstChildElement("meta-data");
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute("android:name") == "android.app.lib_name")
            return metadataElem.attribute("android:value");
        metadataElem = metadataElem.nextSiblingElement("meta-data");
    }
    return QString();
}

bool AndroidTarget::setTargetApplication(const QString & name)
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return false;
    QDomElement metadataElem = doc.documentElement().firstChildElement("application").firstChildElement("activity").firstChildElement("meta-data");
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute("android:name") == "android.app.lib_name") {
            metadataElem.setAttribute(QLatin1String("android:value"), name);
            return saveAndroidManifest(doc);
        }
        metadataElem = metadataElem.nextSiblingElement("meta-data");
    }
    return false;
}

QString AndroidTarget::targetApplicationPath()
{
    QString selectedApp=targetApplication();
    if (!selectedApp.length())
        return QString();
    Qt4Project *qt4Project = qobject_cast<Qt4Project *>(project());
    foreach (Qt4ProFileNode *proFile, qt4Project->applicationProFiles()) {
        if (proFile->projectType() == ApplicationTemplate) {
            if (proFile->targetInformation().target.startsWith(QLatin1String("lib"))
                    && proFile->targetInformation().target.endsWith(QLatin1String(".so"))) {
                if (proFile->targetInformation().target.mid(3, proFile->targetInformation().target.lastIndexOf(QChar('.'))-3)
                        == selectedApp)
                    return proFile->targetInformation().buildDir+"/"+proFile->targetInformation().target;
            } else {
                if (proFile->targetInformation().target == selectedApp)
                    return proFile->targetInformation().buildDir+"/lib"+proFile->targetInformation().target+".so";
            }
        }
    }
    return QString();
}

int AndroidTarget::versionCode()
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return 0;
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("android:versionCode")).toInt();
}

bool AndroidTarget::setVersionCode(int version)
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("android:versionCode"), version);
    return saveAndroidManifest(doc);
}


QString AndroidTarget::versionName()
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return 0;
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("android:versionName"));
}

bool AndroidTarget::setVersionName(const QString &version)
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("android:versionName"), version);
    return saveAndroidManifest(doc);
}

QStringList AndroidTarget::permissions()
{
    QStringList per;
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return per;
    QDomElement permissionElem = doc.documentElement().firstChildElement("uses-permission");
    while (!permissionElem.isNull()) {
        per << permissionElem.attribute(QLatin1String("android:name"));
        permissionElem = permissionElem.nextSiblingElement("uses-permission");
    }
    return per;
}

bool AndroidTarget::setPermissions(const QStringList & permissions)
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return false;
    QDomElement docElement = doc.documentElement();
    QDomElement permissionElem = docElement.firstChildElement("uses-permission");
    while (!permissionElem.isNull()) {
        docElement.removeChild(permissionElem);
        permissionElem = docElement.firstChildElement("uses-permission");
    }

    foreach (const QString &permission, permissions ) {
        permissionElem = doc.createElement("uses-permission");
        permissionElem.setAttribute("android:name", permission);
        docElement.appendChild(permissionElem);
    }

    return saveAndroidManifest(doc);
}


QStringList AndroidTarget::getDependencies(const QString & readelfPath, const QString & lib)
{
    QStringList libs;

    QProcess readelfProc;
    readelfProc.start(readelfPath, QStringList()<<"-d"<<"-W"<<lib);

    if (!readelfProc.waitForFinished(-1)) {
        readelfProc.terminate();
        return libs;
    }

    QList<QByteArray> lines=readelfProc.readAll().trimmed().split('\n');
    foreach (const QByteArray &line, lines) {
        if (line.contains("(NEEDED)") && line.contains("Shared library:") ) {
            const int pos = line.lastIndexOf('[')+1;
            libs << line.mid(pos,line.lastIndexOf(']')-pos);
        }
    }
    return libs;
}

int AndroidTarget::setLibraryLevel(const QString & library, LibrariesMap & mapLibs)
{
    int maxlevel=mapLibs[library].level;
    if (maxlevel>0)
        return maxlevel;
    foreach (QString lib, mapLibs[library].dependencies) {
        foreach (const QString &key, mapLibs.keys()) {
            if (library == key)
                continue;
            if (key == lib) {
                int libLevel = mapLibs[key].level;

                if (libLevel < 0)
                    libLevel = setLibraryLevel(key, mapLibs);

                if (libLevel > maxlevel)
                    maxlevel = libLevel;
                break;
            }
        }
    }
    if (mapLibs[library].level < 0)
        mapLibs[library].level = maxlevel+1;
    return maxlevel+1;
}

bool AndroidTarget::QtLibrariesLessThan(const Library &a, const Library &b)
{
    if (a.level == b.level)
        return a.name < b.name;
    return a.level < b.level;
}

QStringList AndroidTarget::availableQtLibs()
{
    const QString readelfPath = AndroidConfigurations::instance().readelfPath(activeRunConfiguration()->abi().architecture());
    QStringList libs;
    const Qt4Project *const qt4Project = qobject_cast<const Qt4Project *>(project());
    if (!qt4Project || !qt4Project->activeTarget()->activeBuildConfiguration()->qtVersion())
        return libs;
    QString qtLibsPath = qt4Project->activeTarget()->activeBuildConfiguration()->qtVersion()->versionInfo()["QT_INSTALL_LIBS"];
    if (!QFile::exists(readelfPath)) {
        QDirIterator libsIt(qtLibsPath, QStringList() << "libQt*.so");
        while (libsIt.hasNext()) {
            libsIt.next();
            libs << libsIt.fileName().mid(3,libsIt.fileName().indexOf('.') - 3);
        }
        libs.sort();
        return libs;
    }
    LibrariesMap mapLibs;
    QDir libPath;
    QDirIterator it(qtLibsPath, QStringList() << "*.so", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        libPath = it.next();
        const QString library = libPath.absolutePath().mid(libPath.absolutePath().lastIndexOf('/') + 1);
        mapLibs[library].dependencies = getDependencies(readelfPath, libPath.absolutePath());
    }

    // clean dependencies
    foreach (const QString & key, mapLibs.keys()) {
        int it = 0;
        while (it < mapLibs[key].dependencies.size()) {
            const QString &dependName = mapLibs[key].dependencies[it];
            if (!mapLibs.keys().contains(dependName) && dependName.startsWith("lib") && dependName.endsWith(".so")) {
                mapLibs[key].dependencies.removeAt(it);
            } else {
                ++it;
            }
        }
        if (!mapLibs[key].dependencies.size())
            mapLibs[key].level = 0;
    }

    QVector<Library> qtLibraries;
    // calculate the level for every library
    foreach (const QString &key, mapLibs.keys()) {
        if (mapLibs[key].level < 0)
           setLibraryLevel(key, mapLibs);

        if (!mapLibs[key].name.length() && key.startsWith("lib") && key.endsWith(".so"))
            mapLibs[key].name = key.mid(3,key.length() - 6);

        for (int it = 0; it < mapLibs[key].dependencies.size(); it++) {
            const QString &libName = mapLibs[key].dependencies[it];
            if (libName.startsWith("lib") && libName.endsWith(".so"))
                mapLibs[key].dependencies[it] = libName.mid(3,libName.length() - 6);
        }
        qtLibraries.push_back(mapLibs[key]);
    }
    qSort(qtLibraries.begin(), qtLibraries.end(), QtLibrariesLessThan);
    foreach (Library lib, qtLibraries) {
        libs.push_back(lib.name);
    }
    return libs;
}

QIcon AndroidTarget::androidIcon(AndroidIconType type)
{
    switch (type) {
    case HighDPI:
        return QIcon(androidDirPath()+QString("/res/drawable-hdpi/icon.png"));
    case MediumDPI:
        return QIcon(androidDirPath()+QString("/res/drawable-mdpi/icon.png"));
    case LowDPI:
        return QIcon(androidDirPath()+QString("/res/drawable-ldpi/icon.png"));
    }
    return QIcon();
}

bool AndroidTarget::setAndroidIcon(AndroidIconType type, const QString &iconFileName)
{
    switch (type) {
    case HighDPI:
        QFile::remove(androidDirPath() + QString("/res/drawable-hdpi/icon.png"));
        return QFile::copy(iconFileName, androidDirPath() + QString("/res/drawable-hdpi/icon.png"));
    case MediumDPI:
        QFile::remove(androidDirPath() + QString("/res/drawable-mdpi/icon.png"));
        return QFile::copy(iconFileName, androidDirPath() + QString("/res/drawable-mdpi/icon.png"));
    case LowDPI:
        QFile::remove(androidDirPath() + QString("/res/drawable-ldpi/icon.png"));
        return QFile::copy(iconFileName, androidDirPath() + QString("/res/drawable-ldpi/icon.png"));
    }
    return false;
}

QStringList AndroidTarget::libsXml(const QString &tag)
{
    QStringList libs;
    QDomDocument doc;
    if (!openLibsXml(doc))
        return libs;
    QDomElement arrayElem = doc.documentElement().firstChildElement("array");
    while (!arrayElem.isNull()) {
        if (arrayElem.attribute(QLatin1String("name")) == tag) {
            arrayElem = arrayElem.firstChildElement("item");
            while (!arrayElem.isNull()) {
                libs << arrayElem.text();
                arrayElem = arrayElem.nextSiblingElement("item");
            }
            return libs;
        }
        arrayElem = arrayElem.nextSiblingElement("array");
    }
    return libs;
}

bool AndroidTarget::setLibsXml(const QStringList &libs, const QString &tag)
{
    QDomDocument doc;
    if (!openLibsXml(doc))
        return false;
    QDomElement arrayElem = doc.documentElement().firstChildElement("array");
    while (!arrayElem.isNull()) {
        if (arrayElem.attribute(QLatin1String("name")) == tag) {
            doc.documentElement().removeChild(arrayElem);
            arrayElem = doc.createElement("array");
            arrayElem.setAttribute(QLatin1String("name"), tag);
            foreach (const QString &lib, libs) {
                QDomElement item = doc.createElement("item");
                item.appendChild(doc.createTextNode(lib));
                arrayElem.appendChild(item);
            }
            doc.documentElement().appendChild(arrayElem);
            return saveLibsXml(doc);
        }
        arrayElem = arrayElem.nextSiblingElement("array");
    }
    return false;
}

QStringList AndroidTarget::qtLibs()
{
    return libsXml("qt_libs");
}

bool AndroidTarget::setQtLibs(const QStringList & libs)
{
    return setLibsXml(libs, "qt_libs");
}

QStringList AndroidTarget::availablePrebundledLibs()
{
    QStringList libs;
    Qt4Project * qt4Project = qobject_cast<Qt4Project *>(project());
    QList<Qt4Project *> qt4Projects;
    qt4Projects << qt4Project;
    foreach (ProjectExplorer::Project *pr, qt4Project->dependsOn()) {
        qt4Project = qobject_cast<Qt4Project *>(pr);
        if (qt4Project)
            qt4Projects << qt4Project;
    }

    foreach (Qt4Project * qt4Project, qt4Projects)
        foreach (Qt4ProFileNode * node, qt4Project->allProFiles())
            if (node->projectType() == LibraryTemplate)
                libs << QLatin1String("lib") + node->targetInformation().target + QLatin1String(".so");

    return libs;
}

QStringList AndroidTarget::prebundledLibs()
{
    return libsXml("bundled_libs");
}

bool AndroidTarget::setPrebundledLibs(const QStringList & libs)
{

    return setLibsXml(libs, "bundled_libs");
}

QIcon AndroidTarget::highDpiIcon()
{
    return androidIcon(HighDPI);
}

bool AndroidTarget::setHighDpiIcon(const QString &iconFilePath)
{
    return setAndroidIcon(HighDPI, iconFilePath);
}

QIcon AndroidTarget::mediumDpiIcon()
{
    return androidIcon(MediumDPI);
}

bool AndroidTarget::setMediumDpiIcon(const QString &iconFilePath)
{
    return setAndroidIcon(MediumDPI, iconFilePath);
}

QIcon AndroidTarget::lowDpiIcon()
{
    return androidIcon(LowDPI);
}

bool AndroidTarget::setLowDpiIcon(const QString &iconFilePath)
{
    return setAndroidIcon(LowDPI, iconFilePath);
}

QString AndroidTarget::targetSDK()
{
    if (!createAndroidTemplatesIfNecessary())
        return AndroidConfigurations::instance().bestMatch("android-8");
    QFile file(androidDefaultPropertiesPath());
    if (!file.open(QIODevice::ReadOnly))
        return AndroidConfigurations::instance().bestMatch("android-8");
    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        if (line.startsWith("target="))
            return line.trimmed().mid(7);
    }
    return AndroidConfigurations::instance().bestMatch("android-8");
}

bool AndroidTarget::setTargetSDK(const QString & target)
{
    updateProject(target, applicationName());
    return true;
}

void AndroidTarget::raiseError(const QString &reason)
{
    QMessageBox::critical(0, tr("Error creating Android templates"), reason);
}


} // namespace Internal
} // namespace Qt4ProjectManager
