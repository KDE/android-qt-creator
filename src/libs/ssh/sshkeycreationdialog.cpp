/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
#include "sshkeycreationdialog.h"
#include "ui_sshkeycreationdialog.h"

#include "sshkeygenerator.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QApplication>
#include <QDesktopServices>
#include <QMessageBox>

namespace QSsh {

SshKeyCreationDialog::SshKeyCreationDialog(QWidget *parent)
    : QDialog(parent), m_keyGenerator(0), m_ui(new Ui::SshKeyCreationDialog)
{
    m_ui->setupUi(this);
    const QString defaultPath = QDesktopServices::storageLocation(QDesktopServices::HomeLocation)
        + QLatin1String("/.ssh/qtc_id");
    setPrivateKeyFile(defaultPath);

    connect(m_ui->rsa, SIGNAL(toggled(bool)), this, SLOT(keyTypeChanged()));
    connect(m_ui->dsa, SIGNAL(toggled(bool)), this, SLOT(keyTypeChanged()));
    connect(m_ui->privateKeyFileButton, SIGNAL(clicked()), SLOT(handleBrowseButtonClicked()));
    connect(m_ui->generateButton, SIGNAL(clicked()), this, SLOT(generateKeys()));
}

SshKeyCreationDialog::~SshKeyCreationDialog()
{
    delete m_keyGenerator;
}

void SshKeyCreationDialog::keyTypeChanged()
{
    m_ui->comboBox->setCurrentIndex(0);
    m_ui->comboBox->setEnabled(m_ui->rsa->isChecked());
}

void SshKeyCreationDialog::generateKeys()
{
    const SshKeyGenerator::KeyType keyType = m_ui->rsa->isChecked()
        ? SshKeyGenerator::Rsa
        : SshKeyGenerator::Dsa;

    if (!m_keyGenerator)
        m_keyGenerator = new SshKeyGenerator;

    QApplication::setOverrideCursor(Qt::BusyCursor);
    const bool success = m_keyGenerator->generateKeys(keyType, SshKeyGenerator::Mixed,
        m_ui->comboBox->currentText().toUShort());
    QApplication::restoreOverrideCursor();

    if (success) {
        saveKeys();
    } else {
        QMessageBox::critical(this, tr("Key Generation Failed"), m_keyGenerator->error());
    }
}

void SshKeyCreationDialog::handleBrowseButtonClicked()
{
    const QString filePath = QFileDialog::getSaveFileName(this, tr("Choose Private Key File Name"));
    if (!filePath.isEmpty())
        setPrivateKeyFile(filePath);
}

void SshKeyCreationDialog::setPrivateKeyFile(const QString &filePath)
{
    m_ui->privateKeyFileValueLabel->setText(filePath);
    m_ui->generateButton->setEnabled(!privateKeyFilePath().isEmpty());
    m_ui->publicKeyFileLabel->setText(filePath + QLatin1String(".pub"));
}

void SshKeyCreationDialog::saveKeys()
{
    const QString parentDir = QFileInfo(privateKeyFilePath()).dir().path();
    if (!QDir::root().mkpath(parentDir)) {
        QMessageBox::critical(this, tr("Failure To Save Key File"),
            tr("Failed to create directory: '%1'.").arg(parentDir));
        return;
    }

    QFile privateKeyFile(privateKeyFilePath());
    if (!privateKeyFile.open(QIODevice::WriteOnly)
            || !privateKeyFile.write(m_keyGenerator->privateKey())) {
        QMessageBox::critical(this, tr("Saving Private Key File failed"),
            tr("The private key file could not be saved: %1").arg(privateKeyFile.errorString()));
        return;
    }
    QFile::setPermissions(privateKeyFilePath(), QFile::ReadOwner | QFile::WriteOwner);

    QFile publicKeyFile(publicKeyFilePath());
    if (!publicKeyFile.open(QIODevice::WriteOnly)
            || !publicKeyFile.write(m_keyGenerator->publicKey())) {
        QMessageBox::critical(this, tr("Saving Public Key File failed"),
            tr("The public key file could not be saved: %1").arg(publicKeyFile.errorString()));
        return;
    }

    accept();
}

QString SshKeyCreationDialog::privateKeyFilePath() const
{
    return m_ui->privateKeyFileValueLabel->text();
}

QString SshKeyCreationDialog::publicKeyFilePath() const
{
    return m_ui->publicKeyFileLabel->text();
}

} // namespace QSsh
