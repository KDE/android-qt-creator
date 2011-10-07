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

#include "mobileappwizardpages.h"
#include "ui_mobileappwizardgenericoptionspage.h"
#include "ui_mobileappwizardmaemooptionspage.h"
#include "ui_mobileappwizardharmattanoptionspage.h"
#include "ui_mobileappwizardsymbianoptionspage.h"
#include <coreplugin/coreconstants.h>
#include <utils/fileutils.h>

#include <QtCore/QTemporaryFile>
#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

class MobileAppWizardGenericOptionsPagePrivate
{
    Ui::MobileAppWizardGenericOptionsPage ui;
    friend class MobileAppWizardGenericOptionsPage;
};

class MobileAppWizardSymbianOptionsPagePrivate
{
    Ui::MobileAppWizardSymbianOptionsPage ui;
    QString svgIcon;
    friend class MobileAppWizardSymbianOptionsPage;
};

class MobileAppWizardMaemoOptionsPagePrivate
{
    Ui::MobileAppWizardMaemoOptionsPage ui;
    QSize iconSize;
    QString pngIcon;
    friend class MobileAppWizardMaemoOptionsPage;
};

class MobileAppWizardHarmattanOptionsPagePrivate
{
    Ui::MobileAppWizardHarmattanOptionsPage ui;
    QSize iconSize;
    QString pngIcon;
    friend class MobileAppWizardHarmattanOptionsPage;
};

MobileAppWizardGenericOptionsPage::MobileAppWizardGenericOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new MobileAppWizardGenericOptionsPagePrivate)
{
    m_d->ui.setupUi(this);
    m_d->ui.orientationBehaviorComboBox->addItem(tr("Automatically Rotate Orientation"),
        AbstractMobileApp::ScreenOrientationAuto);
    m_d->ui.orientationBehaviorComboBox->addItem(tr("Lock to Landscape Orientation"),
        AbstractMobileApp::ScreenOrientationLockLandscape);
    m_d->ui.orientationBehaviorComboBox->addItem(tr("Lock to Portrait Orientation"),
        AbstractMobileApp::ScreenOrientationLockPortrait);
}

MobileAppWizardGenericOptionsPage::~MobileAppWizardGenericOptionsPage()
{
    delete m_d;
}

void MobileAppWizardGenericOptionsPage::setOrientation(AbstractMobileApp::ScreenOrientation orientation)
{
    QComboBox *const comboBox = m_d->ui.orientationBehaviorComboBox;
    for (int i = 0; i < comboBox->count(); ++i) {
        if (comboBox->itemData(i).toInt() == static_cast<int>(orientation)) {
            comboBox->setCurrentIndex(i);
            break;
        }
    }
}

AbstractMobileApp::ScreenOrientation MobileAppWizardGenericOptionsPage::orientation() const
{
    QComboBox *const comboBox = m_d->ui.orientationBehaviorComboBox;
    const int index = comboBox->currentIndex();
    return static_cast<AbstractMobileApp::ScreenOrientation>(comboBox->itemData(index).toInt());
}


MobileAppWizardSymbianOptionsPage::MobileAppWizardSymbianOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new MobileAppWizardSymbianOptionsPagePrivate)
{
    m_d->ui.setupUi(this);
    const QIcon open = QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon);
    m_d->ui.appIconLoadToolButton->setIcon(open);
    connect(m_d->ui.appIconLoadToolButton, SIGNAL(clicked()), SLOT(openSvgIcon()));
}

MobileAppWizardSymbianOptionsPage::~MobileAppWizardSymbianOptionsPage()
{
    delete m_d;
}

QString MobileAppWizardSymbianOptionsPage::svgIcon() const
{
    return m_d->svgIcon;
}

void MobileAppWizardSymbianOptionsPage::setSvgIcon(const QString &icon)
{
    QPixmap iconPixmap(icon);
    if (!iconPixmap.isNull()) {
        const int symbianIconSize = 44;
        if (iconPixmap.height() > symbianIconSize || iconPixmap.width() > symbianIconSize)
            iconPixmap = iconPixmap.scaledToHeight(symbianIconSize, Qt::SmoothTransformation);
        m_d->ui.appIconPreview->setPixmap(iconPixmap);
        m_d->svgIcon = icon;
    }
}

QString MobileAppWizardSymbianOptionsPage::symbianUid() const
{
    return m_d->ui.uid3LineEdit->text();
}

void MobileAppWizardSymbianOptionsPage::setSymbianUid(const QString &uid)
{
    m_d->ui.uid3LineEdit->setText(uid);
}

void MobileAppWizardSymbianOptionsPage::setNetworkEnabled(bool enableIt)
{
    m_d->ui.enableNetworkCheckBox->setChecked(enableIt);
}

bool MobileAppWizardSymbianOptionsPage::networkEnabled() const
{
    return m_d->ui.enableNetworkCheckBox->isChecked();
}

void MobileAppWizardSymbianOptionsPage::openSvgIcon()
{
    const QString svgIcon = QFileDialog::getOpenFileName(
            this,
            m_d->ui.appIconLabel->text(),
            QDesktopServices::storageLocation(QDesktopServices::PicturesLocation),
            QLatin1String("*.svg"));
    if (!svgIcon.isEmpty())
        setSvgIcon(svgIcon);
}

