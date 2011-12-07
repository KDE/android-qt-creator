/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef LOCALAPPLICATIONRUNCONTROL_H
#define LOCALAPPLICATIONRUNCONTROL_H

#include "runconfiguration.h"
#include "applicationlauncher.h"

namespace ProjectExplorer {

class LocalApplicationRunConfiguration;
namespace Internal {

class LocalApplicationRunControlFactory : public IRunControlFactory
{
    Q_OBJECT
public:
    LocalApplicationRunControlFactory ();
    virtual ~LocalApplicationRunControlFactory();
    virtual bool canRun(RunConfiguration *runConfiguration, const QString &mode) const;
    virtual QString displayName() const;
    virtual RunControl* create(RunConfiguration *runConfiguration, const QString &mode);
    virtual RunConfigWidget *createConfigurationWidget(RunConfiguration  *runConfiguration);
};

class LocalApplicationRunControl : public RunControl
{
    Q_OBJECT
public:
    LocalApplicationRunControl(LocalApplicationRunConfiguration *runConfiguration, QString mode);
    virtual ~LocalApplicationRunControl();
    virtual void start();
    virtual StopResult stop();
    virtual bool isRunning() const;
    virtual QIcon icon() const;
private slots:
    void processStarted();
    void processExited(int exitCode);
    void slotAppendMessage(const QString &err, Utils::OutputFormat isError);
private:
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;
    QString m_executable;
    QString m_commandLineArguments;
    ProjectExplorer::ApplicationLauncher::Mode m_runMode;
    ProcessHandle m_applicationProcessHandle;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // LOCALAPPLICATIONRUNCONTROL_H
