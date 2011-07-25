/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidpackagecreationstep.h"

#include "androidconstants.h"
#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidpackagecreationwidget.h"
#include "androidtarget.h"
#include "qt4projectmanager/qt4nodes.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/foldernavigationwidget.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>
#include <utils/environment.h>

#include <QtCore/QAbstractListModel>
#include <QtCore/QDateTime>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QStringBuilder>
#include <QtCore/QVector>
#include <QtCore/QPair>
#include <QtGui/QWidget>
#include <QtGui/QMessageBox>
#include <QtGui/QInputDialog>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;
using ProjectExplorer::BuildStepList;
using ProjectExplorer::BuildStepConfigWidget;
using ProjectExplorer::Task;

namespace Android {
namespace Internal {

namespace {
    const QLatin1String KeystoreLocationKey("KeystoreLocation");
    const char * const AliasString="Alias name:";
}


using namespace Qt4ProjectManager;


class CertificatesModel: public QAbstractListModel
{
public:
    CertificatesModel(const QString & rowCertificates, QObject * parent):QAbstractListModel(parent)
    {
        int from=rowCertificates.indexOf(AliasString);
        QPair<QString, QString> item;
        while(from>-1)
        {
            from+=11;// strlen(AliasString);
            const int eol=rowCertificates.indexOf("\n", from);
            item.first=rowCertificates.mid(from, eol-from).trimmed();
            const int eoc=rowCertificates.indexOf("*******************************************", eol);
            item.second=rowCertificates.mid(eol+1,eoc-eol-2).trimmed();
            from=rowCertificates.indexOf(AliasString, eoc);
            m_certs.push_back(item);
        }
    }

protected:
    int rowCount ( const QModelIndex & parent = QModelIndex() ) const
    {
        Q_UNUSED(parent)
        return m_certs.size();
    }
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const
    {
        if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::ToolTipRole) )
            return QVariant();
        if (role == Qt::DisplayRole)
            return m_certs[index.row()].first;
        return m_certs[index.row()].second;
    }
private:
    QVector<QPair<QString,QString> > m_certs;
};


AndroidPackageCreationStep::AndroidPackageCreationStep(BuildStepList *bsl)
    : BuildStep(bsl, CreatePackageId)
{
    ctor();
}

AndroidPackageCreationStep::AndroidPackageCreationStep(BuildStepList *bsl,
    AndroidPackageCreationStep *other)
    : BuildStep(bsl, other)
{
    ctor();
}

AndroidPackageCreationStep::~AndroidPackageCreationStep()
{
}

void AndroidPackageCreationStep::ctor()
{
    setDefaultDisplayName(tr("Packaging for Android"));
    m_openPackageLocation = true;
}

bool AndroidPackageCreationStep::init()
{
    return true;
}

void AndroidPackageCreationStep::run(QFutureInterface<bool> &fi)
{
    fi.reportResult(createPackage());
}

BuildStepConfigWidget *AndroidPackageCreationStep::createConfigWidget()
{
    return new AndroidPackageCreationWidget(this);
}

AndroidTarget *AndroidPackageCreationStep::androidTarget() const
{
    return qobject_cast<AndroidTarget *>(buildConfiguration()->target());
}

