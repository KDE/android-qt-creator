/**************************************************************************
**
** This file is part of Qt Creator Instrumentation Tools
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "analyzerrunconfigwidget.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QComboBox>
#include <QtGui/QPushButton>

namespace Analyzer {
namespace Internal {

AnalyzerToolDetailWidget::AnalyzerToolDetailWidget(AbstractAnalyzerSubConfig *config, QWidget *parent)
    : Utils::DetailsWidget(parent)
{
    QTC_ASSERT(config!=0, return);

    // update summary text
    setSummaryText(tr("<strong>%1</strong> settings").arg(config->displayName()));

    // create config widget
    QWidget *configWidget = config->createConfigWidget(this);
    setWidget(configWidget);
}

AnalyzerRunConfigWidget::AnalyzerRunConfigWidget()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QWidget *globalSetting = new QWidget(this);
    QHBoxLayout *globalSettingLayout = new QHBoxLayout(globalSetting);
    globalSettingLayout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(globalSetting);
    QLabel *label = new QLabel(tr("Analyzer Settings:"), globalSetting);
    globalSettingLayout->addWidget(label);
    m_settingsCombo = new QComboBox(globalSetting);
    m_settingsCombo->addItems(QStringList()
                            << QApplication::translate("ProjectExplorer::Internal::EditorSettingsPropertiesPage", "Global")
                            << QApplication::translate("ProjectExplorer::Internal::EditorSettingsPropertiesPage", "Custom")
                            );
    globalSettingLayout->addWidget(m_settingsCombo);
    connect(m_settingsCombo, SIGNAL(activated(int)), this, SLOT(chooseSettings(int)));
    m_restoreButton = new QPushButton(
                QApplication::translate("ProjectExplorer::Internal::EditorSettingsPropertiesPage", "Restore Global"),
                globalSetting);
    globalSettingLayout->addWidget(m_restoreButton);
    connect(m_restoreButton, SIGNAL(clicked()), this, SLOT(restoreGlobal()));
    globalSettingLayout->addStretch(2);

    m_subConfigWidget = new QWidget(this);
    new QVBoxLayout(m_subConfigWidget);
    layout->addWidget(m_subConfigWidget);
}

QString AnalyzerRunConfigWidget::displayName() const
{
    return tr("Analyzer Settings");
}

void AnalyzerRunConfigWidget::setRunConfiguration(ProjectExplorer::RunConfiguration *rc)
{
    QTC_ASSERT(rc, return);

    m_settings = rc->extraAspect<AnalyzerProjectSettings>();
    QTC_ASSERT(m_settings, return);

    // add config widget for each sub config
    foreach (AbstractAnalyzerSubConfig *config, m_settings->customSubConfigs()) {
        QWidget *widget = new AnalyzerToolDetailWidget(config);
        m_subConfigWidget->layout()->addWidget(widget);
    }
    setDetailEnabled(!m_settings->isUsingGlobalSettings());
    m_settingsCombo->setCurrentIndex(m_settings->isUsingGlobalSettings() ? 0 : 1);
    m_restoreButton->setEnabled(!m_settings->isUsingGlobalSettings());
}

void AnalyzerRunConfigWidget::setDetailEnabled(bool value)
{
    QList<AnalyzerToolDetailWidget*> details = findChildren<AnalyzerToolDetailWidget*>();
    foreach (AnalyzerToolDetailWidget *detail, details)
        detail->widget()->setEnabled(value);
}

void AnalyzerRunConfigWidget::chooseSettings(int setting)
{
    QTC_ASSERT(m_settings, return);
    setDetailEnabled(setting != 0);
    m_settings->setUsingGlobalSettings(setting == 0);
    m_restoreButton->setEnabled(!m_settings->isUsingGlobalSettings());
}

void AnalyzerRunConfigWidget::restoreGlobal()
{
    QTC_ASSERT(m_settings, return);
    m_settings->resetCustomToGlobalSettings();
}

} // namespace Internal
} // namespace Analyzer
