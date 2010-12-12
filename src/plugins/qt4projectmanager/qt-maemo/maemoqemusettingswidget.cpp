/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Assistant of the Qt Toolkit.
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "maemoqemusettingswidget.h"
#include "ui_maemoqemusettingswidget.h"

#include "maemoqemusettings.h"

namespace Qt4ProjectManager {
namespace Internal {

MaemoQemuSettingsWidget::MaemoQemuSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MaemoQemuSettingsWidget)
{
    ui->setupUi(this);
    switch (MaemoQemuSettings::openGlMode()) {
    case MaemoQemuSettings::HardwareAcceleration:
        ui->hardwareAccelerationButton->setChecked(true);
        break;
    case MaemoQemuSettings::SoftwareRendering:
        ui->softwareRenderingButton->setChecked(true);
        break;
    case MaemoQemuSettings::AutoDetect:
        ui->autoDetectButton->setChecked(true);
        break;
    }
}

MaemoQemuSettingsWidget::~MaemoQemuSettingsWidget()
{
    delete ui;
}

QString MaemoQemuSettingsWidget::keywords() const
{
    const QChar space = QLatin1Char(' ');
    QString keywords = ui->groupBox->title() + space
        + ui->hardwareAccelerationButton->text() + space
        + ui->softwareRenderingButton->text() + space
        + ui->autoDetectButton->text();
    keywords.remove(QLatin1Char('&'));
    return keywords;
}

void MaemoQemuSettingsWidget::saveSettings()
{
    const MaemoQemuSettings::OpenGlMode openGlMode
        = ui->hardwareAccelerationButton->isChecked()
            ? MaemoQemuSettings::HardwareAcceleration
            : ui->softwareRenderingButton->isChecked()
                  ? MaemoQemuSettings::SoftwareRendering
                  : MaemoQemuSettings::AutoDetect;
    MaemoQemuSettings::setOpenGlMode(openGlMode);
}

} // namespace Internal
} // namespace Qt4ProjectManager
