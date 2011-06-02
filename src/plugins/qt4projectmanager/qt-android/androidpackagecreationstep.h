/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDPACKAGECREATIONSTEP_H
#define ANDROIDPACKAGECREATIONSTEP_H

#include <projectexplorer/buildstep.h>

QT_BEGIN_NAMESPACE
class QDateTime;
class QFile;
class QProcess;
QT_END_NAMESPACE

namespace Qt4ProjectManager {

class Qt4BuildConfiguration;

namespace Internal {
class Qt4AndroidTarget;

class AndroidPackageCreationStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class AndroidPackageCreationFactory;
public:
    AndroidPackageCreationStep(ProjectExplorer::BuildStepList *bsl);
    ~AndroidPackageCreationStep();

    static bool removeDirectory(const QString &dirPath);

    static void stripAndroidLibs(const QStringList & files);

    static const QLatin1String DefaultVersionNumber;

    Qt4AndroidTarget * androidTarget() const;

    void checkRequiredLibraries();

private slots:
    void handleBuildOutput();

private:
    AndroidPackageCreationStep(ProjectExplorer::BuildStepList *buildConfig,
                             AndroidPackageCreationStep *other);

    void ctor();
    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const { return true; }
    bool createPackage(QProcess *buildProc);
    bool runCommand(QProcess *buildProc, const QString &program, const QStringList & arguments);
    void raiseError(const QString &shortMsg,
                    const QString &detailedMsg = QString());

    static const QLatin1String CreatePackageId;

signals:
    void updateRequiredLibrariesModels();
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDPACKAGECREATIONSTEP_H
