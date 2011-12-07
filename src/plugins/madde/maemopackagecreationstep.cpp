/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "maemopackagecreationstep.h"

#include "maemoconstants.h"
#include "maemoglobal.h"
#include "maemopackagecreationwidget.h"
#include "qt4maemotarget.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <utils/environment.h>
#include <utils/fileutils.h>

#include <QtCore/QDateTime>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>

namespace {
    const QLatin1String MagicFileName(".qtcreator");
}

using namespace ProjectExplorer::Constants;
using ProjectExplorer::BuildStepList;
using ProjectExplorer::BuildStepConfigWidget;
using ProjectExplorer::Task;
using namespace Qt4ProjectManager;

namespace Madde {
namespace Internal {

const QLatin1String AbstractMaemoPackageCreationStep::DefaultVersionNumber("0.0.1");

AbstractMaemoPackageCreationStep::AbstractMaemoPackageCreationStep(BuildStepList *bsl,
    const QString &id) : AbstractPackagingStep(bsl, id)
{
}

AbstractMaemoPackageCreationStep::AbstractMaemoPackageCreationStep(BuildStepList *bsl,
    AbstractMaemoPackageCreationStep *other) : AbstractPackagingStep(bsl, other)
{
}

AbstractMaemoPackageCreationStep::~AbstractMaemoPackageCreationStep()
{
}

bool AbstractMaemoPackageCreationStep::init()
{
    if (!AbstractPackagingStep::init())
        return false;
    m_packagingNeeded = isPackagingNeeded();
    if (!isPackagingNeeded())
        return true;

    if (!qt4BuildConfiguration()) {
        raiseError(tr("No Qt4 build configuration"));
        return false;
    }

    m_environment = qt4BuildConfiguration()->environment();
    if (qt4BuildConfiguration()->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild) {
        m_environment.appendOrSet(QLatin1String("DEB_BUILD_OPTIONS"),
            QLatin1String("nostrip"), QLatin1String(" "));
    }

    if (!qt4BuildConfiguration()->qtVersion()) {
        raiseError(tr("Packaging failed: No Qt version."));
        return false;
    }

    m_qmakeCommand = qt4BuildConfiguration()->qtVersion()->qmakeCommand().toString();

    return true;
}

void AbstractMaemoPackageCreationStep::run(QFutureInterface<bool> &fi)
{
    if (!m_packagingNeeded) {
        emit addOutput(tr("Package up to date."), MessageOutput);
        fi.reportResult(true);
        return;
    }

    setPackagingStarted();
    // TODO: Make the build process asynchronous; i.e. no waitFor()-functions etc.
    QProcess * const buildProc = new QProcess;
    connect(buildProc, SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleBuildOutput()));
    connect(buildProc, SIGNAL(readyReadStandardError()), this,
        SLOT(handleBuildOutput()));
    emit addOutput(tr("Creating package file ..."), MessageOutput);
    const bool success = createPackage(buildProc, fi);
    disconnect(buildProc, 0, this, 0);
    buildProc->deleteLater();
    if (success)
        emit addOutput(tr("Package created."), BuildStep::MessageOutput);
    setPackagingFinished(success);
    fi.reportResult(success);
}

BuildStepConfigWidget *AbstractMaemoPackageCreationStep::createConfigWidget()
{
    return new MaemoPackageCreationWidget(this);
}

void AbstractMaemoPackageCreationStep::handleBuildOutput()
{
    QProcess * const buildProc = qobject_cast<QProcess *>(sender());
    if (!buildProc)
        return;
    QByteArray stdOut = buildProc->readAllStandardOutput();
    stdOut.replace('\0', QByteArray()); // Output contains NUL characters.
    if (!stdOut.isEmpty())
        emit addOutput(QString::fromLocal8Bit(stdOut), BuildStep::NormalOutput,
                BuildStep::DontAppendNewline);
    QByteArray errorOut = buildProc->readAllStandardError();
    errorOut.replace('\0', QByteArray());
    if (!errorOut.isEmpty()) {
        emit addOutput(QString::fromLocal8Bit(errorOut), BuildStep::ErrorOutput,
            BuildStep::DontAppendNewline);
    }
}

const Qt4BuildConfiguration *AbstractMaemoPackageCreationStep::qt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(target()->activeBuildConfiguration());
}

