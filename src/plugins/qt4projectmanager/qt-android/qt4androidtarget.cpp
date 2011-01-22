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

#include "qt4androidtarget.h"
#include "qt4androiddeployconfiguration.h"
#include "androidconfigurations.h"
#include "androidrunconfiguration.h"
#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidpackagecreationstep.h"
#include "androidtoolchain.h"

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

Qt4AndroidTarget::Qt4AndroidTarget(Qt4Project *parent, const QString &id) :
    Qt4BaseTarget(parent, id)
  , m_androidFilesWatcher(new QFileSystemWatcher(this))
  , m_buildConfigurationFactory(new Qt4BuildConfigurationFactory(this))
  , m_deployConfigurationFactory(new Qt4AndroidDeployConfigurationFactory(this))
{
    setIcon(QIcon(":/projectexplorer/images/AndroidDevice.png"));
    connect(parent, SIGNAL(addedTarget(ProjectExplorer::Target*)),
        this, SLOT(handleTargetAdded(ProjectExplorer::Target*)));

}

Qt4AndroidTarget::~Qt4AndroidTarget()
{

}

Qt4BuildConfigurationFactory *Qt4AndroidTarget::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

ProjectExplorer::DeployConfigurationFactory *Qt4AndroidTarget::deployConfigurationFactory() const
{
    return m_deployConfigurationFactory;
}

QString Qt4AndroidTarget::defaultBuildDirectory() const
{
    //TODO why?
#if defined(Q_OS_WIN)
    return project()->projectDirectory();
#endif
    return Qt4BaseTarget::defaultBuildDirectory();
}

void Qt4AndroidTarget::createApplicationProFiles()
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

QList<ProjectExplorer::RunConfiguration *> Qt4AndroidTarget::runConfigurationsForNode(ProjectExplorer::Node *n)
{
    QList<ProjectExplorer::RunConfiguration *> result;
    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
        if (AndroidRunConfiguration *qt4c = qobject_cast<AndroidRunConfiguration *>(rc))
                if (qt4c->proFilePath() == n->path())
                    result << rc;
    return result;
}

QString Qt4AndroidTarget::defaultDisplayName()
{
    return QApplication::translate("Qt4ProjectManager::Qt4Target", "Android", "Qt4 Android target display name");
}

void Qt4AndroidTarget::handleTargetAdded(ProjectExplorer::Target *target)
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
    connect(m_androidFilesWatcher, SIGNAL(directoryChanged(QString)), this,
        SIGNAL(androidDirContentsChanged()));
    connect(m_androidFilesWatcher, SIGNAL(fileChanged(QString)), this,
        SIGNAL(androidDirContentsChanged()));
}

void Qt4AndroidTarget::handleTargetToBeRemoved(ProjectExplorer::Target *target)
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

QString Qt4AndroidTarget::androidDirPath()
{
    return project()->projectDirectory()+QLatin1Char('/')+AndroidDirName;
}

QString Qt4AndroidTarget::androidManifestPath()
{
    return androidDirPath()+QLatin1Char('/')+AndroidManifestName;
}

QString Qt4AndroidTarget::androidDefaultPropertiesPath()
{
    return androidDirPath()+QLatin1Char('/')+AndroidDefaultPropertiesName;
}


void Qt4AndroidTarget::updateProject(const QString &targetSDK)
{
    QString androidDir=androidDirPath();
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

bool Qt4AndroidTarget::createAndroidTemplatesIfNecessary(bool forceJava)
{
    const Qt4Project * qt4Project = qobject_cast<Qt4Project*>(project());
    if (!qt4Project)
        return false;
    QDir projectDir(project()->projectDirectory());
    QString androidPath=androidDirPath();

    if (!forceJava && QFileInfo(androidPath).exists()
            && QFileInfo(androidManifestPath()).exists()
            && QFileInfo(androidPath+QLatin1String("/src")).exists()
            && QFileInfo(androidPath+QLatin1String("/res")).exists())
        return true;

    if (!QFileInfo(androidDirPath()).exists())
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
    updateProject(AndroidConfigurations::instance().sdkTargets().at(0));
    return true;
}

bool Qt4AndroidTarget::openAndroidManifest(QDomDocument & doc)
{
    if (!createAndroidTemplatesIfNecessary())
        return false;

    QFile f(androidManifestPath());
    if (!f.open(QIODevice::ReadOnly))
    {
        raiseError(tr("Can't open AndroidManifest.xml file '%1'").arg(androidManifestPath()));
        return false;
    }
    if (!doc.setContent(f.readAll()))
    {
        raiseError(tr("Not enough SDK's found.\nPlease install at least one SDK."));
        return false;
    }
    return true;
}

bool Qt4AndroidTarget::saveAndroidManifest(QDomDocument & doc)
{
    if (!createAndroidTemplatesIfNecessary())
        return false;

    QFile f(androidManifestPath());
    if (!f.open(QIODevice::WriteOnly))
    {
        raiseError(tr("Can't open AndroidManifest.xml file '%1'").arg(androidManifestPath()));
        return false;
    }
    return f.write(doc.toByteArray(4))>=0;
}

QString Qt4AndroidTarget::activityName()
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return QString();
    QDomElement activityElem = doc.documentElement().firstChildElement("application").firstChildElement("activity");
    return activityElem.attribute(QLatin1String("android:name"));
}

