/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDDEPLOYSTEP_H
#define ANDROIDDEPLOYSTEP_H

#include "androidconfigurations.h"

#include <projectexplorer/buildstep.h>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>
#include <QtCore/QProcess>

QT_BEGIN_NAMESPACE
class QEventLoop;
class QTimer;
QT_END_NAMESPACE

namespace Android {
namespace Internal {
class AndroidDeviceConfigListModel;
class AndroidPackageCreationStep;

class AndroidDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class AndroidDeployStepFactory;

public:
    enum AndroidDeployAction
    {
        NoDeploy,
        DeployLocal,
        InstallQASI
    };

public:
    AndroidDeployStep(ProjectExplorer::BuildStepList *bc);

    virtual ~AndroidDeployStep();

    QString deviceSerialNumber();
    int deviceAPILevel();
    QString localLibsRulesFilePath();

    AndroidDeployAction deployAction();
    bool useLocalQtLibs();

public slots:
    void setDeployAction(AndroidDeployAction deploy);
    void setDeployQASIPackagePath(const QString & package);
    void setUseLocalQtLibs(bool useLocal);

signals:
    void done();
    void error();
    void resetDelopyAction();

private slots:
    bool deployPackage();
    void handleBuildOutput();

private:

    AndroidDeployStep(ProjectExplorer::BuildStepList *bc,
        AndroidDeployStep *other);
    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const { return true; }

    virtual QVariantMap toMap() const;
    virtual bool fromMap(const QVariantMap &map);
    void copyLibs(const QString &srcPath, const QString & destPath, QStringList & copiedLibs, const QStringList &filter=QStringList());
    void ctor();
    void raiseError(const QString &error);
    void writeOutput(const QString &text, OutputFormat = MessageOutput);
    bool runCommand(QProcess *buildProc, const QString &program, const QStringList & arguments);

private:
    QString m_deviceSerialNumber;
    int m_deviceAPILevel;
    QString m_QASIPackagePath;
    AndroidDeployAction m_deployAction;
    bool m_useLocalQtLibs;

    static const QLatin1String Id;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDDEPLOYSTEP_H