AbstractQt4MaemoTarget *AbstractMaemoPackageCreationStep::maemoTarget() const
{
    return qobject_cast<AbstractQt4MaemoTarget *>(target());
}

AbstractDebBasedQt4MaemoTarget *AbstractMaemoPackageCreationStep::debBasedMaemoTarget() const
{
    return qobject_cast<AbstractDebBasedQt4MaemoTarget*>(target());
}

AbstractRpmBasedQt4MaemoTarget *AbstractMaemoPackageCreationStep::rpmBasedMaemoTarget() const
{
    return qobject_cast<AbstractRpmBasedQt4MaemoTarget*>(target());
}

bool AbstractMaemoPackageCreationStep::isPackagingNeeded() const
{
    if (AbstractPackagingStep::isPackagingNeeded())
        return true;
    return isMetaDataNewerThan(QFileInfo(packageFilePath()).lastModified());
}

QString AbstractMaemoPackageCreationStep::packageFileName() const
{
    QString error;
    const QString &version = versionString(&error);
    if (version.isEmpty())
        return QString();
    QFileInfo fi(maemoTarget()->packageFileName());
    const QString baseName = replaceDots(fi.completeBaseName());
    return baseName + QLatin1Char('.') + fi.suffix();
}

QString AbstractMaemoPackageCreationStep::versionString(QString *error) const
{
    return maemoTarget()->projectVersion(error);
}

bool AbstractMaemoPackageCreationStep::setVersionString(const QString &version, QString *error)
{
    const bool success = maemoTarget()->setProjectVersion(version, error);
    if (success)
        emit packageFilePathChanged();
    return success;
}

QString AbstractMaemoPackageCreationStep::nativePath(const QFile &file)
{
    return QDir::toNativeSeparators(QFileInfo(file).filePath());
}

bool AbstractMaemoPackageCreationStep::callPackagingCommand(QProcess *proc,
    const QStringList &arguments)
{
    proc->setEnvironment(m_environment.toStringList());
    proc->setWorkingDirectory(cachedPackageDirectory());

    const QString madCommand = MaemoGlobal::madCommand(m_qmakeCommand);
    const QString cmdLine = madCommand + QLatin1Char(' ')
        + arguments.join(QLatin1String(" "));
    emit addOutput(tr("Package Creation: Running command '%1'.").arg(cmdLine),
        BuildStep::MessageOutput);
    MaemoGlobal::callMad(*proc, arguments, m_qmakeCommand, true);
    if (!proc->waitForStarted()) {
        raiseError(tr("Packaging failed: Could not start command '%1'. Reason: %2")
            .arg(cmdLine, proc->errorString()));
        return false;
    }
    proc->waitForFinished(-1);
    if (proc->error() != QProcess::UnknownError || proc->exitCode() != 0) {
        QString mainMessage = tr("Packaging Error: Command '%1' failed.")
            .arg(cmdLine);
        if (proc->error() != QProcess::UnknownError)
            mainMessage += tr(" Reason: %1").arg(proc->errorString());
        else
            mainMessage += tr("Exit code: %1").arg(proc->exitCode());
        raiseError(mainMessage);
        return false;
    }
    return true;
}

void AbstractMaemoPackageCreationStep::preparePackagingProcess(QProcess *proc,
    const Qt4BuildConfiguration *bc, const QString &workingDir)
{
    Utils::Environment env = bc->environment();
    if (bc->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild) {
        env.appendOrSet(QLatin1String("DEB_BUILD_OPTIONS"),
            QLatin1String("nostrip"), QLatin1String(" "));
    }
    proc->setEnvironment(env.toStringList());
    proc->setWorkingDirectory(workingDir);
}

