/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "designersettings.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "settingspage.h"

#include <QtCore/QTextStream>
#include <QtGui/QCheckBox>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

SettingsPageWidget::SettingsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
}

DesignerSettings SettingsPageWidget::settings() const
{
    DesignerSettings ds;
    ds.itemSpacing = m_ui.spinItemSpacing->value();
    ds.snapMargin = m_ui.spinSnapMargin->value();
    return ds;
}

void SettingsPageWidget::setSettings(const DesignerSettings &s)
{
    m_ui.spinItemSpacing->setValue(s.itemSpacing);
    m_ui.spinSnapMargin->setValue(s.snapMargin);
}

QString SettingsPageWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc)
            << ' ' << m_ui.snapMarginLabel->text()
            << ' ' << m_ui.itemSpacingLabel->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

SettingsPage::SettingsPage() :
    m_widget(0)
{
}

QString SettingsPage::id() const
{
    return QLatin1String("QmlDesigner");
}

QString SettingsPage::displayName() const
{
    return tr("Qt Quick Designer");
}

QString SettingsPage::category() const
{
    return QLatin1String("Qt Quick");
}

QString SettingsPage::displayCategory() const
{
    return QCoreApplication::translate("Qt Quick", "Qt Quick");
}

QIcon SettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Constants::SETTINGS_CATEGORY_QML_ICON));
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    m_widget = new SettingsPageWidget(parent);
    m_widget->setSettings(BauhausPlugin::pluginInstance()->settings());
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void SettingsPage::apply()
{
    if (!m_widget) // page was never shown
        return;
    BauhausPlugin::pluginInstance()->setSettings(m_widget->settings());
}

bool SettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
