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

class AndroidDeployStep;
class AndroidToolChain;
class AndroidProFileWrapper;

class AndroidPackageCreationStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class AndroidPackageCreationFactory;
public:
    AndroidPackageCreationStep(ProjectExplorer::BuildStepList *bsl);
    ~AndroidPackageCreationStep();

    QString packageFilePath() const;
    bool isPackagingEnabled() const;
    void setPackagingEnabled(bool enabled) { m_packagingEnabled = enabled; }

    const AndroidToolChain *androidToolChain() const;

    static QString packageName(const ProjectExplorer::Project *project);
    static QString packageFileName(const ProjectExplorer::Project *project,
        const QString &version);
    static bool removeDirectory(const QString &dirPath);

    QString projectName() const;

    static const QLatin1String DefaultVersionNumber;

signals:
    void packageFilePathChanged();
    void qtVersionChanged();

private slots:
    void handleBuildOutput();
    void handleBuildConfigChanged();

private:
    AndroidPackageCreationStep(ProjectExplorer::BuildStepList *buildConfig,
                             AndroidPackageCreationStep *other);

    void ctor();
    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const { return true; }
    virtual QVariantMap toMap() const;
    virtual bool fromMap(const QVariantMap &map);

    bool createPackage(QProcess *buildProc);
    bool runCommand(QProcess *buildProc, const QString &command);
    bool packagingNeeded() const;
    bool isFileNewerThan(const QString &filePath,
        const QDateTime &timeStamp) const;
    void raiseError(const QString &shortMsg,
                    const QString &detailedMsg = QString());
    QString buildDirectory() const;
    const Qt4BuildConfiguration *qt4BuildConfiguration() const;
    AndroidDeployStep * deployStep() const;

    static const QLatin1String CreatePackageId;

    bool m_packagingEnabled;
    const Qt4BuildConfiguration *m_lastBuildConfig;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDPACKAGECREATIONSTEP_H
