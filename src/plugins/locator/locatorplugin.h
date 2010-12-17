/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef LOCATORPLUGIN_H
#define LOCATORPLUGIN_H

#include "ilocatorfilter.h"
#include "directoryfilter.h"

#include <extensionsystem/iplugin.h>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QFutureWatcher>

namespace Locator {
namespace Internal {

class LocatorWidget;
class OpenDocumentsFilter;
class FileSystemFilter;
class SettingsPage;
class LocatorPlugin;

class LocatorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    LocatorPlugin();
    ~LocatorPlugin();

    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();

    QList<ILocatorFilter*> filters();
    QList<ILocatorFilter*> customFilters();
    void setFilters(QList<ILocatorFilter*> f);
    void setCustomFilters(QList<ILocatorFilter*> f);
    int refreshInterval();
    void setRefreshInterval(int interval);

public slots:
    void refresh(QList<ILocatorFilter*> filters = QList<ILocatorFilter*>());
    void saveSettings();
    void openLocator();

private slots:
    void startSettingsLoad();
    void settingsLoaded();

private:
    void loadSettings();

    template <typename S>
    void loadSettingsHelper(S *settings);

    LocatorWidget *m_locatorWidget;
    SettingsPage *m_settingsPage;

    QList<ILocatorFilter*> m_filters;
    QList<ILocatorFilter*> m_customFilters;
    int m_refreshInterval;
    QTimer m_refreshTimer;
    OpenDocumentsFilter *m_openDocumentsFilter;
    FileSystemFilter *m_fileSystemFilter;
    QFutureWatcher<void> m_loadWatcher;
};

template <typename S>
void LocatorPlugin::loadSettingsHelper(S *settings)
{
    settings->beginGroup("QuickOpen");
    m_refreshTimer.setInterval(settings->value("RefreshInterval", 60).toInt() * 60000);

    foreach (ILocatorFilter *filter, m_filters) {
        if (settings->contains(filter->id())) {
            const QByteArray state = settings->value(filter->id()).toByteArray();
            if (!state.isEmpty())
                filter->restoreState(state);
        }
    }
    settings->beginGroup("CustomFilters");
    QList<ILocatorFilter *> customFilters;
    const QStringList keys = settings->childKeys();
    foreach (const QString &key, keys) {
        ILocatorFilter *filter = new DirectoryFilter;
        filter->restoreState(settings->value(key).toByteArray());
        m_filters.append(filter);
        customFilters.append(filter);
    }
    setCustomFilters(customFilters);
    settings->endGroup();
    settings->endGroup();
}

} // namespace Internal
} // namespace Locator

#endif // LOCATORPLUGIN_H