void AndroidPackageCreationStep::checkRequiredLibraries()
{
    QProcess readelfProc;
    QString appPath=androidTarget()->targetApplicationPath();
    if (!QFile::exists(appPath))
    {
        QMessageBox::critical(0, tr("Can't find read elf information"),
                              tr("Can't find '%1'.\n"
                                 "Please make sure your appication "
                                 " built successfully and is selected in Appplication tab ('Run option') ").arg(appPath) );
        return;
    }
    readelfProc.start(AndroidConfigurations::instance().readelfPath(androidTarget()->activeRunConfiguration()->abi().architecture()),
                      QStringList()<<"-d"<<"-W"<<appPath);
    if (!readelfProc.waitForFinished(-1))
    {
        readelfProc.terminate();
        return;
    }
    QStringList libs;
    QList<QByteArray> lines=readelfProc.readAll().trimmed().split('\n');
    foreach(QByteArray line, lines)
    {
        if (line.contains("(NEEDED)") && line.contains("Shared library:") )
        {
            const int pos=line.lastIndexOf('[')+1;
            libs<<line.mid(pos,line.length()-pos-1);
        }
    }
    QStringList checkedLibs = androidTarget()->qtLibs();
    QStringList requiredLibraries;
    foreach(const QString & qtLib, androidTarget()->availableQtLibs())
    {
        if (libs.contains("lib"+qtLib+".so") || checkedLibs.contains(qtLib))
            requiredLibraries<<qtLib;
    }
    androidTarget()->setQtLibs(requiredLibraries);

    checkedLibs = androidTarget()->prebundledLibs();
    requiredLibraries.clear();
    foreach(const QString & qtLib, androidTarget()->availableQtLibs())
    {
        if (libs.contains(qtLib) || checkedLibs.contains(qtLib))
            requiredLibraries<<qtLib;
    }
    androidTarget()->setPrebundledLibs(requiredLibraries);
    emit updateRequiredLibrariesModels();
}

QString AndroidPackageCreationStep::keystorePath()
{
    return m_keystorePath;
}

void AndroidPackageCreationStep::setKeystorePath(const QString & path)
{
    m_keystorePath=path;
    m_certificatePasswd.clear();
    m_keystorePasswd.clear();
}

void AndroidPackageCreationStep::setKeystorePassword(const QString & pwd)
{
    m_keystorePasswd=pwd;
}

void AndroidPackageCreationStep::setCertificateAlias(const QString & alias)
{
    m_certificateAlias=alias;
}

void AndroidPackageCreationStep::setCertificatePassword(const QString & pwd)
{
    m_certificatePasswd=pwd;
}

void AndroidPackageCreationStep::setOpenPackageLocation(bool open)
{
    m_openPackageLocation = open;
}

QAbstractItemModel * AndroidPackageCreationStep::keystoreCertificates()
{
    QString rawCerts;
    QProcess keytoolProc;
    while(!rawCerts.length() || !m_keystorePasswd.length())
    {
        QStringList params;
        params<<"-list"<<"-v"<<"-keystore"<<m_keystorePath<<"-storepass";
        if (!m_keystorePasswd.length())
            keystorePassword();
        if (!m_keystorePasswd.length())
            return 0;
        params<<m_keystorePasswd;
        keytoolProc.start(AndroidConfigurations::instance().keytoolPath(), params);
        if (!keytoolProc.waitForStarted() || !keytoolProc.waitForFinished())
        {
            QMessageBox::critical(0, tr("Error"),
                                  tr("Failed to run keytool"));
            return 0;
        }

        if (keytoolProc.exitCode())
        {
            QMessageBox::critical(0, tr("Error"),
                                  tr("Invalid password"));
            m_keystorePasswd.clear();
        }
        rawCerts=keytoolProc.readAllStandardOutput();
    }
    return new CertificatesModel(rawCerts, this);
}

bool AndroidPackageCreationStep::fromMap(const QVariantMap &map)
{
    if (!BuildStep::fromMap(map))
        return false;
    m_keystorePath= map.value(KeystoreLocationKey).toString();
    return true;
}

QVariantMap AndroidPackageCreationStep::toMap() const
{
    QVariantMap map(BuildStep::toMap());
    map.insert(KeystoreLocationKey, m_keystorePath);
    return map;
}

