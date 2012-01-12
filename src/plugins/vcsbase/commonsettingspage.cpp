/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "commonsettingspage.h"
#include "vcsbaseconstants.h"
#include "nicknamedialog.h"

#include "ui_commonsettingspage.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <QtGui/QMessageBox>

namespace VcsBase {
namespace Internal {

// ------------------ VcsBaseSettingsWidget

CommonSettingsWidget::CommonSettingsWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::CommonSettingsPage)
{
    m_ui->setupUi(this);
    m_ui->submitMessageCheckScriptChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui->nickNameFieldsFileChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui->nickNameMailMapChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui->sshPromptChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    const QString patchToolTip = tr("Command used for reverting diff chunks");
    m_ui->patchCommandLabel->setToolTip(patchToolTip);
    m_ui->patchChooser->setToolTip(patchToolTip);
    m_ui->patchChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
}

CommonSettingsWidget::~CommonSettingsWidget()
{
    delete m_ui;
}

CommonVcsSettings CommonSettingsWidget::settings() const
{
    CommonVcsSettings rc;
    rc.nickNameMailMap = m_ui->nickNameMailMapChooser->path();
    rc.nickNameFieldListFile = m_ui->nickNameFieldsFileChooser->path();
    rc.submitMessageCheckScript = m_ui->submitMessageCheckScriptChooser->path();
    rc.lineWrap= m_ui->lineWrapCheckBox->isChecked();
    rc.lineWrapWidth = m_ui->lineWrapSpinBox->value();
    rc.sshPasswordPrompt = m_ui->sshPromptChooser->path();
    rc.patchCommand = m_ui->patchChooser->path();
    return rc;
}

void CommonSettingsWidget::setSettings(const CommonVcsSettings &s)
{
    m_ui->nickNameMailMapChooser->setPath(s.nickNameMailMap);
    m_ui->nickNameFieldsFileChooser->setPath(s.nickNameFieldListFile);
    m_ui->submitMessageCheckScriptChooser->setPath(s.submitMessageCheckScript);
    m_ui->lineWrapCheckBox->setChecked(s.lineWrap);
    m_ui->lineWrapSpinBox->setValue(s.lineWrapWidth);
    m_ui->sshPromptChooser->setPath(s.sshPasswordPrompt);
    m_ui->patchChooser->setPath(s.patchCommand);
}

QString CommonSettingsWidget::searchKeyWordMatchString() const
{
    const QChar blank = QLatin1Char(' ');
    QString rc = m_ui->lineWrapCheckBox->text()
            + blank + m_ui->submitMessageCheckScriptLabel->text()
            + blank + m_ui->nickNameMailMapLabel->text()
            + blank + m_ui->nickNameFieldsFileLabel->text()
            + blank + m_ui->sshPromptLabel->text()
            ;
    rc.remove(QLatin1Char('&')); // Strip buddy markers.
    return rc;
}

// --------------- VcsBaseSettingsPage
CommonOptionsPage::CommonOptionsPage(QObject *parent) :
    VcsBaseOptionsPage(parent)
{
    m_settings.fromSettings(Core::ICore::instance()->settings());
}

void CommonOptionsPage::updateNickNames()
{
}

QString CommonOptionsPage::id() const
{
    return QLatin1String(Constants::VCS_COMMON_SETTINGS_ID);
}

QString CommonOptionsPage::displayName() const
{
    return QCoreApplication::translate("VcsBase", Constants::VCS_COMMON_SETTINGS_NAME);
}

QWidget *CommonOptionsPage::createPage(QWidget *parent)
{
    m_widget = new CommonSettingsWidget(parent);
    m_widget->setSettings(m_settings);
    if (m_searchKeyWords.isEmpty())
        m_searchKeyWords = m_widget->searchKeyWordMatchString();
    return m_widget;
}

void CommonOptionsPage::apply()
{
    if (m_widget) {
        const CommonVcsSettings newSettings = m_widget->settings();
        if (newSettings != m_settings) {
            m_settings = newSettings;
            m_settings.toSettings(Core::ICore::instance()->settings());
            emit settingsChanged(m_settings);
        }
    }
}

bool CommonOptionsPage::matches(const QString &key) const
{
    return m_searchKeyWords.contains(key, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace VcsBase
