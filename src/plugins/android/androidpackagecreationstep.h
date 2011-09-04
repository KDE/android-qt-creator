/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDPACKAGECREATIONSTEP_H
#define ANDROIDPACKAGECREATIONSTEP_H

#include <projectexplorer/abi.h>
#include <projectexplorer/buildstep.h>
#include <QtCore/QAbstractItemModel>
#include "javaparser.h"

QT_BEGIN_NAMESPACE
class QDateTime;
class QFile;
class QProcess;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
}

namespace Android {
namespace Internal {
class AndroidTarget;

class AndroidPackageCreationStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class AndroidPackageCreationFactory;
public:
    AndroidPackageCreationStep(ProjectExplorer::BuildStepList *bsl);
    ~AndroidPackageCreationStep();

    static bool removeDirectory(const QString &dirPath);

    static void stripAndroidLibs(const QStringList &files, ProjectExplorer::Abi::Architecture architecture);

    static const QLatin1String DefaultVersionNumber;

    AndroidTarget * androidTarget() const;

    void checkRequiredLibraries();

    QString keystorePath();
    void setKeystorePath(const QString &path);
    void setKeystorePassword(const QString &pwd);
    void setCertificateAlias(const QString &alias);
    void setCertificatePassword(const QString &pwd);
    void setOpenPackageLocation(bool open);
    QAbstractItemModel * keystoreCertificates();

protected:
    virtual bool fromMap(const QVariantMap &map);
    virtual QVariantMap toMap() const;

private slots:
    void handleBuildStdOutOutput();
    void handleBuildStdErrOutput();
    void keystorePassword();
    void certificatePassword();

private:
    AndroidPackageCreationStep(ProjectExplorer::BuildStepList *buildConfig,
                             AndroidPackageCreationStep *other);

    void ctor();
    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const { return true; }
    bool createPackage();
    bool runCommand(QProcess *buildProc, const QString &program, const QStringList &arguments);
    void raiseError(const QString &shortMsg,
                    const QString &detailedMsg = QString());

    static const QLatin1String CreatePackageId;

private:
    QString m_keystorePath;
    QString m_keystorePasswd;
    QString m_certificateAlias;
    QString m_certificatePasswd;
    bool    m_openPackageLocation;
    JavaParser m_outputParser;
signals:
    void updateRequiredLibrariesModels();
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDPACKAGECREATIONSTEP_H
