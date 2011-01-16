/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "androidtemplatesmanager.h"
#include "androidconfigurations.h"

#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidpackagecreationstep.h"
#include "androidtoolchain.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
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
#include <QDomDocument>

#include <cctype>

using namespace ProjectExplorer;

namespace {
    const QLatin1String AndroidDirName("android");
    const QLatin1String AndroidManifestName("AndroidManifest.xml");
    const QLatin1String AndroidDefaultPropertiesName("default.properties");
} // anonymous namespace

namespace Qt4ProjectManager {
namespace Internal {

AndroidTemplatesManager *AndroidTemplatesManager::m_instance = 0;

AndroidTemplatesManager *AndroidTemplatesManager::instance(QObject *parent)
{
    Q_ASSERT(!m_instance != !parent);
    if (!m_instance)
        m_instance = new AndroidTemplatesManager(parent);
    return m_instance;
}

AndroidTemplatesManager::AndroidTemplatesManager(QObject *parent) : QObject(parent)
{
    SessionManager * const session
        = ProjectExplorerPlugin::instance()->session();
    connect(session, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
        this, SLOT(handleActiveProjectChanged(ProjectExplorer::Project*)));
    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)), this,
        SLOT(handleActiveProjectChanged(ProjectExplorer::Project*)));
    connect(session, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)),
        this, SLOT(handleProjectToBeRemoved(ProjectExplorer::Project*)));
    handleActiveProjectChanged(session->startupProject());
}

void AndroidTemplatesManager::handleActiveProjectChanged(ProjectExplorer::Project *project)
{
    if (!project || m_androidProjects.contains(project))
        return;

    connect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
        this, SLOT(handleTarget(ProjectExplorer::Target*)));
    connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
        this, SLOT(handleTarget(ProjectExplorer::Target*)));
    const QList<Target *> &targets = project->targets();
    foreach (Target * const target, targets)
        handleTarget(target);
}

bool AndroidTemplatesManager::handleTarget(ProjectExplorer::Target *target)
{

    if (!target
        || target->id() != QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID))
        return false;
    if (!createAndroidTemplatesIfNecessary(target->project()))
        return false;

//    const Qt4Target * const qt4Target = qobject_cast<Qt4Target *>(target);
//    const AndroidDeployStep * const deployStep
//        = AndroidGlobal::buildStep<AndroidDeployStep>(qt4Target->activeDeployConfiguration());
//    connect(deployStep->deployables().data(), SIGNAL(modelReset()), this,
//        SLOT(handleProFileUpdated()), Qt::QueuedConnection);

    Project * const project = target->project();
    if (m_androidProjects.contains(project))
        return true;

    QFileSystemWatcher * const fsWatcher = new QFileSystemWatcher(this);
    fsWatcher->addPath(androidDirPath(project));
    fsWatcher->addPath(androidManifestPath(project));
    connect(fsWatcher, SIGNAL(directoryChanged(QString)), this,
        SLOT(handleAndroidDirContentsChanged()));
    connect(fsWatcher, SIGNAL(fileChanged(QString)), this,
        SLOT(handleAndroidDirContentsChanged()));
    m_androidProjects.insert(project, fsWatcher);

    return true;
}

QString AndroidTemplatesManager::androidDirPath(const ProjectExplorer::Project *project)
{
    return project->projectDirectory()+QLatin1Char('/')+AndroidDirName;
}

QString AndroidTemplatesManager::androidManifestPath(const ProjectExplorer::Project *project)
{
    return androidDirPath(project)+QLatin1Char('/')+AndroidManifestName;
}

QString AndroidTemplatesManager::androidDefaultPropertiesPath(const ProjectExplorer::Project *project)
{
    return androidDirPath(project)+QLatin1Char('/')+AndroidDefaultPropertiesName;
}


