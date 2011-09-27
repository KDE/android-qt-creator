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
#include "maemosettingspages.h"

#include "maemoqemusettings.h"
#include "maemoqemusettingswidget.h"

#include <coreplugin/icore.h>
#include <remotelinux/remotelinux_constants.h>

#include <QtCore/QCoreApplication>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QFrame>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QIcon>
#include <QtGui/QMainWindow>

namespace Madde {
namespace Internal {
namespace {

class MaemoQemuCrashDialog : public QDialog
{
    Q_OBJECT
public:
    MaemoQemuCrashDialog(QWidget *parent = 0) : QDialog(parent)
    {
        setWindowTitle(tr("Qemu error"));
        QString message = tr("Qemu crashed.") + QLatin1String(" <p>");
        const MaemoQemuSettings::OpenGlMode openGlMode
            = MaemoQemuSettings::openGlMode();
        const QString linkString = QLatin1String("</p><a href=\"dummy\">")
            + tr("Click here to change the OpenGL mode.")
            + QLatin1String("</a>");
        if (openGlMode == MaemoQemuSettings::HardwareAcceleration) {
            message += tr("You have configured Qemu to use OpenGL "
                "hardware acceleration, which might not be supported by "
                "your system. You could try using software rendering instead.");
            message += linkString;
        } else if (openGlMode == MaemoQemuSettings::AutoDetect) {
            message += tr("Qemu is currently configured to auto-detect the "
                "OpenGL mode, which is known to not work in some cases. "
                "You might want to use software rendering instead.");
            message += linkString;
        }
        QLabel * const messageLabel = new QLabel(message, this);
        messageLabel->setWordWrap(true);
        messageLabel->setTextFormat(Qt::RichText);
        connect(messageLabel, SIGNAL(linkActivated(QString)),
            SLOT(showSettingsPage()));
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(messageLabel);
        QFrame * const separator = new QFrame;
        separator->setFrameShape(QFrame::HLine);
        separator->setFrameShadow(QFrame::Sunken);
        mainLayout->addWidget(separator);
        QDialogButtonBox * const buttonBox = new QDialogButtonBox;
        buttonBox->addButton(QDialogButtonBox::Ok);
        connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        mainLayout->addWidget(buttonBox);
    }

private:
    Q_SLOT void showSettingsPage()
    {
        Core::ICore::instance()->showOptionsDialog(MaemoQemuSettingsPage::pageCategory(),
            MaemoQemuSettingsPage::pageId());
        accept();
    }
};

} // anonymous namespace


MaemoQemuSettingsPage::MaemoQemuSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
}

MaemoQemuSettingsPage::~MaemoQemuSettingsPage()
{
}

QString MaemoQemuSettingsPage::id() const
{
    return pageId();
}

QString MaemoQemuSettingsPage::displayName() const
{
    return tr("MeeGo Qemu Settings");
}

QString MaemoQemuSettingsPage::category() const
{
    return pageCategory();
}

QString MaemoQemuSettingsPage::displayCategory() const
{
    return QString(); // Already set by device configurations page.
}

QIcon MaemoQemuSettingsPage::categoryIcon() const
{
    return QIcon(); // See above.
}

bool MaemoQemuSettingsPage::matches(const QString &searchKeyWord) const
{
    return m_widget->keywords().contains(searchKeyWord, Qt::CaseInsensitive);
}

QWidget *MaemoQemuSettingsPage::createPage(QWidget *parent)
{
    m_widget = new MaemoQemuSettingsWidget(parent);
    return m_widget;
}

void MaemoQemuSettingsPage::apply()
{
    m_widget->saveSettings();
}

void MaemoQemuSettingsPage::finish()
{
}

void MaemoQemuSettingsPage::showQemuCrashDialog()
{
    MaemoQemuCrashDialog dlg(Core::ICore::instance()->mainWindow());
    dlg.exec();
}

QString MaemoQemuSettingsPage::pageId()
{
    return QLatin1String("ZZ.Qemu Settings");
}

QString MaemoQemuSettingsPage::pageCategory()
{
    return QLatin1String(RemoteLinux::Constants::RemoteLinuxSettingsCategory);
}

} // namespace Internal
} // namespace Madde

#include "maemosettingspages.moc"
