/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef ANDROIDDEPLOYSTEP_H
#define ANDROIDDEPLOYSTEP_H

#include "androidconfigurations.h"

#include <projectexplorer/buildstep.h>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QEventLoop;
class QProcess;
class QTimer;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {
class AndroidDeviceConfigListModel;
class AndroidPackageCreationStep;
class AndroidToolChain;

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

    AndroidDeployAction deployAction();

public slots:
    void setDeployAction(AndroidDeployAction deploy);
    void setDeployQASIPackagePath(const QString & package);

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
    bool runCommand(QProcess *buildProc, const QString &command);

private:
    QString m_deviceSerialNumber;
    QString m_QASIPackagePath;
    AndroidDeployAction m_deployAction;

    static const QLatin1String Id;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDDEPLOYSTEP_H