QString AbstractMaemoPackageCreationStep::replaceDots(const QString &name) const
{
    // Idiotic OVI store requirement for N900 (but not allowed for N9 ...).
    if (qobject_cast<Qt4Maemo5Target *>(target())) {
        QString adaptedName = name;
        return adaptedName.replace(QLatin1Char('.'), QLatin1Char('_'));
    }
    return name;
}

////////////////
// MaemoDebianPackageCreationStep
////////////////

MaemoDebianPackageCreationStep::MaemoDebianPackageCreationStep(BuildStepList *bsl)
    : AbstractMaemoPackageCreationStep(bsl, CreatePackageId)
{
    ctor();
}

const QString MaemoDebianPackageCreationStep::CreatePackageId
    = QLatin1String("MaemoDebianPackageCreationStep");

MaemoDebianPackageCreationStep::MaemoDebianPackageCreationStep(BuildStepList *buildConfig,
    MaemoDebianPackageCreationStep *other)
        : AbstractMaemoPackageCreationStep(buildConfig, other)
{
    ctor();
}

void MaemoDebianPackageCreationStep::ctor()
{
    setDefaultDisplayName(tr("Create Debian Package"));
}

bool MaemoDebianPackageCreationStep::init()
{
    if (!AbstractMaemoPackageCreationStep::init())
        return false;
    m_maddeRoot = MaemoGlobal::maddeRoot(qt4BuildConfiguration()->qtVersion()->qmakeCommand().toString());
    m_projectDirectory = project()->projectDirectory();
    m_pkgFileName = maemoTarget()->packageFileName();
    m_packageName = maemoTarget()->packageName();
    m_templatesDirPath = debBasedMaemoTarget()->debianDirPath();
    m_debugBuild = qt4BuildConfiguration()->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild;
    checkProjectName();
    return true;
}

bool MaemoDebianPackageCreationStep::createPackage(QProcess *buildProc,
    const QFutureInterface<bool> &fi)
{
    Q_UNUSED(fi);
    const bool inSourceBuild = QFileInfo(cachedPackageDirectory()) == QFileInfo(m_projectDirectory);
    if (!copyDebianFiles(inSourceBuild))
        return false;
    const QStringList args = QStringList() << QLatin1String("dpkg-buildpackage")
        << QLatin1String("-nc") << QLatin1String("-uc") << QLatin1String("-us");
    if (!callPackagingCommand(buildProc, args))
        return false;

    QFile::remove(cachedPackageFilePath());

    // Workaround for non-working dh_builddeb --destdir=.
    if (!QDir(cachedPackageDirectory()).isRoot()) {
        QString error;
        if (!error.isEmpty())
            raiseError(tr("Packaging failed: Could not get package name."));
        const QString changesSourceFileName = QFileInfo(m_pkgFileName).completeBaseName()
                + QLatin1String(".changes");
        const QString changesTargetFileName = replaceDots(QFileInfo(m_pkgFileName).completeBaseName())
                + QLatin1String(".changes");
        const QString packageSourceDir = cachedPackageDirectory() + QLatin1String("/../");
        const QString packageSourceFilePath = packageSourceDir + m_pkgFileName;
        const QString changesSourceFilePath = packageSourceDir + changesSourceFileName;
        const QString changesTargetFilePath
            = cachedPackageDirectory() + QLatin1Char('/') + changesTargetFileName;
        QFile::remove(changesTargetFilePath);
        if (!QFile::rename(packageSourceFilePath, cachedPackageFilePath())
                || !QFile::rename(changesSourceFilePath, changesTargetFilePath)) {
            raiseError(tr("Packaging failed: Could not move package files from '%1' to '%2'.")
                .arg(packageSourceDir, cachedPackageDirectory()));
            return false;
        }
    }

    if (inSourceBuild)
        callPackagingCommand(buildProc, QStringList() << QLatin1String("dh_clean"));
    return true;
}

bool MaemoDebianPackageCreationStep::isMetaDataNewerThan(const QDateTime &packageDate) const
{
    const QString debianPath = debBasedMaemoTarget()->debianDirPath();
    if (packageDate <= QFileInfo(debianPath).lastModified())
        return true;
    const QStringList debianFiles = debBasedMaemoTarget()->debianFiles();
    foreach (const QString &debianFile, debianFiles) {
        const QString absFilePath = debianPath + QLatin1Char('/') + debianFile;
        if (packageDate <= QFileInfo(absFilePath).lastModified())
            return true;
    }
    return false;
}

