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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "androidconfigtestdialog.h"
#include "ui_androidconfigtestdialog.h"

#include "androiddeviceconfigurations.h"
#include "androidglobal.h"
#include "androidusedportsgatherer.h"

#include <coreplugin/ssh/sshremoteprocessrunner.h>

#include <QtGui/QPalette>
#include <QtGui/QPushButton>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

AndroidConfigTestDialog::AndroidConfigTestDialog(const AndroidConfig &config, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui_AndroidConfigTestDialog)
    , m_config(config)
    , m_portsGatherer(new AndroidUsedPortsGatherer(this))
{
    setAttribute(Qt::WA_DeleteOnClose);

    m_ui->setupUi(this);
    m_closeButton = m_ui->buttonBox->button(QDialogButtonBox::Close);

    connect(m_closeButton, SIGNAL(clicked()), SLOT(stopConfigTest()));
    connect(m_portsGatherer, SIGNAL(error(QString)),
        SLOT(handlePortListFailure(QString)));
    connect(m_portsGatherer, SIGNAL(portListReady()),
        SLOT(handlePortListReady()));

    startConfigTest();
}

AndroidConfigTestDialog::~AndroidConfigTestDialog()
{
    stopConfigTest();
}

void AndroidConfigTestDialog::startConfigTest()
{
    if (m_testProcessRunner)
        return;

    m_currentTest = GeneralTest;
    m_ui->testResultEdit->setPlainText(tr("Testing configuration..."));
    m_closeButton->setText(tr("Stop Test"));
//    m_testProcessRunner = SshRemoteProcessRunner::create(m_config.server);
//    connect(m_testProcessRunner.data(), SIGNAL(connectionError(Core::SshError)),
//        this, SLOT(handleConnectionError()));
//    connect(m_testProcessRunner.data(), SIGNAL(processClosed(int)), this,
//        SLOT(handleTestProcessFinished(int)));
//    connect(m_testProcessRunner.data(),
//        SIGNAL(processOutputAvailable(QByteArray)), this,
//        SLOT(processSshOutput(QByteArray)));
//    QLatin1String sysInfoCmd("uname -rsm");
//    QLatin1String qtInfoCmd("dpkg-query -W -f '${Package} ${Version} ${Status}\n' 'libqt*' "
//        "|grep ' installed$'");
//    QString command(sysInfoCmd + " && " + qtInfoCmd);
//    m_testProcessRunner->run(command.toUtf8());
}

void AndroidConfigTestDialog::handleConnectionError()
{
    if (!m_testProcessRunner)
        return;
    QString output = tr("Could not connect to host: %1")
        .arg(m_testProcessRunner->connection()->errorString());
    if (m_config.type == AndroidConfig::Simulator)
        output += tr("\nDid you start Qemu?");
    m_ui->testResultEdit->setPlainText(output);
    stopConfigTest();
}

void AndroidConfigTestDialog::handleTestProcessFinished(int exitStatus)
{
    if (!m_testProcessRunner)
        return;

    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    if (m_currentTest == GeneralTest)
        handleGeneralTestResult(exitStatus);
    else
        handleMadDeveloperTestResult(exitStatus);
}

void AndroidConfigTestDialog::handleGeneralTestResult(int exitStatus)
{
    if (exitStatus != SshRemoteProcess::ExitedNormally
        || m_testProcessRunner->process()->exitCode() != 0) {
        m_ui->testResultEdit->setPlainText(tr("Remote process failed: %1")
            .arg(m_testProcessRunner->process()->errorString()));
    } else {
        const QString &output = parseTestOutput();
        if (!m_qtVersionOk) {
            m_ui->errorLabel->setText(tr("Qt version mismatch! "
                " Expected Qt on device: 4.6.2 or later."));
        }
        m_ui->testResultEdit->setPlainText(output);
    }

    m_currentTest = MadDeveloperTest;
    disconnect(m_testProcessRunner.data(),
        SIGNAL(processOutputAvailable(QByteArray)), this,
        SLOT(processSshOutput(QByteArray)));
    const QByteArray command = "test -x " + AndroidGlobal::remoteSudo().toUtf8();
    m_testProcessRunner->run(command);
}

