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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "settingspage.h"
#include "perforcesettings.h"
#include "perforceplugin.h"
#include "perforcechecker.h"

#include <vcsbase/vcsbaseconstants.h>

#include <QtGui/QApplication>
#include <QtGui/QLineEdit>
#include <QtGui/QFileDialog>
#include <QtCore/QTextStream>

using namespace Perforce::Internal;
using namespace Utils;

SettingsPageWidget::SettingsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.errorLabel->clear();
    m_ui.pathChooser->setPromptDialogTitle(tr("Perforce Command"));
    m_ui.pathChooser->setExpectedKind(PathChooser::Command);
    connect(m_ui.testPushButton, SIGNAL(clicked()), this, SLOT(slotTest()));
}

void SettingsPageWidget::slotTest()
{
    if (!m_checker) {
        m_checker = new PerforceChecker(this);
        m_checker->setUseOverideCursor(true);
        connect(m_checker.data(), SIGNAL(failed(QString)), this, SLOT(setStatusError(QString)));
        connect(m_checker.data(), SIGNAL(succeeded(QString)), this, SLOT(testSucceeded(QString)));
    }

    if (m_checker->isRunning())
        return;

    setStatusText(tr("Testing..."));
    const Settings s = settings();
    m_checker->start(s.p4Command, s.commonP4Arguments(), 10000);
}

void SettingsPageWidget::testSucceeded(const QString &repo)
{
    setStatusText(tr("Test succeeded (%1).").arg(QDir::toNativeSeparators(repo)));
}

Settings SettingsPageWidget::settings() const
{
    Settings  settings;
    settings.p4Command = m_ui.pathChooser->path();
    settings.defaultEnv = !m_ui.environmentGroupBox->isChecked();
    settings.p4Port = m_ui.portLineEdit->text();
    settings.p4User = m_ui.userLineEdit->text();
    settings.p4Client=  m_ui.clientLineEdit->text();
    settings.timeOutS = m_ui.timeOutSpinBox->value();
    settings.logCount = m_ui.logCountSpinBox->value();
    settings.promptToSubmit = m_ui.promptToSubmitCheckBox->isChecked();
    settings.autoOpen = m_ui.autoOpenCheckBox->isChecked();
    return settings;
}

void SettingsPageWidget::setSettings(const PerforceSettings &s)
{
    m_ui.pathChooser->setPath(s.p4Command());
    m_ui.environmentGroupBox->setChecked(!s.defaultEnv());
    m_ui.portLineEdit->setText(s.p4Port());
    m_ui.clientLineEdit->setText(s.p4Client());
    m_ui.userLineEdit->setText(s.p4User());
    m_ui.logCountSpinBox->setValue(s.logCount());
    m_ui.timeOutSpinBox->setValue(s.timeOutS());
    m_ui.promptToSubmitCheckBox->setChecked(s.promptToSubmit());
    m_ui.autoOpenCheckBox->setChecked(s.autoOpen());
}

void SettingsPageWidget::setStatusText(const QString &t)
{
    m_ui.errorLabel->setStyleSheet(QString());
    m_ui.errorLabel->setText(t);
}

void SettingsPageWidget::setStatusError(const QString &t)
{
    m_ui.errorLabel->setStyleSheet(QLatin1String("background-color: red"));
    m_ui.errorLabel->setText(t);
}

QString SettingsPageWidget::searchKeywords() const
{
    QString rc;
    QLatin1Char sep(' ');
    QTextStream(&rc)
            << sep << m_ui.configGroupBox->title()
            << sep << m_ui.commandLabel->text()
            << sep << m_ui.environmentGroupBox->title()
            << sep << m_ui.portLabel->text()
            << sep << m_ui.clientLabel->text()
            << sep << m_ui.userLabel->text()
            << sep << m_ui.miscGroupBox->title()
            << sep << m_ui.logCountLabel->text()
            << sep << m_ui.timeOutLabel->text()
            << sep << m_ui.promptToSubmitCheckBox->text()
            << sep << m_ui.autoOpenCheckBox->text()
               ;
    rc.remove(QLatin1Char('&'));
    return rc;
}

SettingsPage::SettingsPage()
{
}

QString SettingsPage::id() const
{
    return QLatin1String(VCSBase::Constants::VCS_ID_PERFORCE);
}

QString SettingsPage::displayName() const
{
    return tr("Perforce");
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    m_widget = new SettingsPageWidget(parent);
    m_widget->setSettings(PerforcePlugin::perforcePluginInstance()->settings());
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void SettingsPage::apply()
{
    PerforcePlugin::perforcePluginInstance()->setSettings(m_widget->settings());
}

bool SettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
