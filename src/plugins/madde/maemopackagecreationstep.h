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

#ifndef MAEMOPACKAGECREATIONSTEP_H
#define MAEMOPACKAGECREATIONSTEP_H

#include <remotelinux/abstractpackagingstep.h>
#include <utils/environment.h>

QT_BEGIN_NAMESPACE
class QDateTime;
class QFile;
class QProcess;
QT_END_NAMESPACE

namespace Qt4ProjectManager { class Qt4BuildConfiguration; }

namespace RemoteLinux {
class RemoteLinuxDeployConfiguration;
}

namespace Madde {
namespace Internal {
class AbstractQt4MaemoTarget;
class AbstractDebBasedQt4MaemoTarget;
class AbstractRpmBasedQt4MaemoTarget;

class AbstractMaemoPackageCreationStep : public RemoteLinux::AbstractPackagingStep
{
    Q_OBJECT
public:
    virtual ~AbstractMaemoPackageCreationStep();

    QString versionString(QString *error) const;
    bool setVersionString(const QString &version, QString *error);

    static void preparePackagingProcess(QProcess *proc,
        const Qt4ProjectManager::Qt4BuildConfiguration *bc,
        const QString &workingDir);

    const Qt4ProjectManager::Qt4BuildConfiguration *qt4BuildConfiguration() const;
    AbstractQt4MaemoTarget *maemoTarget() const;
    AbstractDebBasedQt4MaemoTarget *debBasedMaemoTarget() const;
    AbstractRpmBasedQt4MaemoTarget *rpmBasedMaemoTarget() const;

    static const QLatin1String DefaultVersionNumber;


protected:
    AbstractMaemoPackageCreationStep(ProjectExplorer::BuildStepList *bsl,
        const QString &id);
    AbstractMaemoPackageCreationStep(ProjectExplorer::BuildStepList *buildConfig,
                             AbstractMaemoPackageCreationStep *other);

    bool callPackagingCommand(QProcess *proc, const QStringList &arguments);
    QString replaceDots(const QString &name) const;
    virtual bool init();

private slots:
    void handleBuildOutput();

private:
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    virtual QString packageFileName() const;

    virtual bool createPackage(QProcess *buildProc, const QFutureInterface<bool> &fi) = 0;
    virtual bool isMetaDataNewerThan(const QDateTime &packageDate) const = 0;

    static QString nativePath(const QFile &file);
    bool isPackagingNeeded() const;

    const Qt4ProjectManager::Qt4BuildConfiguration *m_lastBuildConfig;
    bool m_packagingNeeded;
    Utils::Environment m_environment;
    QString m_qmakeCommand;
};


class MaemoDebianPackageCreationStep : public AbstractMaemoPackageCreationStep
{
    Q_OBJECT
    friend class MaemoPackageCreationFactory;
public:
    MaemoDebianPackageCreationStep(ProjectExplorer::BuildStepList *bsl);

    static void ensureShlibdeps(QByteArray &rulesContent);

private:
    MaemoDebianPackageCreationStep(ProjectExplorer::BuildStepList *buildConfig,
        MaemoDebianPackageCreationStep *other);

    virtual bool init();
    virtual bool createPackage(QProcess *buildProc, const QFutureInterface<bool> &fi);
    virtual bool isMetaDataNewerThan(const QDateTime &packageDate) const;

    void ctor();
    static QString packagingCommand(const QString &maddeRoot, const QString &commandName);
    bool copyDebianFiles(bool inSourceBuild);
    void checkProjectName();
    bool adaptRulesFile(const QString &templatePath, const QString &rulesFilePath);

    QString m_maddeRoot;
    QString m_projectDirectory;
    QString m_pkgFileName;
    QString m_packageName;
    QString m_templatesDirPath;
    bool m_debugBuild;

    static const QString CreatePackageId;
};

class MaemoRpmPackageCreationStep : public AbstractMaemoPackageCreationStep
{
    Q_OBJECT
    friend class MaemoPackageCreationFactory;
public:
    MaemoRpmPackageCreationStep(ProjectExplorer::BuildStepList *bsl);

private:
    bool init();
    virtual bool createPackage(QProcess *buildProc, const QFutureInterface<bool> &fi);
    virtual bool isMetaDataNewerThan(const QDateTime &packageDate) const;

    MaemoRpmPackageCreationStep(ProjectExplorer::BuildStepList *buildConfig,
        MaemoRpmPackageCreationStep *other);

    void ctor();
    QString rpmBuildDir() const;

    QString m_specFile;
    QString m_packageFileName;

    static const QString CreatePackageId;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOPACKAGECREATIONSTEP_H
