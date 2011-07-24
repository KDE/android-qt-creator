#ifndef ANDROIDCREATEKEYSTORECERTIFICATE_H
#define ANDROIDCREATEKEYSTORECERTIFICATE_H

#include <QDialog>

namespace Ui {
    class AndroidCreateKeystoreCertificate;
}
namespace Android {
namespace Internal {
class AndroidCreateKeystoreCertificate : public QDialog
{
    Q_OBJECT
    enum PasswordStatus
    {
        Invald,
        DontMatch,
        Match
    };

public:
    explicit AndroidCreateKeystoreCertificate(QWidget *parent = 0);
    ~AndroidCreateKeystoreCertificate();
    QString keystoreFilePath();
    QString keystorePassword();
    QString certificateAlias();
    QString certificatePassword();

private slots:
    PasswordStatus checkKeystrorePassword();
    PasswordStatus checkCertificatePassword();
    void on_keystoreShowPassCheckBox_stateChanged(int state);
    void on_certificateShowPassCheckBox_stateChanged(int state);
    void on_buttonBox_accepted();

private:
    Ui::AndroidCreateKeystoreCertificate *ui;
    QString m_keystoreFilePath;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDCREATEKEYSTORECERTIFICATE_H
