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

#include "androidconfigtestdialog.h"
#include "androiddeviceconfigurations.h"
#include "androidremoteprocessesdialog.h"
#include "androidsshconfigdialog.h"

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
        << ' ' << m_ui->NDKLocationLineEdit->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

void AndroidSettingsWidget::initGui()
{
    m_ui->setupUi(this);
}


void AndroidSettingsWidget::saveSettings()
{
    // We must defer this step because of a stupid bug on MacOS. See QTCREATORBUG-1675.
    m_saveSettingsRequested = true;
}

//AndroidConfig &AndroidSettingsWidget::currentConfig()
//{
//    return m_androidConfig;
//}


void AndroidSettingsWidget::SDKLocationEditingFinished()
{
    m_androidConfig.SDKLocation = m_ui->SDKLocationLineEdit->text();
}

void AndroidSettingsWidget::NDKLocationEditingFinished()
{
    m_androidConfig.NDKLocation = m_ui->NDKLocationLineEdit->text();
}


} // namespace Internal
} // namespace Qt4ProjectManager