bool AndroidPackageCreationStep::createPackage()
{
    const Qt4BuildConfiguration * bc=static_cast<Qt4BuildConfiguration *>(buildConfiguration());
    AndroidTarget * target=androidTarget();
    checkRequiredLibraries();
    emit addOutput(tr("Copy Qt app & libs to Android package ..."), MessageOutput);

    const QString androidDir(target->androidDirPath());

    QString androidLibPath;
    if (bc->qt4Target()->qt4Project()->rootProjectNode()
            ->variableValue(Qt4ProjectManager::ConfigVar).contains("x86"))
        androidLibPath=androidDir+QLatin1String("/libs/x86");
    else
        if (bc->qt4Target()->qt4Project()->rootProjectNode()
            ->variableValue(Qt4ProjectManager::ConfigVar).contains("armeabi-v7a"))
        androidLibPath=androidDir+QLatin1String("/libs/armeabi-v7a");
        else
            androidLibPath=androidDir+QLatin1String("/libs/armeabi");
    QStringList build;
    build<<"clean";
    QFile::remove(androidLibPath+QLatin1String("/gdbserver"));
    if (bc->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild || !m_certificateAlias.length())
    {
            build<<"debug";
            if (!QFile::copy(AndroidConfigurations::instance().gdbServerPath(target->activeRunConfiguration()->abi().architecture()),
                             androidLibPath+QLatin1String("/gdbserver")))
            {
                raiseError(tr("Can't copy gdbserver from '%1' to '%2'").arg(AndroidConfigurations::instance().gdbServerPath(target->activeRunConfiguration()->abi().architecture()))
                           .arg(androidLibPath+QLatin1String("/gdbserver")));
                return false;
            }
    }
    else
        build<<"release";

    emit addOutput(tr("Creating package file ..."), MessageOutput);
    if (!target->createAndroidTemplatesIfNecessary())
        return false;

    target->updateProject(target->targetSDK(), target->applicationName());

    QProcess * const buildProc = new QProcess;

    connect(buildProc, SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleBuildOutput()));
    connect(buildProc, SIGNAL(readyReadStandardError()), this,
        SLOT(handleBuildOutput()));

    buildProc->setWorkingDirectory(androidDir);

    if (!runCommand(buildProc, AndroidConfigurations::instance().antToolPath(), build))
    {
        disconnect(buildProc, 0, this, 0);
        buildProc->deleteLater();
        return false;
    }

    if (!(bc->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild) && m_certificateAlias.length())
    {
        emit addOutput(tr("Signing package ..."), MessageOutput);
        while(true)
        {
            if (!m_certificatePasswd.length())
                QMetaObject::invokeMethod(this,"certificatePassword", Qt::BlockingQueuedConnection);

            if (!m_certificatePasswd.length())
            {
                disconnect(buildProc, 0, this, 0);
                buildProc->deleteLater();
                return false;
            }

            QByteArray keyPass(m_certificatePasswd.toUtf8());
            keyPass+="\n";
            build.clear();
            build<<"-verbose"<<"-keystore"<<m_keystorePath<<"-storepass"<<m_keystorePasswd
                <<target->apkPath(AndroidTarget::ReleaseBuildUnsigned)
                <<m_certificateAlias;
            buildProc->start(AndroidConfigurations::instance().jarsignerPath(), build);
            if (!buildProc->waitForStarted())
            {
                disconnect(buildProc, 0, this, 0);
                buildProc->deleteLater();
                return false;
            }
            buildProc->write(keyPass);
            if (!buildProc->waitForBytesWritten() || !buildProc->waitForFinished())
            {
                disconnect(buildProc, 0, this, 0);
                buildProc->deleteLater();
                return false;
            }
            if (!buildProc->exitCode())
                break;
            emit addOutput(tr("Failed, try again"), ErrorMessageOutput);
            m_certificatePasswd.clear();
        }
        if (QFile::rename(target->apkPath(AndroidTarget::ReleaseBuildUnsigned), target->apkPath(AndroidTarget::ReleaseBuildSigned)))
        {
            emit addOutput(tr("Release signed package created to %1")
                           .arg(target->apkPath(AndroidTarget::ReleaseBuildSigned))
                           , MessageOutput);

            if (m_openPackageLocation)
                FolderNavigationWidget::showInGraphicalShell(0,
                                                                              target->apkPath(AndroidTarget::ReleaseBuildSigned));
        }
    }
    emit addOutput(tr("Package created."), BuildStep::MessageOutput);
    disconnect(buildProc, 0, this, 0);
    buildProc->deleteLater();
    return true;
}