void MaemoDebianPackageCreationStep::checkProjectName()
{
    const QRegExp legalName(QLatin1String("[0-9-+a-z\\.]+"));
    if (!legalName.exactMatch(project()->displayName())) {
        emit addTask(Task(Task::Warning,
            tr("Your project name contains characters not allowed in "
               "Debian packages.\nThey must only use lower-case letters, "
               "numbers, '-', '+' and '.'.\n""We will try to work around that, "
               "but you may experience problems."),
            QString(), -1, TASK_CATEGORY_BUILDSYSTEM));
    }
}

bool MaemoDebianPackageCreationStep::copyDebianFiles(bool inSourceBuild)
{
    const QString debianDirPath = cachedPackageDirectory() + QLatin1String("/debian");
    const QString magicFilePath
        = debianDirPath + QLatin1Char('/') + MagicFileName;
    if (inSourceBuild && QFileInfo(debianDirPath).isDir()
        && !QFileInfo(magicFilePath).exists()) {
        raiseError(tr("Packaging failed: Foreign debian directory detected. "
            "You are not using a shadow build and there is a debian "
            "directory in your project root ('%1'). Qt Creator will not "
            "overwrite that directory. Please remove it or use the "
            "shadow build feature.").arg(QDir::toNativeSeparators(debianDirPath)));
        return false;
    }
    QString error;
    if (!Utils::FileUtils::removeRecursively(debianDirPath, &error)) {
        raiseError(tr("Packaging failed: Could not remove directory '%1': %2")
            .arg(debianDirPath, error));
        return false;
    }
    QDir buildDir(cachedPackageDirectory());
    if (!buildDir.mkdir("debian")) {
        raiseError(tr("Could not create Debian directory '%1'.").arg(debianDirPath));
        return false;
    }
    QDir templatesDir(m_templatesDirPath);
    const QStringList &files = templatesDir.entryList(QDir::Files);
    foreach (const QString &fileName, files) {
        const QString srcFile = m_templatesDirPath + QLatin1Char('/') + fileName;
        QString newFileName = fileName;
        if (newFileName == Qt4HarmattanTarget::aegisManifestFileName())
            newFileName = m_packageName + QLatin1String(".aegis");

        const QString destFile = debianDirPath + QLatin1Char('/') + newFileName;
        if (fileName == QLatin1String("rules")) {
            if (!adaptRulesFile(srcFile, destFile))
                return false;
            continue;
        }

        if (newFileName == maemoTarget()->packageName() + QLatin1String(".aegis")) {
            Utils::FileReader reader;
            if (!reader.fetch(srcFile)) {
                raiseError(tr("Could not read manifest file '%1': %2.")
                    .arg(QDir::toNativeSeparators(srcFile), reader.errorString()));
                return false;
            }

            if (reader.data().isEmpty() || reader.data().startsWith("AutoGenerateAegisFile"))
                continue;
            if (reader.data().startsWith("NoAegisFile")) {
                QFile targetFile(destFile);
                if (!targetFile.open(QIODevice::WriteOnly)) {
                    raiseError(tr("Could not write manifest file '%1': %2.")
                        .arg(QDir::toNativeSeparators(destFile), targetFile.errorString()));
                    return false;
                }
                continue;
            }
        }

        if (!QFile::copy(srcFile, destFile)) {
            raiseError(tr("Could not copy file '%1' to '%2'.")
                .arg(QDir::toNativeSeparators(srcFile), QDir::toNativeSeparators(destFile)));
            return false;
        }
    }

    QFile magicFile(magicFilePath);
    if (!magicFile.open(QIODevice::WriteOnly)) {
        raiseError(tr("Error: Could not create file '%1'.")
            .arg(QDir::toNativeSeparators(magicFilePath)));
        return false;
    }

    return true;
}