QString Qt4AndroidTarget::intentName()
{
    return packageName()+QLatin1Char('/')+activityName();
}

QString Qt4AndroidTarget::packageName()
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return QString();
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("package"));
}

bool Qt4AndroidTarget::setPackageName(const QString & name)
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("package"), name);
    return saveAndroidManifest(doc);
}

QString Qt4AndroidTarget::applicationName()
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return QString();
    return doc.documentElement().firstChildElement("application").attribute(QLatin1String("android:label"));
}

bool Qt4AndroidTarget::setApplicationName(const QString & name)
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return false;
    doc.documentElement().firstChildElement("application").setAttribute(QLatin1String("android:label"), name);
    doc.documentElement().firstChildElement("application").firstChildElement("activity")
            .setAttribute(QLatin1String("android:label"), name);
    return saveAndroidManifest(doc);
}

QString Qt4AndroidTarget::targetApplication()
{
    QString app;
    return app;
}

bool Qt4AndroidTarget::setTargetApplication(const QString & name)
{
    return false;
}


int Qt4AndroidTarget::versionCode()
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return 0;
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("android:versionCode")).toInt();
}

bool Qt4AndroidTarget::setVersionCode(int version)
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("android:versionCode"), version);
    return saveAndroidManifest(doc);
}


QString Qt4AndroidTarget::versionName()
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return 0;
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("android:versionName"));
}

bool Qt4AndroidTarget::setVersionName(const QString &version)
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("android:versionName"), version);
    return saveAndroidManifest(doc);
}

QStringList Qt4AndroidTarget::permissions()
{
    QStringList per;
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return per;
    QDomElement permissionElem = doc.documentElement().firstChildElement("uses-permission");
    while(!permissionElem.isNull())
    {
        per<<permissionElem.attribute(QLatin1String("android:name"));
        permissionElem = permissionElem.nextSiblingElement("uses-permission");
    }
    return per;
}

bool Qt4AndroidTarget::setPermissions(const QStringList & permissions)
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return false;
    QDomElement docElement=doc.documentElement();
    QDomElement permissionElem = docElement.firstChildElement("uses-permission");
    while(!permissionElem.isNull())
    {
        docElement.removeChild(permissionElem);
        permissionElem = docElement.firstChildElement("uses-permission");
    }

    foreach(const QString & permission, permissions )
    {
        permissionElem = doc.createElement("uses-permission");
        permissionElem.setAttribute("android:name", permission);
        docElement.appendChild(permissionElem);
    }

    return saveAndroidManifest(doc);
}

QStringList Qt4AndroidTarget::availableQtLibs()
{
    QStringList libs;
    const Qt4Project * const qt4Project = qobject_cast<const Qt4Project *>(project());
    QString libsPath=qt4Project->activeTarget()->activeBuildConfiguration()->qtVersion()->sourcePath()+"/lib";
    QDirIterator libsIt(libsPath, QStringList()<<"libQt*.so");
    while(libsIt.hasNext())
    {
        libsIt.next();
        libs<<libsIt.fileName().mid(3,libsIt.fileName().indexOf('.')-3);
    }
    return libs;
}

QStringList Qt4AndroidTarget::qtLibs()
{
    QStringList libs;

    return libs;
}

bool Qt4AndroidTarget::setQtLibs(const QStringList & qtLibs)
{

    return true;
}

QStringList Qt4AndroidTarget::availablePrebundledLibs()
{
    QStringList libs;
    Qt4Project * qt4Project = qobject_cast<Qt4Project *>(project());
    QList<Qt4Project *> qt4Projects;
    qt4Projects<<qt4Project;
    foreach (ProjectExplorer::Project* pr, qt4Project->dependsOn())
    {
        qt4Project = qobject_cast<Qt4Project *>(pr);
        if (qt4Project)
            qt4Projects<<qt4Project;
    }

    foreach(Qt4Project * qt4Project, qt4Projects)
        foreach(Qt4ProFileNode * node, qt4Project->leafProFiles())
            if (node->projectType()== LibraryTemplate)
                libs<<QLatin1String("lib")+node->targetInformation().target+QLatin1String(".so");

    return libs;
}

QStringList Qt4AndroidTarget::prebundledLibs()
{
    QStringList libs;

    return libs;
}

bool Qt4AndroidTarget::setPrebundledLibs(const QStringList & qtLibs)
{

    return true;
}

QString Qt4AndroidTarget::targetSDK()
{
    if (!createAndroidTemplatesIfNecessary())
        return "android-4";
    QFile file(androidDefaultPropertiesPath());
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

bool Qt4AndroidTarget::setTargetSDK(const QString & target)
{
    updateProject(target);
    return true;
}

QIcon Qt4AndroidTarget::packageManagerIcon()
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

bool Qt4AndroidTarget::setPackageManagerIcon(const QString &iconFilePath)
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

void Qt4AndroidTarget::raiseError(const QString &reason)
{
    QMessageBox::critical(0, tr("Error creating Android templates"), reason);
}


} // namespace Internal
} // namespace Qt4ProjectManager