void AndroidPackageCreationStep::stripAndroidLibs(const QStringList & files, Abi::Architecture architecture)
{
    QProcess stripProcess;
    foreach(QString file, files)
    {
        stripProcess.start(AndroidConfigurations::instance().stripPath(architecture)+" --strip-unneeded "+file);
        if (!stripProcess.waitForFinished(-1))
            stripProcess.terminate();
    }
}

bool AndroidPackageCreationStep::removeDirectory(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists())
        return true;

    const QStringList &files
        = dir.entryList(QDir::Files | QDir::Hidden | QDir::System);
    foreach (const QString &fileName, files) {
        if (!dir.remove(fileName))
            return false;
    }

    const QStringList &subDirs
        = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QString &subDirName, subDirs) {
        if (!removeDirectory(dirPath + QLatin1Char('/') + subDirName))
            return false;
    }

    return dir.rmdir(dirPath);
}

bool AndroidPackageCreationStep::runCommand(QProcess *buildProc
    , const QString &program, const QStringList & arguments)
{
    emit addOutput(tr("Package deploy: Running command '%1 %2'.").arg(program).arg(arguments.join(" ")), BuildStep::MessageOutput);
    buildProc->start(program, arguments);
    if (!buildProc->waitForStarted()) {
        raiseError(tr("Packaging failed."),
                   tr("Packaging error: Could not start command '%1 %2'. Reason: %3")
                               .arg(program).arg(arguments.join(" ")).arg(buildProc->errorString()));
        return false;
    }
    buildProc->waitForFinished(-1);
    if (buildProc->error() != QProcess::UnknownError
        || buildProc->exitCode() != 0) {
        QString mainMessage = tr("Packaging Error: Command '%1 %2' failed.")
                .arg(program).arg(arguments.join(" "));
        if (buildProc->error() != QProcess::UnknownError)
            mainMessage += tr(" Reason: %1").arg(buildProc->errorString());
        else
            mainMessage += tr("Exit code: %1").arg(buildProc->exitCode());
        raiseError(mainMessage);
        return false;
    }
    return true;
}

void AndroidPackageCreationStep::handleBuildOutput()
{
    QProcess * const buildProc = qobject_cast<QProcess *>(sender());
    if (!buildProc)
        return;
    const QByteArray &stdOut = buildProc->readAllStandardOutput();
    const QByteArray &errorOut = buildProc->readAllStandardError();
    if (!stdOut.isEmpty())
        emit addOutput(QString::fromLocal8Bit(stdOut), BuildStep::NormalOutput);
    if (!errorOut.isEmpty()) {
        emit addOutput(QString::fromLocal8Bit(errorOut), BuildStep::ErrorOutput);
    }
}

void AndroidPackageCreationStep::keystorePassword()
{
    m_keystorePasswd.clear();
    bool ok;
    QString text = QInputDialog::getText(0, tr("Keystore"),
                                               tr("Keystore password:"), QLineEdit::Password,
                                               "", &ok);
    if (ok && !text.isEmpty())
        m_keystorePasswd = text;
}

void AndroidPackageCreationStep::certificatePassword()
{
    m_certificatePasswd.clear();
    bool ok;
    QString text = QInputDialog::getText(0, tr("Certificate"),
                                         tr("Certificate password (%1):").arg(m_certificateAlias), QLineEdit::Password,
                                               "", &ok);
    if (ok && !text.isEmpty())
        m_certificatePasswd = text;
}

void AndroidPackageCreationStep::raiseError(const QString &shortMsg,
                                          const QString &detailedMsg)
{
    emit addOutput(detailedMsg.isNull() ? shortMsg : detailedMsg, BuildStep::ErrorOutput);
    emit addTask(Task(Task::Error, shortMsg, QString(), -1,
                      TASK_CATEGORY_BUILDSYSTEM));
}

const QLatin1String AndroidPackageCreationStep::CreatePackageId("Qt4ProjectManager.AndroidPackageCreationStep");

} // namespace Internal
} // namespace Qt4ProjectManager