MobileAppWizardMaemoOptionsPage::MobileAppWizardMaemoOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new MobileAppWizardMaemoOptionsPagePrivate)
{
    m_d->ui.setupUi(this);
    m_d->iconSize = QSize(64, 64);
    m_d->ui.pngIconButton->setIconSize(m_d->iconSize);
    connect(m_d->ui.pngIconButton, SIGNAL(clicked()), this, SLOT(openPngIcon()));
}

MobileAppWizardMaemoOptionsPage::~MobileAppWizardMaemoOptionsPage()
{
    delete m_d;
}

QString MobileAppWizardMaemoOptionsPage::pngIcon() const
{
    return m_d->pngIcon;
}


class PngIconScaler : public QObject
{
    Q_OBJECT
public:
    PngIconScaler(const QSize &expectedSize, const QString &iconPath)
        : m_expectedSize(expectedSize)
        , m_iconPath(iconPath)
        , m_pixmap(iconPath)
    {
    }

    bool hasRightSize() const { return m_expectedSize == m_pixmap.size(); }
    QPixmap pixmap() const { return m_pixmap; }

    bool scale(QString *newPath) {
        const QMessageBox::StandardButton button
                = QMessageBox::warning(QApplication::activeWindow(),
                                       tr("Wrong Icon Size"),
                                       tr("The icon needs to be %1x%2 pixels big, "
                                          "but is not. Do you want Qt Creator to scale it?")
                                       .arg(m_expectedSize.width()).arg(m_expectedSize.height()),
                                       QMessageBox::Ok | QMessageBox::Cancel);
        if (button != QMessageBox::Ok)
            return false;

        m_pixmap = m_pixmap.scaled(m_expectedSize);
        Utils::TempFileSaver saver;
        saver.setAutoRemove(false);
        if (!saver.hasError())
            saver.setResult(m_pixmap.save(
                                saver.file(), QFileInfo(m_iconPath).suffix().toAscii().constData()));
        if (!saver.finalize()) {
            QMessageBox::critical(QApplication::activeWindow(),
                                  tr("File Error"),
                                  tr("Could not copy icon file: %1").arg(saver.errorString()));
            return false;
        }
        *newPath = saver.fileName();
        return true;
    }
private:
    QSize m_expectedSize;
    QString m_iconPath;
    QPixmap m_pixmap;
};


void MobileAppWizardMaemoOptionsPage::setPngIcon(const QString &icon)
{
    QString actualIconPath;
    PngIconScaler scaler(m_d->iconSize, icon);
    if (scaler.hasRightSize()) {
        actualIconPath = icon;
    } else {
        if (!scaler.scale(&actualIconPath))
            return;
    }

    m_d->ui.pngIconButton->setIcon(scaler.pixmap());
    m_d->pngIcon = actualIconPath;
}

void MobileAppWizardMaemoOptionsPage::openPngIcon()
{
    const QString iconPath = QFileDialog::getOpenFileName(this,
        m_d->ui.appIconLabel->text(), m_d->pngIcon,
        QLatin1String("*.png"));
    if (!iconPath.isEmpty())
        setPngIcon(iconPath);
}

MobileAppWizardHarmattanOptionsPage::MobileAppWizardHarmattanOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new MobileAppWizardHarmattanOptionsPagePrivate)
{
    m_d->ui.setupUi(this);
    m_d->iconSize = QSize(80, 80);
    m_d->ui.pngIconButton->setIconSize(m_d->iconSize);
    connect(m_d->ui.pngIconButton, SIGNAL(clicked()), this, SLOT(openPngIcon()));
}

MobileAppWizardHarmattanOptionsPage::~MobileAppWizardHarmattanOptionsPage()
{
    delete m_d;
}

QString MobileAppWizardHarmattanOptionsPage::pngIcon() const
{
    return m_d->pngIcon;
}

void MobileAppWizardHarmattanOptionsPage::setPngIcon(const QString &icon)
{
    QString actualIconPath;
    PngIconScaler scaler(m_d->iconSize, icon);
    if (scaler.hasRightSize()) {
        actualIconPath = icon;
    } else {
        if (!scaler.scale(&actualIconPath))
            return;
    }

    m_d->ui.pngIconButton->setIcon(scaler.pixmap());
    m_d->pngIcon = actualIconPath;
}

void MobileAppWizardHarmattanOptionsPage::openPngIcon()
{
    const QString iconPath = QFileDialog::getOpenFileName(this,
        m_d->ui.appIconLabel->text(), m_d->pngIcon,
        QLatin1String("*.png"));
    if (!iconPath.isEmpty())
        setPngIcon(iconPath);
}

void MobileAppWizardHarmattanOptionsPage::setBoosterOptionEnabled(bool enable)
{
    m_d->ui.makeBoostableCheckBox->setEnabled(enable);
    m_d->ui.makeBoostableCheckBox->setChecked(enable);
}

bool MobileAppWizardHarmattanOptionsPage::supportsBooster() const
{
    return m_d->ui.makeBoostableCheckBox->isChecked();
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "mobileappwizardpages.moc"
