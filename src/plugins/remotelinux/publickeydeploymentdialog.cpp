/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
#include "publickeydeploymentdialog.h"

#include "linuxdeviceconfiguration.h"
#include "sshkeydeployer.h"

#include <utils/ssh/sshconnection.h>

#include <QtCore/QTimer>
#include <QtGui/QFileDialog>

namespace RemoteLinux {
namespace Internal {
class PublicKeyDeploymentDialogPrivate
{
public:
    SshKeyDeployer *keyDeployer;
    bool done;
};
} // namespace Internal;

using namespace Internal;

PublicKeyDeploymentDialog::PublicKeyDeploymentDialog(const LinuxDeviceConfiguration::ConstPtr &deviceConfig,
        QWidget *parent)
    : QProgressDialog(parent), d(new PublicKeyDeploymentDialogPrivate)
{
    setAutoReset(false);
    setAutoClose(false);
    setMinimumDuration(0);
    setMaximum(1);

    d->keyDeployer = new SshKeyDeployer(this);
    d->done = false;

    setLabelText(tr("Waiting for file name..."));
    const Utils::SshConnectionParameters sshParams = deviceConfig->sshParameters();
    const QString &dir = QFileInfo(sshParams.privateKeyFile).path();
    QString publicKeyFileName = QFileDialog::getOpenFileName(this,
        tr("Choose Public Key File"), dir,
        tr("Public Key Files (*.pub);;All Files (*)"));
    if (publicKeyFileName.isEmpty()) {
        QTimer::singleShot(0, this, SLOT(reject()));
        return;
    }

    setLabelText(tr("Deploying..."));
    setValue(0);
    connect(this, SIGNAL(canceled()), SLOT(handleCanceled()));
    connect(d->keyDeployer, SIGNAL(error(QString)), SLOT(handleDeploymentError(QString)));
    connect(d->keyDeployer, SIGNAL(finishedSuccessfully()), SLOT(handleDeploymentSuccess()));
    d->keyDeployer->deployPublicKey(sshParams, publicKeyFileName);
}

PublicKeyDeploymentDialog::~PublicKeyDeploymentDialog()
{
    delete d;
}

void PublicKeyDeploymentDialog::handleDeploymentSuccess()
{
    handleDeploymentFinished(QString());
    setValue(1);
    d->done = true;
}

void PublicKeyDeploymentDialog::handleDeploymentError(const QString &errorMsg)
{
    handleDeploymentFinished(errorMsg);
}

void PublicKeyDeploymentDialog::handleDeploymentFinished(const QString &errorMsg)
{
    QString buttonText;
    const char *textColor;
    if (errorMsg.isEmpty()) {
        buttonText = tr("Deployment finished successfully.");
        textColor = "blue";
    } else {
        buttonText = errorMsg;
        textColor = "red";
    }
    setLabelText(QString::fromLocal8Bit("<font color=\"%1\">%2</font>").arg(textColor, buttonText));
    setCancelButtonText(tr("Close"));
}

void PublicKeyDeploymentDialog::handleCanceled()
{
    disconnect(d->keyDeployer, 0, this, 0);
    d->keyDeployer->stopDeployment();
    if (d->done)
        accept();
    else
        reject();
}

} // namespace RemoteLinux
