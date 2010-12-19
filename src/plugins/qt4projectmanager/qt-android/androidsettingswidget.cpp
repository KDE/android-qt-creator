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

#include "androidsettingswidget.h"

#include "ui_androidsettingswidget.h"

#include "androidconfigurations.h"

#include <coreplugin/ssh/sshremoteprocessrunner.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QIntValidator>

#include <algorithm>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

AndroidSettingsWidget::AndroidSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_ui(new Ui_AndroidSettingsWidget),
      m_androidConfig(AndroidConfigurations::instance().config()),
      m_saveSettingsRequested(false)
{
    initGui();
}

AndroidSettingsWidget::~AndroidSettingsWidget()
{
    if (m_saveSettingsRequested)
        AndroidConfigurations::instance().setConfig(m_androidConfig);
    delete m_ui;
}

QString AndroidSettingsWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << m_ui->SDKLocationLabel->text()
        << ' ' << m_ui->SDKLocationLineEdit->text()
        << ' ' << m_ui->NDKLocationLabel->text()
        << ' ' << m_ui->NDKLocationLineEdit->text()
        << ' ' << m_ui->AntLocationLabel->text()
        << ' ' << m_ui->AntLocationLineEdit->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

void AndroidSettingsWidget::initGui()
{
    m_ui->setupUi(this);
    if (checkSDK(m_androidConfig.SDKLocation))
        m_ui->SDKLocationLineEdit->setText(m_androidConfig.SDKLocation);
    else
        m_androidConfig.SDKLocation="";

    m_ui->NDKLocationLineEdit->setText(m_androidConfig.NDKLocation);
    m_ui->AntLocationLineEdit->setText(m_androidConfig.AntLocation);
}

void AndroidSettingsWidget::saveSettings()
{
    // We must defer this step because of a stupid bug on MacOS. See QTCREATORBUG-1675.
    m_saveSettingsRequested = true;
}

bool AndroidSettingsWidget::checkSDK(const QString & location)
{
    m_ui->devicesFrame->setEnabled(false);
    if (!location.length())
        return false;
    if (!QFile::exists(location+QLatin1String("/platform-tools/adb")) || !QFile::exists(location+QLatin1String("/tools/android")) || !QFile::exists(location+QLatin1String("/tools/emulator")) )
    {
        QMessageBox::critical(this, tr("Android SDK Folder"), tr("\"%1\" doesn't seem to be an Android SDK top folder").arg(location));
        return false;
    }
    m_ui->devicesFrame->setEnabled(true);
    return true;
}

bool AndroidSettingsWidget::checkNDK(const QString & location)
{
    if (!location.length())
        return false;
    return true;
    if (!QFile::exists(location+QLatin1String("/platforms")) || !QFile::exists(location+QLatin1String("/toolchains")) || !QFile::exists(location+QLatin1String("/sources/cxx-stl")) )
    {
        QMessageBox::critical(this, tr("Android SDK Folder"), tr("\"%1\" doesn't seem to be an Android NDK top folder'").arg(location));
        return false;
    }
    return true;

}

void AndroidSettingsWidget::SDKLocationEditingFinished()
{
    QString location=m_ui->SDKLocationLineEdit->text();
    if (!checkSDK(location))
        return;
    m_androidConfig.SDKLocation = location;
}

void AndroidSettingsWidget::NDKLocationEditingFinished()
{
    QString location=m_ui->NDKLocationLineEdit->text();
    if (!checkNDK(location))
        return;
    m_androidConfig.NDKLocation = location;
}

void AndroidSettingsWidget::AntLocationEditingFinished()
{
    QString location=m_ui->AntLocationLineEdit->text();
    if (!location.length() || !QFile::exists(location))
        return;
    m_androidConfig.AntLocation = location;
}

void AndroidSettingsWidget::browseSDKLocation()
{
    QString dir=QFileDialog::getExistingDirectory(this, tr("Select Android SDK folder"));
    if (!checkSDK(dir))
        return;
    m_ui->SDKLocationLineEdit->setText(dir);
    SDKLocationEditingFinished();
}

void AndroidSettingsWidget::browseNDKLocation()
{
    QString dir=QFileDialog::getExistingDirectory(this, tr("Select Android NDK folder"));
    if (!checkNDK(dir))
        return;
    m_ui->NDKLocationLineEdit->setText(dir);
    NDKLocationEditingFinished();
}

void AndroidSettingsWidget::browseAntLocation()
{
    QString dir=QDir::homePath();
#ifdef Q_OS_LINUX
    dir=QLatin1String("/usr/bin/ant");
#endif
    QString file=QFileDialog::getOpenFileName(this, tr("Select ant file"),dir,QLatin1String("ant"));
    if (!file.length())
        return;
    m_ui->AntLocationLineEdit->setText(file);
    AntLocationEditingFinished();
}

} // namespace Internal
} // namespace Qt4ProjectManager
