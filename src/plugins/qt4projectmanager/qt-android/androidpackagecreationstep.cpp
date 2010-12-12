/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "androidpackagecreationstep.h"

#include "androidconstants.h"
#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidpackagecreationwidget.h"
#include "androidtemplatesmanager.h"
#include "androidtoolchain.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qt4buildconfiguration.h>
#include <qt4project.h>
#include <qt4target.h>
#include <utils/environment.h>

#include <QtCore/QDateTime>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QStringBuilder>
#include <QtGui/QWidget>

namespace {
    const QLatin1String PackagingEnabledKey("Packaging Enabled");
    const QLatin1String MagicFileName(".qtcreator");
}

using namespace ProjectExplorer::Constants;
using ProjectExplorer::BuildStepList;
using ProjectExplorer::BuildStepConfigWidget;
using ProjectExplorer::Task;

namespace Qt4ProjectManager {
namespace Internal {

const QLatin1String AndroidPackageCreationStep::DefaultVersionNumber("0.0.1");

AndroidPackageCreationStep::AndroidPackageCreationStep(BuildStepList *bsl)
    : ProjectExplorer::BuildStep(bsl, CreatePackageId),
      m_packagingEnabled(true)
{
    ctor();
}

AndroidPackageCreationStep::AndroidPackageCreationStep(BuildStepList *bsl,
    AndroidPackageCreationStep *other)
    : BuildStep(bsl, other),
      m_packagingEnabled(other->m_packagingEnabled)
{
    ctor();
}

AndroidPackageCreationStep::~AndroidPackageCreationStep()
{
}

void AndroidPackageCreationStep::ctor()
{
    setDefaultDisplayName(tr("Packaging for Android"));

    m_lastBuildConfig = qt4BuildConfiguration();
    connect(target(),
        SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
        this, SLOT(handleBuildConfigChanged()));
    handleBuildConfigChanged();
}

bool AndroidPackageCreationStep::init()
{
    return true;
}

QVariantMap AndroidPackageCreationStep::toMap() const
{
    QVariantMap map(ProjectExplorer::BuildStep::toMap());
    map.insert(PackagingEnabledKey, m_packagingEnabled);
    return map;
}

bool AndroidPackageCreationStep::fromMap(const QVariantMap &map)
{
    m_packagingEnabled = map.value(PackagingEnabledKey, true).toBool();
    return ProjectExplorer::BuildStep::fromMap(map);
}

void AndroidPackageCreationStep::run(QFutureInterface<bool> &fi)
{

    bool success=true;
//    if (m_packagingEnabled) {
//        // TODO: Make the build process asynchronous; i.e. no waitFor()-functions etc.
//        QProcess * const buildProc = new QProcess;
//        connect(buildProc, SIGNAL(readyReadStandardOutput()), this,
//            SLOT(handleBuildOutput()));
//        connect(buildProc, SIGNAL(readyReadStandardError()), this,
//            SLOT(handleBuildOutput()));
//        success = createPackage(buildProc);
//        disconnect(buildProc, 0, this, 0);
//        buildProc->deleteLater();
//    }

    fi.reportResult(success);
}

BuildStepConfigWidget *AndroidPackageCreationStep::createConfigWidget()
{
    return new AndroidPackageCreationWidget(this);
}

bool AndroidPackageCreationStep::createPackage(QProcess *buildProc)
{
//    if (!packagingNeeded()) {
//        emit addOutput(tr("Package up to date."), MessageOutput);
//        return true;
//    }

//    emit addOutput(tr("Creating package file ..."), MessageOutput);
//    checkProjectName();
//    QString error;
//    if (!preparePackagingProcess(buildProc, qt4BuildConfiguration(),
//       buildDirectory(), &error)) {
//        raiseError(error);
//        return false;
//    }

//    const QString projectDir
//        = buildConfiguration()->target()->project()->projectDirectory();
//    const bool inSourceBuild
//        = QFileInfo(buildDirectory()) == QFileInfo(projectDir);
//    if (!copyDebianFiles(inSourceBuild))
//        return false;

//    if (!runCommand(buildProc, QLatin1String("dpkg-buildpackage -nc -uc -us")))
//        return false;

//    // Workaround for non-working dh_builddeb --destdir=.
//    if (!QDir(buildDirectory()).isRoot()) {
//        const ProjectExplorer::Project * const project
//            = buildConfiguration()->target()->project();
//        QString error;
//        const QString pkgFileName = packageFileName(project,
//            AndroidTemplatesManager::instance()->version(project, &error));
//        if (!error.isEmpty())
//            raiseError(tr("Packaging failed."), error);
//        const QString changesFileName = QFileInfo(pkgFileName)
//            .completeBaseName() + QLatin1String(".changes");
//        const QString packageSourceDir = buildDirectory() + QLatin1String("/../");
//        const QString packageSourceFilePath
//            = packageSourceDir + pkgFileName;
//        const QString changesSourceFilePath
//            = packageSourceDir + changesFileName;
//        const QString changesTargetFilePath
//            = buildDirectory() + QLatin1Char('/') + changesFileName;
//        QFile::remove(packageFilePath());
//        QFile::remove(changesTargetFilePath);
//        if (!QFile::rename(packageSourceFilePath, packageFilePath())
//            || !QFile::rename(changesSourceFilePath, changesTargetFilePath)) {
//            raiseError(tr("Packaging failed."),
//                tr("Could not move package files from %1 to %2.")
//                .arg(packageSourceDir, buildDirectory()));
//            return false;
//        }
//    }

//    emit addOutput(tr("Package created."), BuildStep::MessageOutput);
//    if (inSourceBuild) {
//        buildProc->start(packagingCommand(androidToolChain(),
//            QLatin1String("dh_clean")));
//        buildProc->waitForFinished();
//        buildProc->terminate();
//    }
    return true;
}

bool AndroidPackageCreationStep::copyDebianFiles(bool inSourceBuild)
{
#warning FIXME Android

//    const QString debianDirPath = buildDirectory() + QLatin1String("/debian");
//    const QString magicFilePath
//        = debianDirPath + QLatin1Char('/') + MagicFileName;
//    if (inSourceBuild && QFileInfo(debianDirPath).isDir()
//        && !QFileInfo(magicFilePath).exists()) {
//        raiseError(tr("Packaging failed: Foreign debian directory detected."),
//             tr("You are not using a shadow build and there is a debian "
//                "directory in your project root ('%1'). Qt Creator will not "
//                "overwrite that directory. Please remove it or use the "
//                "shadow build feature.")
//                   .arg(QDir::toNativeSeparators(debianDirPath)));
//        return false;
//    }
//    if (!removeDirectory(debianDirPath)) {
//        raiseError(tr("Packaging failed."),
//            tr("Could not remove directory '%1'.").arg(debianDirPath));
//        return false;
//    }
//    QDir buildDir(buildDirectory());
//    if (!buildDir.mkdir("debian")) {
//        raiseError(tr("Could not create Debian directory '%1'.")
//                   .arg(debianDirPath));
//        return false;
//    }
//    const QString templatesDirPath = AndroidTemplatesManager::instance()
//        ->debianDirPath(buildConfiguration()->target()->project());
//    QDir templatesDir(templatesDirPath);
//    const QStringList &files = templatesDir.entryList(QDir::Files);
//    const bool harmattanWorkaroundNeeded
//        = androidToolChain()->version() == AndroidToolChain::android_8
//            && !qt4BuildConfiguration()->qt4Target()->qt4Project()
//                   ->applicationProFiles().isEmpty();
//    foreach (const QString &fileName, files) {
//        const QString srcFile
//                = templatesDirPath + QLatin1Char('/') + fileName;
//        const QString destFile
//                = debianDirPath + QLatin1Char('/') + fileName;
//        if (!QFile::copy(srcFile, destFile)) {
//            raiseError(tr("Could not copy file '%1' to '%2'")
//                       .arg(QDir::toNativeSeparators(srcFile),
//                            QDir::toNativeSeparators(destFile)));
//            return false;
//        }

//        // Workaround for Harmattan icon bug
//        if (harmattanWorkaroundNeeded && fileName == QLatin1String("rules"))
//            addWorkaroundForHarmattanBug(destFile);
//    }

//    QFile magicFile(magicFilePath);
//    if (!magicFile.open(QIODevice::WriteOnly)) {
//        raiseError(tr("Error: Could not create file '%1'.")
//            .arg(QDir::toNativeSeparators(magicFilePath)));
//        return false;
//    }

    return true;
}

bool AndroidPackageCreationStep::removeDirectory(const QString &dirPath)
{
//    QDir dir(dirPath);
//    if (!dir.exists())
//        return true;

//    const QStringList &files
//        = dir.entryList(QDir::Files | QDir::Hidden | QDir::System);
//    foreach (const QString &fileName, files) {
//        if (!dir.remove(fileName))
//            return false;
//    }

//    const QStringList &subDirs
//        = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
//    foreach (const QString &subDirName, subDirs) {
//        if (!removeDirectory(dirPath + QLatin1Char('/') + subDirName))
//            return false;
//    }

//    return dir.rmdir(dirPath);
}

bool AndroidPackageCreationStep::runCommand(QProcess *buildProc,
    const QString &command)
{
//    emit addOutput(tr("Package Creation: Running command '%1'.").arg(command), BuildStep::MessageOutput);
//    buildProc->start(packagingCommand(androidToolChain(), command));
//    if (!buildProc->waitForStarted()) {
//        raiseError(tr("Packaging failed."),
//            tr("Packaging error: Could not start command '%1'. Reason: %2")
//            .arg(command).arg(buildProc->errorString()));
//        return false;
//    }
//    buildProc->waitForFinished(-1);
//    if (buildProc->error() != QProcess::UnknownError
//        || buildProc->exitCode() != 0) {
//        QString mainMessage = tr("Packaging Error: Command '%1' failed.")
//            .arg(command);
//        if (buildProc->error() != QProcess::UnknownError)
//            mainMessage += tr(" Reason: %1").arg(buildProc->errorString());
//        else
//            mainMessage += tr("Exit code: %1").arg(buildProc->exitCode());
//        raiseError(mainMessage);
//        return false;
//    }
    return true;
}

void AndroidPackageCreationStep::handleBuildOutput()
{
//    QProcess * const buildProc = qobject_cast<QProcess *>(sender());
//    if (!buildProc)
//        return;
//    const QByteArray &stdOut = buildProc->readAllStandardOutput();
//    if (!stdOut.isEmpty())
//        emit addOutput(QString::fromLocal8Bit(stdOut), BuildStep::NormalOutput);
//    const QByteArray &errorOut = buildProc->readAllStandardError();
//    if (!errorOut.isEmpty()) {
//        emit addOutput(QString::fromLocal8Bit(errorOut), BuildStep::ErrorOutput);
//    }
}

void AndroidPackageCreationStep::handleBuildConfigChanged()
{
//    if (m_lastBuildConfig)
//        disconnect(m_lastBuildConfig, 0, this, 0);
//    m_lastBuildConfig = qt4BuildConfiguration();
//    connect(m_lastBuildConfig, SIGNAL(qtVersionChanged()), this,
//        SIGNAL(qtVersionChanged()));
//    connect(m_lastBuildConfig, SIGNAL(buildDirectoryChanged()), this,
//        SIGNAL(packageFilePathChanged()));
//    emit qtVersionChanged();
//    emit packageFilePathChanged();
}

const Qt4BuildConfiguration *AndroidPackageCreationStep::qt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(buildConfiguration());
}

QString AndroidPackageCreationStep::buildDirectory() const
{
    return qt4BuildConfiguration()->buildDirectory();
}

QString AndroidPackageCreationStep::projectName() const
{
    return qt4BuildConfiguration()->qt4Target()->qt4Project()
        ->rootProjectNode()->displayName().toLower();
}

const AndroidToolChain *AndroidPackageCreationStep::androidToolChain() const
{
    return static_cast<AndroidToolChain *>(qt4BuildConfiguration()->toolChain());
}

AndroidDeployStep *AndroidPackageCreationStep::deployStep() const
{
    AndroidDeployStep * const deployStep
        = AndroidGlobal::buildStep<AndroidDeployStep>(target()->activeDeployConfiguration());
    Q_ASSERT(deployStep &&
        "Fatal error: Android build configuration without deploy step.");
    return deployStep;
}


bool AndroidPackageCreationStep::packagingNeeded() const
{
//    const QSharedPointer<AndroidDeployables> &deployables
//        = deployStep()->deployables();
//    QFileInfo packageInfo(packageFilePath());
//    if (!packageInfo.exists() || deployables->isModified())
//        return true;

//    const int deployableCount = deployables->deployableCount();
//    for (int i = 0; i < deployableCount; ++i) {
//        if (isFileNewerThan(deployables->deployableAt(i).localFilePath,
//                packageInfo.lastModified()))
//            return true;
//    }

//    const ProjectExplorer::Project * const project = target()->project();
//    const AndroidTemplatesManager * const templatesManager
//        = AndroidTemplatesManager::instance();
//    const QString debianPath = templatesManager->debianDirPath(project);
//    if (packageInfo.lastModified() <= QFileInfo(debianPath).lastModified())
//        return true;
//    const QStringList debianFiles = templatesManager->debianFiles(project);
//    foreach (const QString &debianFile, debianFiles) {
//        const QString absFilePath = debianPath + QLatin1Char('/') + debianFile;
//        if (packageInfo.lastModified() <= QFileInfo(absFilePath).lastModified())
//            return true;
//    }

    return false;
}

bool AndroidPackageCreationStep::isFileNewerThan(const QString &filePath,
    const QDateTime &timeStamp) const
{
//    QFileInfo fileInfo(filePath);
//    if (!fileInfo.exists() || fileInfo.lastModified() >= timeStamp)
//        return true;
//    if (fileInfo.isDir()) {
//        const QStringList dirContents = QDir(filePath)
//            .entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
//        foreach (const QString &curFileName, dirContents) {
//            const QString curFilePath
//                = filePath + QLatin1Char('/') + curFileName;
//            if (isFileNewerThan(curFilePath, timeStamp))
//                return true;
//        }
//    }
    return false;
}

QString AndroidPackageCreationStep::packageFilePath() const
{
    QString error;
    const QString &version = versionString(&error);
    if (version.isEmpty())
        return QString();
    return buildDirectory() % '/'
        % packageFileName(buildConfiguration()->target()->project(), version);
}

bool AndroidPackageCreationStep::isPackagingEnabled() const
{
    return m_packagingEnabled;
}

QString AndroidPackageCreationStep::versionString(QString *error) const
{
    return AndroidTemplatesManager::instance()
        ->version(buildConfiguration()->target()->project(), error);

}

bool AndroidPackageCreationStep::setVersionString(const QString &version,
    QString *error)
{
    const bool success = AndroidTemplatesManager::instance()
        ->setVersion(buildConfiguration()->target()->project(), version, error);
    if (success)
        emit packageFilePathChanged();
    return success;
}

QString AndroidPackageCreationStep::nativePath(const QFile &file)
{
    return QDir::toNativeSeparators(QFileInfo(file).filePath());
}

void AndroidPackageCreationStep::raiseError(const QString &shortMsg,
                                          const QString &detailedMsg)
{
    emit addOutput(detailedMsg.isNull() ? shortMsg : detailedMsg, BuildStep::ErrorOutput);
    emit addTask(Task(Task::Error, shortMsg, QString(), -1,
                      TASK_CATEGORY_BUILDSYSTEM));
}

bool AndroidPackageCreationStep::preparePackagingProcess(QProcess *proc,
    const Qt4BuildConfiguration *bc, const QString &workingDir, QString *error)
{
#warning FIXME ANDROID
    return true;

//    const AndroidToolChain * const tc
//        = dynamic_cast<const AndroidToolChain *>(bc->toolChain());
//    if (!tc) {
//        *error = tr("Build configuration has no Android toolchain.");
//        return false;
//    }
//    QFile configFile(tc->targetRoot() % QLatin1String("/config.sh"));
//    if (!configFile.open(QIODevice::ReadOnly)) {
//        *error = tr("Cannot open MADDE config file '%1'.")
//            .arg(nativePath(configFile));
//        return false;
//    }

//    Utils::Environment env = bc->environment();
//    const QString &path
//        = QDir::toNativeSeparators(tc->maddeRoot() + QLatin1Char('/'));
//#ifdef Q_OS_WIN
//    env.prependOrSetPath(path % QLatin1String("bin"));
//#endif
//    env.prependOrSetPath(tc->targetRoot() % QLatin1String("/bin"));
//    env.prependOrSetPath(path % QLatin1String("madbin"));

//    if (bc->qmakeBuildConfiguration() & QtVersion::DebugBuild) {
//        env.appendOrSet(QLatin1String("DEB_BUILD_OPTIONS"),
//            QLatin1String("nostrip"), QLatin1String(" "));
//    }

//    QString perlLib
//        = QDir::fromNativeSeparators(path % QLatin1String("madlib/perl5"));
//#ifdef Q_OS_WIN
//    perlLib = perlLib.remove(QLatin1Char(':'));
//    perlLib = perlLib.prepend(QLatin1Char('/'));
//#endif
//    env.set(QLatin1String("PERL5LIB"), perlLib);
//    env.set(QLatin1String("PWD"), workingDir);

//    const QRegExp envPattern(QLatin1String("([^=]+)=[\"']?([^;\"']+)[\"']? ;.*"));
//    QByteArray line;
//    do {
//        line = configFile.readLine(200);
//        if (envPattern.exactMatch(line))
//            env.set(envPattern.cap(1), envPattern.cap(2));
//    } while (!line.isEmpty());

//    proc->setEnvironment(env.toStringList());
//    proc->setWorkingDirectory(workingDir);
//    return true;
}

QString AndroidPackageCreationStep::packagingCommand(const AndroidToolChain *tc,
    const QString &commandName)
{
//    QString perl;
//#ifdef Q_OS_WIN
//    perl = tc->maddeRoot() + QLatin1String("/bin/perl.exe ");
//#endif
//    return perl + tc->maddeRoot() % QLatin1String("/madbin/") % commandName;
}

void AndroidPackageCreationStep::checkProjectName()
{
//    const QRegExp legalName(QLatin1String("[0-9-+a-z\\.]+"));
//    if (!legalName.exactMatch(buildConfiguration()->target()->project()->displayName())) {
//        emit addTask(Task(Task::Warning,
//            tr("Your project name contains characters not allowed in Debian packages.\n"
//               "They must only use lower-case letters, numbers, '-', '+' and '.'.\n"
//               "We will try to work around that, but you may experience problems."),
//               QString(), -1, TASK_CATEGORY_BUILDSYSTEM));
//    }
}

QString AndroidPackageCreationStep::packageName(const ProjectExplorer::Project *project)
{
    QString packageName = project->displayName().toLower();
    const QRegExp legalLetter(QLatin1String("[a-z0-9+-.]"), Qt::CaseSensitive,
        QRegExp::WildcardUnix);
    for (int i = 0; i < packageName.length(); ++i) {
        if (!legalLetter.exactMatch(packageName.mid(i, 1)))
            packageName[i] = QLatin1Char('-');
    }
    return packageName;
}

QString AndroidPackageCreationStep::packageFileName(const ProjectExplorer::Project *project,
    const QString &version)
{
    return packageName(project) % QLatin1Char('_') % version
        % QLatin1String("_armel.deb");
}

const QLatin1String AndroidPackageCreationStep::CreatePackageId("Qt4ProjectManager.AndroidPackageCreationStep");

} // namespace Internal
} // namespace Qt4ProjectManager
