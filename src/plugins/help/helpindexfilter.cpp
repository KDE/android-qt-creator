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

#include "helpindexfilter.h"

#include "centralwidget.h"
#include "topicchooser.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <QtGui/QIcon>

using namespace Locator;
using namespace Help;
using namespace Help::Internal;

Q_DECLARE_METATYPE(ILocatorFilter*)

HelpIndexFilter::HelpIndexFilter()
{
    setIncludedByDefault(false);
    setShortcutString(QString(QLatin1Char('?')));
    m_icon = QIcon(QLatin1String(":/help/images/bookmark.png"));
}

HelpIndexFilter::~HelpIndexFilter()
{
}

QString HelpIndexFilter::displayName() const
{
    return tr("Help Index");
}

QString HelpIndexFilter::id() const
{
    return QLatin1String("HelpIndexFilter");
}

ILocatorFilter::Priority HelpIndexFilter::priority() const
{
    return Medium;
}

QList<FilterEntry> HelpIndexFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry)
{
    QStringList keywords;
    if (entry.length() < 2)
        keywords = Core::HelpManager::instance()->findKeywords(entry, 200);
    else
        keywords = Core::HelpManager::instance()->findKeywords(entry);

    QList<FilterEntry> entries;
    foreach (const QString &keyword, keywords) {
        if (future.isCanceled())
            break;
        entries.append(FilterEntry(this, keyword, QVariant(), m_icon));
    }

    return entries;
}

void HelpIndexFilter::accept(FilterEntry selection) const
{
    const QString &key = selection.displayName;
    const QMap<QString, QUrl> &links = Core::HelpManager::instance()->linksForKeyword(key);

    if (links.size() == 1) {
        emit linkActivated(links.begin().value());
    } else if (!links.isEmpty()) {
        TopicChooser tc(CentralWidget::instance(), key, links);
        if (tc.exec() == QDialog::Accepted)
            emit linkActivated(tc.link());
    }
}

void HelpIndexFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    // Nothing to refresh
}