QString MaemoDebianPackageCreationStep::packagingCommand(const QString &maddeRoot, const QString &commandName)
{
    QString perl;
#ifdef Q_OS_WIN
    perl = maddeRoot + QLatin1String("/bin/perl.exe ");
#endif
    return perl + maddeRoot + QLatin1String("/madbin/") + commandName;
}

void MaemoDebianPackageCreationStep::ensureShlibdeps(QByteArray &rulesContent)
{
    QString contentAsString = QString::fromLocal8Bit(rulesContent);
    const QString whiteSpace(QLatin1String("[ \\t]*"));
    const QString pattern = QLatin1String("\\n") + whiteSpace
        + QLatin1Char('#') + whiteSpace + QLatin1String("dh_shlibdeps")
        + QLatin1String("([^\\n]*)\\n");
    contentAsString.replace(QRegExp(pattern),
        QLatin1String("\n\tdh_shlibdeps\\1\n"));
    rulesContent = contentAsString.toLocal8Bit();
}

bool MaemoDebianPackageCreationStep::adaptRulesFile(
    const QString &templatePath, const QString &rulesFilePath)
{
    Utils::FileReader reader;
    if (!reader.fetch(templatePath)) {
        raiseError(reader.errorString());
        return false;
    }
    QByteArray content = reader.data();
    // Always check for dependencies in release builds.
    if (!m_debugBuild)
        ensureShlibdeps(content);

    Utils::FileSaver saver(rulesFilePath);
    saver.write(content);
    if (!saver.finalize()) {
        raiseError(saver.errorString());
        return false;
    }
    QFile rulesFile(rulesFilePath);
    rulesFile.setPermissions(rulesFile.permissions() | QFile::ExeUser);
    return true;
}

/////////////////
// MaemoRpmPackageCreationStep
/////////////////

MaemoRpmPackageCreationStep::MaemoRpmPackageCreationStep(BuildStepList *bsl)
    : AbstractMaemoPackageCreationStep(bsl, CreatePackageId)
{
    ctor();
}

MaemoRpmPackageCreationStep::MaemoRpmPackageCreationStep(BuildStepList *buildConfig,
    MaemoRpmPackageCreationStep *other)
        : AbstractMaemoPackageCreationStep(buildConfig, other)
{
    ctor();
}

void MaemoRpmPackageCreationStep::ctor()
{
    setDefaultDisplayName(tr("Create RPM Package"));
}

bool MaemoRpmPackageCreationStep::init()
{
    m_specFile = rpmBasedMaemoTarget()->specFilePath();
    m_packageFileName = rpmBasedMaemoTarget()->packageFileName();
    return AbstractMaemoPackageCreationStep::init();
}

bool MaemoRpmPackageCreationStep::createPackage(QProcess *buildProc,
    const QFutureInterface<bool> &fi)
{
    Q_UNUSED(fi);
    const QStringList args = QStringList() << QLatin1String("rrpmbuild")
        << QLatin1String("-bb") << m_specFile;
    if (!callPackagingCommand(buildProc, args))
        return false;
    QFile::remove(cachedPackageFilePath());
    const QString packageSourceFilePath = rpmBuildDir() + QLatin1Char('/')
        + m_packageFileName;
    if (!QFile::rename(packageSourceFilePath, cachedPackageFilePath())) {
        raiseError(tr("Packaging failed: Could not move package file from %1 to %2.")
            .arg(packageSourceFilePath, cachedPackageFilePath()));
        return false;
    }

    return true;
}

bool MaemoRpmPackageCreationStep::isMetaDataNewerThan(const QDateTime &packageDate) const
{
    const QDateTime specFileChangeDate
        = QFileInfo(rpmBasedMaemoTarget()->specFilePath()).lastModified();
    return packageDate <= specFileChangeDate;
}

QString MaemoRpmPackageCreationStep::rpmBuildDir() const
{
    return cachedPackageDirectory() + QLatin1String("/rrpmbuild");
}

const QString MaemoRpmPackageCreationStep::CreatePackageId
    = QLatin1String("MaemoRpmPackageCreationStep");

} // namespace Internal
} // namespace Madde
