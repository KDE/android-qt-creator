/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef MAEMOINSTALLTOSYSROOTSTEP_H
#define MAEMOINSTALLTOSYSROOTSTEP_H

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildstep.h>

#include <QtCore/QStringList>

QT_FORWARD_DECLARE_CLASS(QProcess)

namespace RemoteLinux {
namespace Internal {

class AbstractMaemoInstallPackageToSysrootStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
public:
    virtual bool init() { return true; }
    virtual void run(QFutureInterface<bool> &fi);

protected:
    AbstractMaemoInstallPackageToSysrootStep(ProjectExplorer::BuildStepList *bsl,
        const QString &id);
    AbstractMaemoInstallPackageToSysrootStep(ProjectExplorer::BuildStepList *bsl,
        AbstractMaemoInstallPackageToSysrootStep *other);

private slots:
    void handleInstallerStdout();
    void handleInstallerStderr();

private:
    virtual QStringList madArguments() const=0;

    QProcess *m_installerProcess;
};

class MaemoInstallDebianPackageToSysrootStep : public AbstractMaemoInstallPackageToSysrootStep
{
    Q_OBJECT
public:
    explicit MaemoInstallDebianPackageToSysrootStep(ProjectExplorer::BuildStepList *bsl);
    MaemoInstallDebianPackageToSysrootStep(ProjectExplorer::BuildStepList *bsl,
        MaemoInstallDebianPackageToSysrootStep *other);

    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    static const QString Id;
    static QString displayName();
private:
    virtual QStringList madArguments() const;
};

class MaemoInstallRpmPackageToSysrootStep : public AbstractMaemoInstallPackageToSysrootStep
{
    Q_OBJECT
public:
    explicit MaemoInstallRpmPackageToSysrootStep(ProjectExplorer::BuildStepList *bsl);
    MaemoInstallRpmPackageToSysrootStep(ProjectExplorer::BuildStepList *bsl,
        MaemoInstallRpmPackageToSysrootStep *other);

    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    static const QString Id;
    static QString displayName();
private:
    virtual QStringList madArguments() const;
};

class MaemoCopyToSysrootStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
public:
    explicit MaemoCopyToSysrootStep(ProjectExplorer::BuildStepList *bsl);
    MaemoCopyToSysrootStep(ProjectExplorer::BuildStepList *bsl,
        MaemoCopyToSysrootStep *other);

    virtual bool init() { return true; }
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    static const QString Id;
    static QString displayName();
};

class MaemoMakeInstallToSysrootStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    explicit MaemoMakeInstallToSysrootStep(ProjectExplorer::BuildStepList *bsl);
    MaemoMakeInstallToSysrootStep(ProjectExplorer::BuildStepList *bsl,
        MaemoMakeInstallToSysrootStep *other);

    virtual bool immutable() const { return false; }
    virtual bool init();
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    static const QString Id;
    static QString displayName();
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMOINSTALLTOSYSROOTSTEP_H