void AndroidTemplatesManager::updateProject(const ProjectExplorer::Project *project, const QString &targetSDK)
{
    QString androidDir=androidDirPath(project);
    bool commentLines=targetSDK=="android-4";
    QDirIterator it(androidDir,QStringList()<<"*.java",QDir::Files, QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        it.next();
        qDebug()<<"Parsing:"<<it.filePath();
        QFile file(it.filePath());
        if (!file.open(QIODevice::ReadWrite))
            continue;
        QList<QByteArray> lines=file.readAll().split('\n');

        bool modified=false;
        bool comment=false;
        for (int i=0;i<lines.size();i++)
        {
            if (lines[i].contains("@!ANDROID-4"))
            {
                comment = !comment;
                continue;
            }
            if (!comment)
                continue;
            if (commentLines)
            {
                if (!lines[i].trimmed().startsWith("//"))
                {
                    lines[i] = "// "+lines[i];
                    modified =  true;
                }
            }
            else
            {
                if (lines[i].trimmed().startsWith("//"))
                {
                    lines[i] = lines[i].mid(3);
                    modified =  true;
                }
            }
        }
        if (modified)
        {
            file.resize(0);
            foreach(const QByteArray & line, lines)
            {
                file.write(line);
                file.write("\n");
            }
        }
        file.close();
    }

    QProcess androidProc;
    QStringList params;
    params<<"update"<<"project"<<"-p"<<androidDir;
    if (targetSDK.length())
        params<<"-t"<<targetSDK;
    androidProc.start(AndroidConfigurations::instance().androidToolPath(), params);
    androidProc.waitForFinished();
}

bool AndroidTemplatesManager::createAndroidTemplatesIfNecessary(ProjectExplorer::Project *project, bool forceJava)
{
    const Qt4Project * qt4Project = qobject_cast<Qt4Project*>(project);
    if (!qt4Project)
        return false;
    QDir projectDir(project->projectDirectory());
    QString androidPath=androidDirPath(project);

    if (!forceJava && QFileInfo(androidPath).exists()
            && QFileInfo(androidManifestPath(project)).exists()
            && QFileInfo(androidPath+QLatin1String("/src")).exists()
            && QFileInfo(androidPath+QLatin1String("/res")).exists())
        return true;

    if (!QFileInfo(androidDirPath(project)).exists())
        if (!projectDir.mkdir(AndroidDirName) && !forceJava)
        {
            raiseError(tr("Error creating Android directory '%1'.")
                .arg(AndroidDirName));
            return false;
        }

    QList<QtVersion*> versions=QtVersionManager::instance()->versionsForTargetId(QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID));
    if (!versions.size())
    {
        raiseError(tr("Not enough Qt for Android SDKs found.\nPlease install at least one SDK."));
        return false;
    }

    if (forceJava)
        AndroidPackageCreationStep::removeDirectory(AndroidDirName+QLatin1String("/src"));

    QStringList androidFiles;
    QDirIterator it(versions[0]->sourcePath()+QLatin1String("/src/android/java"),QDirIterator::Subdirectories);
    int pos=it.path().size();
    while(it.hasNext())
    {
        it.next();
        if (it.fileInfo().isDir())
            projectDir.mkpath(AndroidDirName+it.filePath().mid(pos));
        else
        {
            QFile::copy(it.filePath(), androidPath+it.filePath().mid(pos));
            androidFiles<<androidPath+it.filePath().mid(pos);
        }
    }
    qt4Project->rootProjectNode()->addFiles(UnknownFileType,androidFiles);

    QStringList sdks=AndroidConfigurations::instance().sdkTargets();
    if (!sdks.size())
    {
        raiseError(tr("Not enough SDK's found.\nPlease install at least one SDK."));
        return false;
    }
    updateProject(project, AndroidConfigurations::instance().sdkTargets().at(0));
    return true;
}

bool AndroidTemplatesManager::openAndroidManifest(ProjectExplorer::Project *project, QDomDocument & doc)
{
    if (!createAndroidTemplatesIfNecessary(project))
        return false;

    QFile f(androidManifestPath(project));
    if (!f.open(QIODevice::ReadOnly))
    {
        raiseError(tr("Can't open AndroidManifest.xml file '%1'").arg(androidManifestPath(project)));
        return false;
    }
    if (!doc.setContent(f.readAll()))
    {
        raiseError(tr("Not enough SDK's found.\nPlease install at least one SDK."));
        return false;
    }
    return true;
}

bool AndroidTemplatesManager::saveAndroidManifest(ProjectExplorer::Project *project, QDomDocument & doc)
{
    if (!createAndroidTemplatesIfNecessary(project))
        return false;

    QFile f(androidManifestPath(project));
    if (!f.open(QIODevice::WriteOnly))
    {
        raiseError(tr("Can't open AndroidManifest.xml file '%1'").arg(androidManifestPath(project)));
        return false;
    }
    return f.write(doc.toByteArray(4))>=0;
}

QString AndroidTemplatesManager::activityName(ProjectExplorer::Project *project)
{
    QDomDocument doc;
    if (!openAndroidManifest(project, doc))
        return QString();
    QDomElement activityElem = doc.documentElement().firstChildElement("application").firstChildElement("activity");
    return activityElem.attribute(QLatin1String("android:name"));
}

QString AndroidTemplatesManager::intentName(ProjectExplorer::Project *project)
{
    return packageName(project)+QLatin1Char('/')+activityName(project);
}

QString AndroidTemplatesManager::packageName(ProjectExplorer::Project *project)
{
    QDomDocument doc;
    if (!openAndroidManifest(project, doc))
        return QString();
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("package"));
}

bool AndroidTemplatesManager::setPackageName(ProjectExplorer::Project *project, const QString & name)
{
    QDomDocument doc;
    if (!openAndroidManifest(project, doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("package"), name);
    return saveAndroidManifest(project, doc);
}

QString AndroidTemplatesManager::applicationName(ProjectExplorer::Project *project)
{
    QDomDocument doc;
    if (!openAndroidManifest(project, doc))
        return QString();
    QDomElement activityElem = doc.documentElement().firstChildElement("application").firstChildElement("activity");
    return activityElem.attribute(QLatin1String("android:label"));
}

bool AndroidTemplatesManager::setApplicationName(ProjectExplorer::Project *project, const QString & name)
{
    QDomDocument doc;
    if (!openAndroidManifest(project, doc))
        return false;
    QDomElement activityElem = doc.documentElement().firstChildElement("application").firstChildElement("activity");
    activityElem.setAttribute(QLatin1String("android:label"), name);
    return saveAndroidManifest(project, doc);
}

int AndroidTemplatesManager::versionCode(ProjectExplorer::Project *project)
{
    QDomDocument doc;
    if (!openAndroidManifest(project, doc))
        return 0;
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("android:versionCode")).toInt();
}

bool AndroidTemplatesManager::setVersionCode(ProjectExplorer::Project *project, int version)
{
    QDomDocument doc;
    if (!openAndroidManifest(project, doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("android:versionCode"), version);
    return saveAndroidManifest(project, doc);
}


QString AndroidTemplatesManager::versionName(ProjectExplorer::Project *project)
{
    QDomDocument doc;
    if (!openAndroidManifest(project, doc))
        return 0;
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("android:versionName"));
}

