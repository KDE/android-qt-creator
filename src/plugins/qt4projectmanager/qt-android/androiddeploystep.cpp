/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androiddeploystep.h"

#include "androidconstants.h"
#include "androiddeploystepwidget.h"
#include "androiddeviceconfiglistmodel.h"
#include "androidglobal.h"
#include "androidpackagecreationstep.h"
#include "androidrunconfiguration.h"
#include "qt4androidtarget.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qt4project.h>
#include <qt4target.h>

#include <qt4projectmanager/qt4buildconfiguration.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtCore/QProcess>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(State, state, m_state)

using namespace Core;
using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {
namespace { const int DefaultMountPort = 1050; }

const QLatin1String AndroidDeployStep::Id("Qt4ProjectManager.AndroidDeployStep");

AndroidDeployStep::AndroidDeployStep(ProjectExplorer::BuildStepList *parent)
    : BuildStep(parent, Id)
{
    ctor();
}

AndroidDeployStep::AndroidDeployStep(ProjectExplorer::BuildStepList *parent,
    AndroidDeployStep *other)
    : BuildStep(parent, other)
{
    ctor();
}

AndroidDeployStep::~AndroidDeployStep() { }

void AndroidDeployStep::ctor()
{
    //: AndroidDeployStep default display name
    setDefaultDisplayName(tr("Deploy to Android device"));
    m_deployAction = NoDeploy;
    m_useLocalQtLibs = false;
}

bool AndroidDeployStep::init()
{
    return true;
}

void AndroidDeployStep::run(QFutureInterface<bool> &fi)
{
    fi.reportResult(deployPackage());
}

BuildStepConfigWidget *AndroidDeployStep::createConfigWidget()
{
    return new AndroidDeployStepWidget(this);
}

AndroidDeployStep::AndroidDeployAction AndroidDeployStep::deployAction()
{
    return m_deployAction;
}

bool AndroidDeployStep::useLocalQtLibs()
{
    return m_useLocalQtLibs;
}

void AndroidDeployStep::setDeployAction(AndroidDeployStep::AndroidDeployAction deploy)
{
    m_deployAction = deploy;
}

void AndroidDeployStep::setDeployQASIPackagePath(const QString & package)
{
    m_QASIPackagePath = package;
    m_deployAction = InstallQASI;
}

void AndroidDeployStep::setUseLocalQtLibs(bool useLocal)
{
    m_useLocalQtLibs=useLocal;
}

QVariantMap AndroidDeployStep::toMap() const
{
    QVariantMap map(BuildStep::toMap());
//    map.insert(AndroidDeployQtLibsKey, m_deployQtLibs);
//    map.insert(AndroidForceDeployKey, m_forceDeploy);
    return map;
}

bool AndroidDeployStep::fromMap(const QVariantMap &map)
{
    if (!BuildStep::fromMap(map))
        return false;
//    m_deployQtLibs = map.value(AndroidDeployQtLibsKey, m_deployQtLibs).toBool();
//    m_forceDeploy = map.value(AndroidForceDeployKey, m_forceDeploy).toBool();
    return true;
}

bool AndroidDeployStep::runCommand(QProcess *buildProc,
    const QString &command)
{

    writeOutput(tr("Package deploy: Running command '%1'.").arg(command), BuildStep::MessageOutput);
    buildProc->start(command);
    if (!buildProc->waitForStarted()) {
        writeOutput(tr("Packaging error: Could not start command '%1'. Reason: %2")
            .arg(command).arg(buildProc->errorString()), BuildStep::ErrorMessageOutput);
        return false;
    }
    buildProc->waitForFinished(-1);
    if (buildProc->error() != QProcess::UnknownError
        || buildProc->exitCode() != 0) {
        QString mainMessage = tr("Packaging Error: Command '%1' failed.")
            .arg(command);
        if (buildProc->error() != QProcess::UnknownError)
            mainMessage += tr(" Reason: %1").arg(buildProc->errorString());
        else
            mainMessage += tr("Exit code: %1").arg(buildProc->exitCode());
        writeOutput(mainMessage, BuildStep::ErrorMessageOutput);
        return false;
    }
    return true;
}

void AndroidDeployStep::handleBuildOutput()
{
    QProcess * const buildProc = qobject_cast<QProcess *>(sender());
    if (!buildProc)
        return;
    const QByteArray &stdOut = buildProc->readAllStandardOutput();
    if (!stdOut.isEmpty())
        emit addOutput(QString::fromLocal8Bit(stdOut), BuildStep::NormalOutput);
    const QByteArray &errorOut = buildProc->readAllStandardError();
    if (!errorOut.isEmpty()) {
        emit addOutput(QString::fromLocal8Bit(errorOut), BuildStep::ErrorOutput);
    }
}

QString AndroidDeployStep::deviceSerialNumber()
{
    return m_deviceSerialNumber;
}

void AndroidDeployStep::copyLibs(const QString &srcPath, const QString &destPath, QStringList & copiedLibs, const QStringList &filter)
{
    QDir dir;
    dir.mkpath(destPath);
    QDirIterator libsIt(srcPath, filter, QDir::NoFilter, QDirIterator::Subdirectories);
    int pos=srcPath.size();
    while(libsIt.hasNext())
    {
        libsIt.next();
        const QString destFile(destPath+libsIt.filePath().mid(pos));
        if (libsIt.fileInfo().isDir())
            dir.mkpath(destFile);
        else
        {
            QFile::copy(libsIt.filePath(), destFile);
            copiedLibs.append(destFile);
        }
    }
}

bool AndroidDeployStep::deployPackage()
{
    const Qt4BuildConfiguration * const bc
        = static_cast<Qt4BuildConfiguration *>(buildConfiguration());
    Qt4AndroidTarget * androidTarget = qobject_cast<Qt4AndroidTarget *>(target());
    if (!androidTarget)
    {
        raiseError(tr("Cannot deploy: current target is not android."));
        return false;
    }
    const QString packageName=androidTarget->packageName();
    const QString targetSDK=androidTarget->targetSDK();

    writeOutput(tr("Please wait, searching for a siutable device for target:%1.").arg(targetSDK));
    m_deviceSerialNumber=AndroidConfigurations::instance().getDeployDeviceSerialNumber(targetSDK.mid(targetSDK.indexOf('-')+1).toInt());
    if (!m_deviceSerialNumber.length())
    {
        m_deviceSerialNumber.clear();
        raiseError(tr("Cannot deploy: no devices or emulators found for your package."));
        return false;
    }

    QProcess proc;
    connect(&proc, SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleBuildOutput()));
    connect(&proc, SIGNAL(readyReadStandardError()), this,
        SLOT(handleBuildOutput()));

    if (m_deployAction == DeployLocal)
    {
        writeOutput(tr("Clean old qt libs"));
        runCommand(&proc, AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)
                   +" shell rm -r /data/local/qt");

        writeOutput(tr("Deploy qt libs ... this may take some time, please wait"));
        const QString tempPath=QDir::tempPath()+"/android_qt_libs_"+packageName;
        AndroidPackageCreationStep::removeDirectory(tempPath);
        QStringList stripFiles;
        copyLibs(bc->qtVersion()->sourcePath()+"/lib", tempPath+"/lib",stripFiles,QStringList()<<"*.so");
        copyLibs(bc->qtVersion()->sourcePath()+"/plugins", tempPath+"/plugins",stripFiles);
        copyLibs(bc->qtVersion()->sourcePath()+"/imports", tempPath+"/imports",stripFiles);
        AndroidPackageCreationStep::stripAndroidLibs(stripFiles);
        runCommand(&proc,AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)
                   +QString(" push %1 /data/local/qt").arg(tempPath));
        AndroidPackageCreationStep::removeDirectory(tempPath);
        emit (resetDelopyAction());
    }

    if (m_deployAction == InstallQASI)
    {
        if (!runCommand(&proc,AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)
                   +QString(" install -r ") +m_QASIPackagePath))
        {
            raiseError(tr("Qt Android smart installer instalation failed"));
            return false;
        }
        emit (resetDelopyAction());
    }
    proc.setWorkingDirectory(androidTarget->androidDirPath());

    writeOutput(tr("Installing package onto %1.").arg(m_deviceSerialNumber));
    runCommand(&proc,AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)
               +QLatin1String(" uninstall ")
               +packageName);

    if (!runCommand(&proc, AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)+QString(" install \"%1\"").arg(androidTarget->apkPath())))
    {
        raiseError(tr("Package instalation failed"));
        return false;
    }

    if (bc->qmakeBuildConfiguration() & QtVersion::DebugBuild)
    {
        writeOutput(tr("Pulling files necessary for debugging"));
        runCommand(&proc, AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)
                                           +QString(" pull /system/bin/app_process %1/app_process")
                                            .arg(bc->qt4Target()->qt4Project()->rootProjectNode()->buildDir()));
        runCommand(&proc, AndroidConfigurations::instance().adbToolPath(m_deviceSerialNumber)
                                           +QString(" pull /system/lib/libc.so %1/libc.so")
                                            .arg(bc->qt4Target()->qt4Project()->rootProjectNode()->buildDir()));
    }
    return true;
}

void AndroidDeployStep::raiseError(const QString &errorString)
{
    emit addTask(Task(Task::Error, errorString, QString(), -1,
        ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
}

void AndroidDeployStep::writeOutput(const QString &text, OutputFormat format)
{
    emit addOutput(text, format);
}

} // namespace Internal
} // namespace Qt4ProjectManager
