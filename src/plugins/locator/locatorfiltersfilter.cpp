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

#include "locatorfiltersfilter.h"
#include "locatorplugin.h"
#include "locatorwidget.h"

#include <coreplugin/coreconstants.h>

using namespace Locator;
using namespace Locator::Internal;

Q_DECLARE_METATYPE(ILocatorFilter*)

LocatorFiltersFilter::LocatorFiltersFilter(LocatorPlugin *plugin,
                                               LocatorWidget *locatorWidget):
    m_plugin(plugin),
    m_locatorWidget(locatorWidget),
    m_icon(QIcon(Core::Constants::ICON_NEXT))
{
    setIncludedByDefault(true);
    setHidden(true);
}

QString LocatorFiltersFilter::displayName() const
{
    return tr("Available filters");
}

QString LocatorFiltersFilter::id() const
{
    return QLatin1String("FiltersFilter");
}

ILocatorFilter::Priority LocatorFiltersFilter::priority() const
{
    return High;
}

QList<FilterEntry> LocatorFiltersFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry)
{
    QList<FilterEntry> entries;
    if (!entry.isEmpty())
        return entries;

    QMap<QString, ILocatorFilter*> uniqueFilters;
    foreach (ILocatorFilter *filter, m_plugin->filters()) {
        const QString filterId = filter->shortcutString() + QLatin1Char(',') + filter->displayName();
        uniqueFilters.insert(filterId, filter);
    }

    foreach (ILocatorFilter *filter, uniqueFilters) {
        if (future.isCanceled())
            break;
        if (!filter->shortcutString().isEmpty() && !filter->isHidden() && filter->isEnabled()) {
            FilterEntry filterEntry(this,
                                    filter->shortcutString(),
                                    QVariant::fromValue(filter),
                                    m_icon);
            filterEntry.extraInfo = filter->displayName();
            entries.append(filterEntry);
        }
    }

    return entries;
}

void LocatorFiltersFilter::accept(FilterEntry selection) const
{
    ILocatorFilter *filter = selection.internalData.value<ILocatorFilter*>();
    if (filter)
        m_locatorWidget->show(filter->shortcutString() + QLatin1Char(' '),
                           filter->shortcutString().length() + 1);
}

void LocatorFiltersFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    // Nothing to refresh
}

bool LocatorFiltersFilter::isConfigurable() const
{
    return false;
}
