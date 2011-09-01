/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidcreatekeystorecertificate.h"
#include "androidconfigurations.h"
#include "ui_androidcreatekeystorecertificate.h"
#include <QtGui/QFileDialog>
#include <QtCore/QProcess>
#include <QtGui/QMessageBox>

using namespace Android::Internal;

AndroidCreateKeystoreCertificate::AndroidCreateKeystoreCertificate(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AndroidCreateKeystoreCertificate)
{
    ui->setupUi(this);
    connect(ui->keystorePassLineEdit, SIGNAL(textChanged(QString)), this, SLOT(checkKeystorePassword()));
    connect(ui->keystoreRetypePassLineEdit, SIGNAL(textChanged(QString)), this, SLOT(checkKeystorePassword()));
    connect(ui->certificatePassLineEdit, SIGNAL(textChanged(QString)), this, SLOT(checkCertificatePassword()));
    connect(ui->certificateRetypePassLineEdit, SIGNAL(textChanged(QString)), this, SLOT(checkCertificatePassword()));
}

AndroidCreateKeystoreCertificate::~AndroidCreateKeystoreCertificate()
{
    delete ui;
}

QString AndroidCreateKeystoreCertificate::keystoreFilePath()
{
    return m_keystoreFilePath;
}

QString AndroidCreateKeystoreCertificate::keystorePassword()
{
    return ui->keystorePassLineEdit->text();
}

QString AndroidCreateKeystoreCertificate::certificateAlias()
{
    return ui->aliasNameLineEdit->text();
}

QString AndroidCreateKeystoreCertificate::certificatePassword()
{
    return ui->certificatePassLineEdit->text();
}

AndroidCreateKeystoreCertificate::PasswordStatus AndroidCreateKeystoreCertificate::checkKeystorePassword()
{
    if (ui->keystorePassLineEdit->text().length() < 6) {
        ui->keystorePassInfoLabel->setText(tr("<span style=\" color:#ff0000;\">Password is too short</span>"));
        return Invalid;
    }
    if (ui->keystorePassLineEdit->text() != ui->keystoreRetypePassLineEdit->text()) {
            ui->keystorePassInfoLabel->setText(tr("<span style=\" color:#ff0000;\">Passwords don't match</span>"));
            return DontMatch;
    }
    ui->keystorePassInfoLabel->setText(tr("<span style=\" color:#00ff00;\">Password is ok</span>"));
    return Match;
}

AndroidCreateKeystoreCertificate::PasswordStatus AndroidCreateKeystoreCertificate::checkCertificatePassword()
{
    if (ui->certificatePassLineEdit->text().length() < 6) {
        ui->certificatePassInfoLabel->setText(tr("<span style=\" color:#ff0000;\">Password is too short</span>"));
        return Invalid;
    }
    if (ui->certificatePassLineEdit->text() != ui->certificateRetypePassLineEdit->text()) {
            ui->certificatePassInfoLabel->setText(tr("<span style=\" color:#ff0000;\">Passwords don't match</span>"));
            return DontMatch;
    }
    ui->certificatePassInfoLabel->setText(tr("<span style=\" color:#00ff00;\">Password is ok</span>"));
    return Match;
}

void AndroidCreateKeystoreCertificate::on_keystoreShowPassCheckBox_stateChanged(int state)
{
    ui->keystorePassLineEdit->setEchoMode(state==Qt::Checked?QLineEdit::Normal:QLineEdit::Password);
    ui->keystoreRetypePassLineEdit->setEchoMode(ui->keystorePassLineEdit->echoMode());
}

void AndroidCreateKeystoreCertificate::on_certificateShowPassCheckBox_stateChanged(int state)
{
    ui->certificatePassLineEdit->setEchoMode(state==Qt::Checked?QLineEdit::Normal:QLineEdit::Password);
    ui->certificateRetypePassLineEdit->setEchoMode(ui->certificatePassLineEdit->echoMode());
}

void AndroidCreateKeystoreCertificate::on_buttonBox_accepted()
{
    switch (checkKeystorePassword()) {
    case Invalid:
        ui->keystorePassLineEdit->setFocus();
        return;
    case DontMatch:
        ui->keystoreRetypePassLineEdit->setFocus();
        return;
    default:
        break;
    }

    switch (checkCertificatePassword()) {
    case Invalid:
        ui->certificatePassLineEdit->setFocus();
        return;
    case DontMatch:
        ui->certificateRetypePassLineEdit->setFocus();
        return;
    default:
        break;
    }

    if (!ui->aliasNameLineEdit->text().length())
        ui->aliasNameLineEdit->setFocus();

    if (!ui->commonNameLineEdit->text().length())
        ui->commonNameLineEdit->setFocus();

    if (!ui->organizationNameLineEdit->text().length())
        ui->organizationNameLineEdit->setFocus();

    if (!ui->localityNameLineEdit->text().length())
        ui->localityNameLineEdit->setFocus();

    if (!ui->countryLineEdit->text().length())
        ui->countryLineEdit->setFocus();

    m_keystoreFilePath=QFileDialog::getSaveFileName(this, tr("Keystore file name"),
                                                    QDir::homePath() + "/android_release.keystore",
                                                    tr("Keystore files (*.keystore *.jks)"));
    if (!m_keystoreFilePath.length())
        return;
    QString distinguishedNames(QString("CN=%1, O=%2, L=%3, C=%4")
                               .arg(ui->commonNameLineEdit->text().replace(",", "\\,"))
                               .arg(ui->organizationNameLineEdit->text().replace(",", "\\,"))
                               .arg(ui->localityNameLineEdit->text().replace(",", "\\,"))
                               .arg(ui->countryLineEdit->text().replace(",", "\\,")));

    if (ui->organizationUnitLineEdit->text().length())
        distinguishedNames+=", OU="+ui->organizationUnitLineEdit->text().replace(",","\\,");

    if (ui->stateNameLineEdit->text().length())
        distinguishedNames+=", S="+ui->stateNameLineEdit->text().replace(",","\\,");

    QStringList params;
    params<<"-genkey"<<"-keyalg"<<"RSA"
         <<"-keystore"<<m_keystoreFilePath
         <<"-storepass"<<ui->keystorePassLineEdit->text()
         <<"-alias"<<ui->aliasNameLineEdit->text()
         <<"-keysize"<<ui->keySizeSpinBox->text()
         <<"-validity"<<ui->validitySpinBox->text()
         <<"-keypass"<<ui->certificatePassLineEdit->text()
         <<"-dname"<<distinguishedNames;

    QProcess genKeyCertProc;
    genKeyCertProc.start(AndroidConfigurations::instance().keytoolPath(), params );

    if (!genKeyCertProc.waitForStarted() || !genKeyCertProc.waitForFinished())
        return;

    if (genKeyCertProc.exitCode()) {
        QMessageBox::critical(this, tr("Error")
                              , genKeyCertProc.readAllStandardOutput()
                              + genKeyCertProc.readAllStandardError());
        return;
    }
    accept();
}
