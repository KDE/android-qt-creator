/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidsettingspage.h"

#include "androidsettingswidget.h"

#include "androidconstants.h"

#include <QtCore/QCoreApplication>

namespace Qt4ProjectManager {
namespace Internal {

AndroidSettingsPage::AndroidSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
}

AndroidSettingsPage::~AndroidSettingsPage()
{
}

QString AndroidSettingsPage::id() const
{
    return QLatin1String("ZZ.Android Configurations");
}

QString AndroidSettingsPage::displayName() const
{
    return tr("Android Configurations");
}

QString AndroidSettingsPage::category() const
{
    return QLatin1String(Qt4ProjectManager::Constants::ANDROID_SETTINGS_CATEGORY);
}

QString AndroidSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("Android",
        Qt4ProjectManager::Constants::ANDROID_SETTINGS_TR_CATEGORY);
}

QIcon AndroidSettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Qt4ProjectManager::Constants::ANDROID_SETTINGS_CATEGORY_ICON));
}

bool AndroidSettingsPage::matches(const QString &searchKeyWord) const
{
    return m_keywords.contains(searchKeyWord, Qt::CaseInsensitive);
}

QWidget *AndroidSettingsPage::createPage(QWidget *parent)
{
    m_widget = new AndroidSettingsWidget(parent);
    if (m_keywords.isEmpty())
        m_keywords = m_widget->searchKeywords();
    return m_widget;
}

void AndroidSettingsPage::apply()
{
    m_widget->saveSettings();
}

void AndroidSettingsPage::finish()
{
}

} // namespace Internal
} // namespace Qt4ProjectManager
