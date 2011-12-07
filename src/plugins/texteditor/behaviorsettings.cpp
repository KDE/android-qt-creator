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

#include "behaviorsettings.h"

#include <utils/settingsutils.h>

#include <QtCore/QSettings>
#include <QtCore/QString>

static const char mouseNavigationKey[] = "MouseNavigation";
static const char scrollWheelZoomingKey[] = "ScrollWheelZooming";
static const char constrainTooltips[] = "ConstrainTooltips";
static const char camelCaseNavigationKey[] = "CamelCaseNavigation";
static const char groupPostfix[] = "BehaviorSettings";

namespace TextEditor {

BehaviorSettings::BehaviorSettings() :
    m_mouseNavigation(true),
    m_scrollWheelZooming(true),
    m_constrainTooltips(false),
    m_camelCaseNavigation(true)
{
}

void BehaviorSettings::toSettings(const QString &category, QSettings *s) const
{
    Utils::toSettings(QLatin1String(groupPostfix), category, s, this);
}

void BehaviorSettings::fromSettings(const QString &category, const QSettings *s)
{
    *this = BehaviorSettings();
    Utils::fromSettings(QLatin1String(groupPostfix), category, s, this);
}

void BehaviorSettings::toMap(const QString &prefix, QVariantMap *map) const
{
    map->insert(prefix + QLatin1String(mouseNavigationKey), m_mouseNavigation);
    map->insert(prefix + QLatin1String(scrollWheelZoomingKey), m_scrollWheelZooming);
    map->insert(prefix + QLatin1String(constrainTooltips), m_constrainTooltips);
    map->insert(prefix + QLatin1String(camelCaseNavigationKey), m_camelCaseNavigation);
}

void BehaviorSettings::fromMap(const QString &prefix, const QVariantMap &map)
{
    m_mouseNavigation =
        map.value(prefix + QLatin1String(mouseNavigationKey), m_mouseNavigation).toBool();
    m_scrollWheelZooming =
        map.value(prefix + QLatin1String(scrollWheelZoomingKey), m_scrollWheelZooming).toBool();
    m_constrainTooltips =
        map.value(prefix + QLatin1String(constrainTooltips), m_constrainTooltips).toBool();
    m_camelCaseNavigation =
        map.value(prefix + QLatin1String(camelCaseNavigationKey), m_camelCaseNavigation).toBool();
}

bool BehaviorSettings::equals(const BehaviorSettings &ds) const
{
    return m_mouseNavigation == ds.m_mouseNavigation
        && m_scrollWheelZooming == ds.m_scrollWheelZooming
        && m_constrainTooltips == ds.m_constrainTooltips
        && m_camelCaseNavigation == ds.m_camelCaseNavigation
        ;
}

} // namespace TextEditor