bool AndroidTemplatesManager::setVersionName(ProjectExplorer::Project *project, const QString &version)
{
    QDomDocument doc;
    if (!openAndroidManifest(project, doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("android:versionName"), version);
    return saveAndroidManifest(project, doc);
}

QStringList AndroidTemplatesManager::permissions(ProjectExplorer::Project *project)
{
    QStringList per;
    QDomDocument doc;
    if (!openAndroidManifest(project, doc))
        return per;
    QDomElement permissionElem = doc.documentElement().firstChildElement("uses-permission");
    while(!permissionElem.isNull())
    {
        per<<permissionElem.attribute(QLatin1String("android:name"));
        permissionElem = permissionElem.nextSiblingElement("uses-permission");
    }
    return per;
}

bool AndroidTemplatesManager::setPermissions(ProjectExplorer::Project *project, const QStringList & permissions)
{
    QDomDocument doc;
    if (!openAndroidManifest(project, doc))
        return false;
    QDomElement docElement=doc.documentElement();
    QDomElement permissionElem = docElement.firstChildElement("uses-permission");
    while(!permissionElem.isNull())
    {
        doc.removeChild(permissionElem);
        permissionElem = docElement.firstChildElement("uses-permission");
    }

    foreach(const QString & permission, permissions )
    {
        permissionElem = doc.createElement("uses-permission");
        permissionElem.setAttribute("android:name", permission);
        docElement.appendChild(permissionElem);
    }

    return saveAndroidManifest(project, doc);
}

QStringList AndroidTemplatesManager::availableQtLibs(ProjectExplorer::Project *project)
{
    QStringList libs;
    const Qt4Project * const qt4Project = qobject_cast<const Qt4Project *>(project);
    QString libsPath=qt4Project->activeTarget()->activeBuildConfiguration()->qtVersion()->sourcePath()+"/lib";
    QDirIterator libsIt(libsPath, QStringList()<<"libQt*.so");
    while(libsIt.hasNext())
    {
        libsIt.next();
        libs<<libsIt.fileName().mid(5,libsIt.fileName().indexOf('.')-5);
    }
    return libs;
}

QStringList AndroidTemplatesManager::qtLibs(ProjectExplorer::Project *project)
{
    QStringList libs;

    return libs;
}

bool AndroidTemplatesManager::setQtLibs(ProjectExplorer::Project *project, const QStringList & qtLibs)
{

    return true;
}

QStringList AndroidTemplatesManager::availablePrebundledLibs(ProjectExplorer::Project *project)
{
    QStringList libs;
    Qt4Project * qt4Project = qobject_cast<Qt4Project *>(project);
    QList<Qt4Project *> qt4Projects;
    qt4Projects<<qt4Project;
    foreach (ProjectExplorer::Project* pr, qt4Project->dependsOn())
    {
        qt4Project = qobject_cast<Qt4Project *>(pr);
        if (qt4Project)
            qt4Projects<<qt4Project;
    }

    foreach(qt4Project, qt4Projects)
        foreach(Qt4ProFileNode * node, qt4Project->leafProFiles())
            if (node->projectType()== LibraryTemplate)
                libs<<QLatin1String("lib")+node->targetInformation().target+QLatin1String(".so");

    return libs;
}

QStringList AndroidTemplatesManager::prebundledLibs(ProjectExplorer::Project *project)
{
    QStringList libs;

    return libs;
}

bool AndroidTemplatesManager::setPrebundledLibs(ProjectExplorer::Project *project, const QStringList & qtLibs)
{

    return true;
}

QString AndroidTemplatesManager::targetSDK(ProjectExplorer::Project *project)
{
    if (!createAndroidTemplatesIfNecessary(project))
        return "android-4";
    QFile file(androidDefaultPropertiesPath(project));
    if (!file.open(QIODevice::ReadOnly))
        return "android-4";
    while(!file.atEnd())
    {
        QByteArray line=file.readLine();
        if (line.startsWith("target="))
            return line.trimmed().mid(7);
    }
    return "android-4";
}

bool AndroidTemplatesManager::setTargetSDK(ProjectExplorer::Project *project, const QString & target)
{
    updateProject(project, target);
    return true;
}


//    QString error;
//    const Qt4Target * const qt4Target = qobject_cast<const Qt4Target *>(target);
//    Q_ASSERT_X(qt4Target, Q_FUNC_INFO, "Target ID does not match actual type.");
//    const Qt4BuildConfiguration * const bc
//        = qt4Target->activeBuildConfiguration();
//    const AndroidToolChain * const tc
//        = dynamic_cast<AndroidToolChain *>(bc->toolChain());
//    if (!tc) {
//        qDebug("Android target has no Android toolchain.");
//        return false;
//    }
//    if (!AndroidPackageCreationStep::preparePackagingProcess(&dh_makeProc, bc,
//        projectDir.path() + QLatin1Char('/') + PackagingDirName, &error)) {
//        raiseError(error);
//        return false;
//    }

//    const QString dhMakeDebianDir = projectDir.path() + QLatin1Char('/')
//        + PackagingDirName + QLatin1String("/debian");
//    AndroidPackageCreationStep::removeDirectory(dhMakeDebianDir);
//    const QString command = QLatin1String("dh_make -s -n -p ")
//        + AndroidPackageCreationStep::packageName(project) + QLatin1Char('_')
//        + AndroidPackageCreationStep::DefaultVersionNumber;
//    dh_makeProc.start(AndroidPackageCreationStep::packagingCommand(tc, command));
//    if (!dh_makeProc.waitForStarted()) {
//        raiseError(tr("Unable to create Debian templates: dh_make failed (%1)")
//            .arg(dh_makeProc.errorString()));
//        return false;
//    }
//    dh_makeProc.write("\n"); // Needs user input.
//    dh_makeProc.waitForFinished(-1);
//    if (dh_makeProc.error() != QProcess::UnknownError
//        || dh_makeProc.exitCode() != 0) {
//        raiseError(tr("Unable to create debian templates: dh_make failed (%1)")
//            .arg(dh_makeProc.errorString()));
//        return false;
//    }

//    if (!QFile::rename(dhMakeDebianDir, debianDirPath(project))) {
//        raiseError(tr("Unable to move new debian directory to '%1'.")
//            .arg(QDir::toNativeSeparators(debianDirPath(project))));
//        AndroidPackageCreationStep::removeDirectory(dhMakeDebianDir);
//        return false;
//    }

//    QDir debianDir(debianDirPath(project));
//    const QStringList &files = debianDir.entryList(QDir::Files);
//    foreach (const QString &fileName, files) {
//        if (fileName.endsWith(QLatin1String(".ex"), Qt::CaseInsensitive)
//            || fileName.compare(QLatin1String("README.debian"), Qt::CaseInsensitive) == 0
//            || fileName.compare(QLatin1String("dirs"), Qt::CaseInsensitive) == 0
//            || fileName.compare(QLatin1String("docs"), Qt::CaseInsensitive) == 0) {
//            debianDir.remove(fileName);
//        }
//    }

//    return adaptRulesFile(project) && adaptControlFile(project);
//}

//bool AndroidTemplatesManager::adaptRulesFile(const Project *project)
//{
//    const QString rulesFilePath = debianDirPath(project) + "/rules";
//    QFile rulesFile(rulesFilePath);
//    if (!rulesFile.open(QIODevice::ReadWrite)) {
//        raiseError(tr("Packaging Error: Cannot open file '%1'.")
//                   .arg(QDir::toNativeSeparators(rulesFilePath)));
//        return false;
//    }
//    QByteArray rulesContents = rulesFile.readAll();
//    rulesContents.replace("DESTDIR", "INSTALL_ROOT");
//    rulesContents.replace("dh_shlibdeps", "# dh_shlibdeps");
////    rulesContents.replace("$(MAKE) clean", "# $(MAKE) clean");
////    const Qt4Project * const qt4Project
////        = static_cast<const Qt4Project *>(project);
////    const QString proFileName
////        = QFileInfo(qt4Project->rootProjectNode()->path()).fileName();
////    rulesContents.replace("# Add here commands to configure the package.",
////        "qmake " + proFileName.toLocal8Bit());

//    // Would be the right solution, but does not work (on Windows),
//    // because dpkg-genchanges doesn't know about it (and can't be told).
//    // rulesContents.replace("dh_builddeb", "dh_builddeb --destdir=.");

//    rulesFile.resize(0);
//    rulesFile.write(rulesContents);
//    rulesFile.close();
//    if (rulesFile.error() != QFile::NoError) {
//        raiseError(tr("Packaging Error: Cannot write file '%1'.")
//                   .arg(QDir::toNativeSeparators(rulesFilePath)));
//        return false;
//    }


bool AndroidTemplatesManager::adaptControlFile(const Project *project)
{
//    QFile controlFile(controlFilePath(project));
//    if (!controlFile.open(QIODevice::ReadWrite)) {
//        raiseError(tr("Packaging Error: Cannot open file '%1'.")
//                   .arg(QDir::toNativeSeparators(controlFilePath(project))));
//        return false;
//    }

//    QByteArray controlContents = controlFile.readAll();

//    adaptControlFileField(controlContents, "Section", "user/hidden");
//    adaptControlFileField(controlContents, "Priority", "optional");
//    const int buildDependsOffset = controlContents.indexOf("Build-Depends:");
//    if (buildDependsOffset == -1) {
//        qDebug("Unexpected: no Build-Depends field in debian control file.");
//    } else {
//        int buildDependsNewlineOffset
//            = controlContents.indexOf('\n', buildDependsOffset);
//        if (buildDependsNewlineOffset == -1) {
//            controlContents += '\n';
//            buildDependsNewlineOffset = controlContents.length() - 1;
//        }
//        controlContents.insert(buildDependsNewlineOffset,
//            ", libqt4-dev");
//    }

//    controlFile.resize(0);
//    controlFile.write(controlContents);
//    controlFile.close();
//    if (controlFile.error() != QFile::NoError) {
//        raiseError(tr("Packaging Error: Cannot write file '%1'.")
//                   .arg(QDir::toNativeSeparators(controlFilePath(project))));
//        return false;
//    }
    return true;
}

void AndroidTemplatesManager::adaptControlFileField(QByteArray &document,
    const QByteArray &fieldName, const QByteArray &newFieldValue)
{
//    QByteArray adaptedLine = fieldName + ": " + newFieldValue;
//    const int lineOffset = document.indexOf(fieldName + ":");
//    if (lineOffset == -1) {
//        document.append(adaptedLine).append('\n');
//    } else {
//        int newlineOffset = document.indexOf('\n', lineOffset);
//        if (newlineOffset == -1) {
//            newlineOffset = document.length();
//            adaptedLine += '\n';
//        }
//        document.replace(lineOffset, newlineOffset - lineOffset, adaptedLine);
//    }
}

bool AndroidTemplatesManager::updateDesktopFiles(const Qt4Target *target)
{
//    const Qt4Target * const qt4Target = qobject_cast<const Qt4Target *>(target);
//    Q_ASSERT_X(qt4Target, Q_FUNC_INFO,
//        "Impossible: Target has Android id, but could not be cast to Qt4Target.");
//    const QList<Qt4ProFileNode *> &applicationProjects
//        = qt4Target->qt4Project()->applicationProFiles();
    bool success = true;
//    foreach (Qt4ProFileNode *proFileNode, applicationProjects)
//        success &= updateDesktopFile(qt4Target, proFileNode);
    return success;
}

bool AndroidTemplatesManager::updateDesktopFile(const Qt4Target *target,
    Qt4ProFileNode *proFileNode)
{
//    const QString appName = proFileNode->targetInformation().target;
//    const QString desktopFilePath = QFileInfo(proFileNode->path()).path()
//        + QLatin1Char('/') + appName + QLatin1String(".desktop");
//    QFile desktopFile(desktopFilePath);
//    const bool existsAlready = desktopFile.exists();
//    if (!desktopFile.open(QIODevice::ReadWrite)) {
//        qWarning("Failed to open '%s': %s", qPrintable(desktopFilePath),
//            qPrintable(desktopFile.errorString()));
//        return false;
//    }

//    const QByteArray desktopTemplate("[Desktop Entry]\nEncoding=UTF-8\n"
//        "Version=1.0\nType=Application\nTerminal=false\nName=\nExec=\n"
//        "Icon=\nX-Window-Icon=\nX-HildonDesk-ShowInToolbar=true\n"
//        "X-Osso-Type=application/x-executable\n");
//    QByteArray desktopFileContents
//        = existsAlready ? desktopFile.readAll() : desktopTemplate;

//    QString executable;
//    const QSharedPointer<AndroidDeployables> &deployables
//        = AndroidGlobal::buildStep<AndroidDeployStep>(target->activeDeployConfiguration())
//            ->deployables();
//    for (int i = 0; i < deployables->modelCount(); ++i) {
//        const AndroidDeployableListModel * const model = deployables->modelAt(i);
//        if (model->proFilePath() == proFileNode->path()) {
//            executable = model->remoteExecutableFilePath();
//            break;
//        }
//    }
//    if (executable.isEmpty()) {
//        qWarning("Strange: Project file node not managed by AndroidDeployables.");
//    } else {
//        int execNewLinePos, execValuePos;
//        findLine("Exec=", desktopFileContents, execNewLinePos, execValuePos);
//        desktopFileContents.replace(execValuePos, execNewLinePos - execValuePos,
//            executable.toUtf8());
//    }

//    int nameNewLinePos, nameValuePos;
//    findLine("Name=", desktopFileContents, nameNewLinePos, nameValuePos);
//    if (nameNewLinePos == nameValuePos)
//        desktopFileContents.insert(nameValuePos, appName.toUtf8());
//    int iconNewLinePos, iconValuePos;
//    findLine("Icon=", desktopFileContents, iconNewLinePos, iconValuePos);
//    if (iconNewLinePos == iconValuePos)
//        desktopFileContents.insert(iconValuePos, appName.toUtf8());

//    desktopFile.resize(0);
//    desktopFile.write(desktopFileContents);
//    desktopFile.close();
//    if (desktopFile.error() != QFile::NoError) {
//        qWarning("Could not write '%s': %s", qPrintable(desktopFilePath),
//            qPrintable(desktopFile.errorString()));
//    }

//    if (!existsAlready) {
//        proFileNode->addFiles(UnknownFileType,
//            QStringList() << desktopFilePath);
//        QFile proFile(proFileNode->path());
//        if (!proFile.open(QIODevice::ReadWrite)) {
//            qWarning("Failed to open '%s': %s", qPrintable(proFileNode->path()),
//                qPrintable(proFile.errorString()));
//            return false;
//        }
//        QByteArray proFileContents = proFile.readAll();
//        proFileContents += "\nunix:!symbian {\n"
//            "    desktopfile.files = $${TARGET}.desktop\n"
//            "    maemo5 {\n"
//            "        desktopfile.path = /usr/share/applications/hildon\n"
//            "    } else {\n"
//            "        desktopfile.path = /usr/share/applications\n    }\n"
//            "    INSTALLS += desktopfile\n}\n";
//        proFile.resize(0);
//        proFile.write(proFileContents);
//        proFile.close();
//        if (proFile.error() != QFile::NoError) {
//            qWarning("Could not write '%s': %s", qPrintable(proFileNode->path()),
//                qPrintable(proFile.errorString()));
//            return false;
//        }
//    }
    return true;
}

void AndroidTemplatesManager::handleProjectToBeRemoved(ProjectExplorer::Project *project)
{
//    AndroidProjectMap::Iterator it = m_androidProjects.find(project);
//    if (it != m_androidProjects.end()) {
//        delete it.value();
//        m_androidProjects.erase(it);
//    }
}

void AndroidTemplatesManager::handleProFileUpdated()
{
//    const AndroidDeployables * const deployables
//        = qobject_cast<AndroidDeployables *>(sender());
//    if (!deployables)
//        return;
//    const Target * const target = deployables->buildStep()->target();
//    if (m_androidProjects.contains(target->project()))
//        updateDesktopFiles(qobject_cast<const Qt4Target *>(target));
}

//QString AndroidTemplatesManager::version(const Project *project)
//{
//    return "1.0";
//    QSharedPointer<QFile> changeLog
//        = openFile(changeLogFilePath(project), QIODevice::ReadOnly, error);
//    if (!changeLog)
//        return QString();
//    const QByteArray &firstLine = changeLog->readLine();
//    const int openParenPos = firstLine.indexOf('(');
//    if (openParenPos == -1) {
//        *error = tr("Debian changelog file '%1' has unexpected format.")
//                .arg(QDir::toNativeSeparators(changeLog->fileName()));
//        return QString();
//    }
//    const int closeParenPos = firstLine.indexOf(')', openParenPos);
//    if (closeParenPos == -1) {
//        *error = tr("Debian changelog file '%1' has unexpected format.")
//                .arg(QDir::toNativeSeparators(changeLog->fileName()));
//        return QString();
//    }
//    return QString::fromUtf8(firstLine.mid(openParenPos + 1,
//        closeParenPos - openParenPos - 1).data());
//}

//bool AndroidTemplatesManager::setVersion(const Project *project, const QString &version)
//{
//    QSharedPointer<QFile> changeLog
//        = openFile(changeLogFilePath(project), QIODevice::ReadWrite, error);
//    if (!changeLog)
//        return false;

//    QString content = QString::fromUtf8(changeLog->readAll());
//    content.replace(QRegExp(QLatin1String("\\([a-zA-Z0-9_\\.]+\\)")),
//        QLatin1Char('(') + version + QLatin1Char(')'));
//    changeLog->resize(0);
//    changeLog->write(content.toUtf8());
//    changeLog->close();
//    if (changeLog->error() != QFile::NoError) {
//        *error = tr("Error writing Debian changelog file '%1': %2")
//            .arg(QDir::toNativeSeparators(changeLog->fileName()),
//                 changeLog->errorString());
//        return false;
//    }
//    return true;
//}

QIcon AndroidTemplatesManager::packageManagerIcon(Project *project)
{
//    QSharedPointer<QFile> controlFile
//        = openFile(controlFilePath(project), QIODevice::ReadOnly, error);
//    if (!controlFile)
        return QIcon();

//    bool iconFieldFound = false;
//    QByteArray currentLine;
//    while (!iconFieldFound && !controlFile->atEnd()) {
//        currentLine = controlFile->readLine();
//        iconFieldFound = currentLine.startsWith(IconFieldName);
//    }
//    if (!iconFieldFound)
//        return QIcon();

//    int pos = IconFieldName.length();
//    currentLine = currentLine.trimmed();
//    QByteArray base64Icon;
//    do {
//        while (pos < currentLine.length())
//            base64Icon += currentLine.at(pos++);
//        do
//            currentLine = controlFile->readLine();
//        while (currentLine.startsWith('#'));
//        if (currentLine.isEmpty() || !isspace(currentLine.at(0)))
//            break;
//        currentLine = currentLine.trimmed();
//        if (currentLine.isEmpty())
//            break;
//        pos = 0;
//    } while (true);
//    QPixmap pixmap;
//    if (!pixmap.loadFromData(QByteArray::fromBase64(base64Icon))) {
//        *error = tr("Invalid icon data in Debian control file.");
//        return QIcon();
//    }
//    return QIcon(pixmap);
}

bool AndroidTemplatesManager::setPackageManagerIcon(Project *project, const QString &iconFilePath)
{
//    const QSharedPointer<QFile> controlFile
//        = openFile(controlFilePath(project), QIODevice::ReadWrite, error);
//    if (!controlFile)
//        return false;
//    const QPixmap pixmap(iconFilePath);
//    if (pixmap.isNull()) {
//        *error = tr("Could not read image file '%1'.").arg(iconFilePath);
//        return false;
//    }

//    QByteArray iconAsBase64;
//    QBuffer buffer(&iconAsBase64);
//    buffer.open(QIODevice::WriteOnly);
//    if (!pixmap.scaled(48, 48).save(&buffer,
//        QFileInfo(iconFilePath).suffix().toAscii())) {
//        *error = tr("Could not export image file '%1'.").arg(iconFilePath);
//        return false;
//    }
//    buffer.close();
//    iconAsBase64 = iconAsBase64.toBase64();
//    QByteArray contents = controlFile->readAll();
//    const int iconFieldPos = contents.startsWith(IconFieldName)
//        ? 0 : contents.indexOf('\n' + IconFieldName);
//    if (iconFieldPos == -1) {
//        if (!contents.endsWith('\n'))
//            contents += '\n';
//        contents.append(IconFieldName).append(' ').append(iconAsBase64)
//            .append('\n');
//    } else {
//        const int oldIconStartPos
//            = (iconFieldPos != 0) + iconFieldPos + IconFieldName.length();
//        int nextEolPos = contents.indexOf('\n', oldIconStartPos);
//        while (nextEolPos != -1 && nextEolPos != contents.length() - 1
//            && contents.at(nextEolPos + 1) != '\n'
//            && (contents.at(nextEolPos + 1) == '#'
//                || std::isspace(contents.at(nextEolPos + 1))))
//            nextEolPos = contents.indexOf('\n', nextEolPos + 1);
//        if (nextEolPos == -1)
//            nextEolPos = contents.length();
//        contents.replace(oldIconStartPos, nextEolPos - oldIconStartPos,
//            ' ' + iconAsBase64);
//    }
//    controlFile->resize(0);
//    controlFile->write(contents);
//    if (controlFile->error() != QFile::NoError) {
//        *error = tr("Error writing file '%1': %2")
//            .arg(QDir::toNativeSeparators(controlFile->fileName()),
//                controlFile->errorString());
//        return false;
//    }
    return true;
}

void AndroidTemplatesManager::raiseError(const QString &reason)
{
    QMessageBox::critical(0, tr("Error creating Android templates"), reason);
}

void AndroidTemplatesManager::handleAndroidDirContentsChanged()
{
    Project * const project
        = findProject(qobject_cast<QFileSystemWatcher *>(sender()));
    if (project)
        emit androidDirContentsChanged(project);
}

QSharedPointer<QFile> AndroidTemplatesManager::openFile(const QString &filePath,
    QIODevice::OpenMode mode, QString *error) const
{
    const QString nativePath = QDir::toNativeSeparators(filePath);
    QSharedPointer<QFile> file(new QFile(filePath));
    if (!file->exists()) {
        *error = tr("File '%1' does not exist").arg(nativePath);
    } else if (!file->open(mode)) {
        *error = tr("Cannot open file '%1': %2")
            .arg(nativePath, file->errorString());
    }
    return file;
}

Project *AndroidTemplatesManager::findProject(const QFileSystemWatcher *fsWatcher) const
{
    for (AndroidProjectMap::ConstIterator it = m_androidProjects.constBegin();
        it != m_androidProjects.constEnd(); ++it) {
        if (it.value() == fsWatcher)
            return it.key();
    }
    return 0;
}

void AndroidTemplatesManager::findLine(const QByteArray &string,
    QByteArray &document, int &lineEndPos, int &valuePos)
{
    int lineStartPos = document.indexOf(string);
    if (lineStartPos == -1) {
        lineStartPos = document.length();
        document += string + '\n';
    }
    valuePos = lineStartPos + string.length();
    lineEndPos = document.indexOf('\n', lineStartPos);
    if (lineEndPos == -1) {
        lineEndPos = document.length();
        document += '\n';
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
