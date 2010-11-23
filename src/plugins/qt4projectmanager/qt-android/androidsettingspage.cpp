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

#include "androidsettingspage.h"

#include "androidsettingswidget.h"

#include <projectexplorer/projectexplorerconstants.h>

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
    using namespace ProjectExplorer;
    return QLatin1String(Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
}

QString AndroidSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("ProjectExplorer",
        ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
}

QIcon AndroidSettingsPage::categoryIcon() const
{
    using namespace ProjectExplorer;
    return QIcon(QLatin1String(Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
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