void AndroidConfigTestDialog::handleMadDeveloperTestResult(int exitStatus)
{
    if (exitStatus != SshRemoteProcess::ExitedNormally) {
        m_ui->testResultEdit->setPlainText(tr("Remote process failed: %1")
            .arg(m_testProcessRunner->process()->errorString()));
    } else if (m_testProcessRunner->process()->exitCode() != 0) {
        m_ui->errorLabel->setText(m_ui->errorLabel->text()
            + QLatin1String("<br>") + tr("Mad Developer is not installed.<br>"
                  "You will not be able to deploy to this device."));
    }
    if (m_config.freePorts().hasMore())
        m_portsGatherer->start(m_testProcessRunner->connection(),
            m_config.freePorts());
    else
        finish();
}

void AndroidConfigTestDialog::handlePortListFailure(const QString &errMsg)
{
    m_ui->testResultEdit->appendPlainText(tr("Error retrieving list of used ports: %1")
        .arg(errMsg));
    finish();
}

void AndroidConfigTestDialog::handlePortListReady()
{
    const QList<int> &usedPorts = m_portsGatherer->usedPorts();
    QString output;
    if (usedPorts.isEmpty()) {
        output = tr("All specified ports are available.");
    } else {
        output = tr("The following supposedly free ports are being used on the device:");
        foreach (const int port, usedPorts)
            output += QLatin1Char(' ') + QString::number(port);
    }
    m_ui->testResultEdit->appendPlainText(output);
    finish();
}

void AndroidConfigTestDialog::finish()
{
    if (m_ui->errorLabel->text().isEmpty()) {
        QPalette palette = m_ui->errorLabel->palette();
        palette.setColor(m_ui->errorLabel->foregroundRole(),
            QColor(QLatin1String("blue")));
        m_ui->errorLabel->setPalette(palette);
        m_ui->errorLabel->setText(tr("Device configuration okay."));
    }
    stopConfigTest();
}

void AndroidConfigTestDialog::stopConfigTest()
{
    if (m_testProcessRunner) {
        disconnect(m_testProcessRunner.data(), 0, this, 0);
        m_testProcessRunner = SshRemoteProcessRunner::Ptr();
    }
    m_deviceTestOutput.clear();
    m_closeButton->setText(tr("Close"));
}

void AndroidConfigTestDialog::processSshOutput(const QByteArray &output)
{
    m_deviceTestOutput.append(QString::fromUtf8(output));
}

QString AndroidConfigTestDialog::parseTestOutput()
{
    m_qtVersionOk = false;

    QString output;
    const QRegExp unamePattern(QLatin1String("Linux (\\S+)\\s(\\S+)"));
    int index = unamePattern.indexIn(m_deviceTestOutput);
    if (index == -1) {
        output = tr("Device configuration test failed: Unexpected output:\n%1")
            .arg(m_deviceTestOutput);
        return output;
    }

    output = tr("Hardware architecture: %1\n").arg(unamePattern.cap(2));
    output.append(tr("Kernel version: %1\n").arg(unamePattern.cap(1)));
    const QRegExp dkpgPattern(QLatin1String("(\\S+) (\\S*(\\d+)\\.(\\d+)\\.(\\d+)\\S*) \\S+ \\S+ \\S+"));
    index = dkpgPattern.indexIn(m_deviceTestOutput);
    if (index == -1) {
        output.append(tr("No Qt packages installed."));
        return output;
    }

    output.append(tr("List of installed Qt packages:") + QLatin1Char('\n'));
    do {
        output.append(QLatin1Char('\t') + dkpgPattern.cap(1) + QLatin1Char(' ')
            + dkpgPattern.cap(2) + QLatin1Char('\n'));
        index = dkpgPattern.indexIn(m_deviceTestOutput, index
            + dkpgPattern.cap(0).length());
        if (!m_qtVersionOk && QT_VERSION_CHECK(dkpgPattern.cap(3).toInt(),
            dkpgPattern.cap(4).toInt(), dkpgPattern.cap(5).toInt()) >= 0x040602) {
            m_qtVersionOk = true;
        }
    } while (index != -1);
    return output;
}

} // namespace Internal
} // namespace Qt4ProjectManager
