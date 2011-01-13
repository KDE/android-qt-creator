/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "locatorplugin.h"
#include "locatorconstants.h"
#include "locatorfiltersfilter.h"
#include "locatormanager.h"
#include "locatorwidget.h"
#include "opendocumentsfilter.h"
#include "filesystemfilter.h"
#include "settingspage.h"

#include <coreplugin/statusbarwidget.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/settingsdatabase.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/uniqueidmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <qtconcurrent/QtConcurrentTools>

#include <QtCore/QSettings>
#include <QtCore/QtPlugin>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtGui/QAction>

/*!
    \namespace Locator
    The Locator namespace provides the hooks for Locator content.
*/
/*!
    \namespace Locator::Internal
    \internal
*/

using namespace Locator;
using namespace Locator::Internal;

namespace {
    static bool filterLessThan(const ILocatorFilter *first, const ILocatorFilter *second)
    {
        if (first->priority() < second->priority())
            return true;
        if (first->priority() > second->priority())
            return false;
        if (first->id().compare(second->id(), Qt::CaseInsensitive) < 0)
            return true;
        return false;
    }
}

LocatorPlugin::LocatorPlugin()
{
    m_refreshTimer.setSingleShot(false);
    connect(&m_refreshTimer, SIGNAL(timeout()), this, SLOT(refresh()));
}

LocatorPlugin::~LocatorPlugin()
{
    removeObject(m_openDocumentsFilter);
    removeObject(m_fileSystemFilter);
    removeObject(m_settingsPage);
    delete m_openDocumentsFilter;
    delete m_fileSystemFilter;
    delete m_settingsPage;
    qDeleteAll(m_customFilters);
}

bool LocatorPlugin::initialize(const QStringList &, QString *)
{
    Core::ICore *core = Core::ICore::instance();
    m_settingsPage = new SettingsPage(this);
    addObject(m_settingsPage);

    m_locatorWidget = new LocatorWidget(this);
    m_locatorWidget->setEnabled(false);
    Core::StatusBarWidget *view = new Core::StatusBarWidget;
    view->setWidget(m_locatorWidget);
    view->setContext(Core::Context("LocatorWidget"));
    view->setPosition(Core::StatusBarWidget::First);
    addAutoReleasedObject(view);

    QAction *action = new QAction(m_locatorWidget->windowIcon(), m_locatorWidget->windowTitle(), this);
    Core::Command *cmd = core->actionManager()
        ->registerAction(action, "QtCreator.Locate", Core::Context(Core::Constants::C_GLOBAL));
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+K"));
    connect(action, SIGNAL(triggered()), this, SLOT(openLocator()));

    Core::ActionContainer *mtools = core->actionManager()->actionContainer(Core::Constants::M_TOOLS);
    mtools->addAction(cmd);

    addObject(new LocatorManager(m_locatorWidget));

    m_openDocumentsFilter = new OpenDocumentsFilter(core->editorManager());
    addObject(m_openDocumentsFilter);

    m_fileSystemFilter = new FileSystemFilter(core->editorManager(), m_locatorWidget);
    addObject(m_fileSystemFilter);

    addAutoReleasedObject(new LocatorFiltersFilter(this, m_locatorWidget));

    connect(core, SIGNAL(coreOpened()), this, SLOT(startSettingsLoad()));
    return true;
}

void LocatorPlugin::openLocator()
{
    m_locatorWidget->show("");
}

void LocatorPlugin::extensionsInitialized()
{
    m_filters = ExtensionSystem::PluginManager::instance()->getObjects<ILocatorFilter>();
    qSort(m_filters.begin(), m_filters.end(), filterLessThan);
    setFilters(m_filters);
}

void LocatorPlugin::startSettingsLoad()
{
    connect(&m_loadWatcher, SIGNAL(finished()), this, SLOT(settingsLoaded()));
    m_loadWatcher.setFuture(QtConcurrent::run(this, &LocatorPlugin::loadSettings));
}

void LocatorPlugin::loadSettings()
{
    Core::ICore *core = Core::ICore::instance();
    QSettings *qs = core->settings();

    // Backwards compatibility to old settings location
    if (qs->contains("QuickOpen/FiltersFilter")) {
        loadSettingsHelper(qs);
    } else {
        Core::SettingsDatabase *settings = core->settingsDatabase();
        loadSettingsHelper(settings);
    }

    qs->remove("QuickOpen");
}

void LocatorPlugin::settingsLoaded()
{
    m_locatorWidget->updateFilterList();
    m_locatorWidget->setEnabled(true);
    if (m_refreshTimer.interval() > 0)
        m_refreshTimer.start();
}

void LocatorPlugin::saveSettings()
{
    Core::ICore *core = Core::ICore::instance();
    if (core && core->settingsDatabase()) {
        Core::SettingsDatabase *s = core->settingsDatabase();
        s->beginGroup("QuickOpen");
        s->remove("");
        s->setValue("RefreshInterval", refreshInterval());
        foreach (ILocatorFilter *filter, m_filters) {
            if (!m_customFilters.contains(filter))
                s->setValue(filter->id(), filter->saveState());
        }
        s->beginGroup("CustomFilters");
        int i = 0;
        foreach (ILocatorFilter *filter, m_customFilters) {
            s->setValue(QString("directory%1").arg(i), filter->saveState());
            ++i;
        }
        s->endGroup();
        s->endGroup();
    }
}

/*!
    \fn QList<ILocatorFilter*> LocatorPlugin::filters()

    Return all filters, including the ones created by the user.
*/
QList<ILocatorFilter*> LocatorPlugin::filters()
{
    return m_filters;
}

/*!
    \fn QList<ILocatorFilter*> LocatorPlugin::customFilters()

    This returns a subset of all the filters, that contains only the filters that
    have been created by the user at some point (maybe in a previous session).
 */
QList<ILocatorFilter*> LocatorPlugin::customFilters()
{
    return m_customFilters;
}

void LocatorPlugin::setFilters(QList<ILocatorFilter*> f)
{
    m_filters = f;
    m_locatorWidget->updateFilterList();
}

void LocatorPlugin::setCustomFilters(QList<ILocatorFilter *> filters)
{
    m_customFilters = filters;
}

int LocatorPlugin::refreshInterval()
{
    return m_refreshTimer.interval() / 60000;
}

void LocatorPlugin::setRefreshInterval(int interval)
{
    if (interval < 1) {
        m_refreshTimer.stop();
        m_refreshTimer.setInterval(0);
        return;
    }
    m_refreshTimer.setInterval(interval * 60000);
    m_refreshTimer.start();
}

void LocatorPlugin::refresh(QList<ILocatorFilter*> filters)
{
    if (filters.isEmpty())
        filters = m_filters;
    QFuture<void> task = QtConcurrent::run(&ILocatorFilter::refresh, filters);
    Core::FutureProgress *progress = Core::ICore::instance()
            ->progressManager()->addTask(task, tr("Indexing"),
                                         Locator::Constants::TASK_INDEX);
    connect(progress, SIGNAL(finished()), this, SLOT(saveSettings()));
}

Q_EXPORT_PLUGIN(LocatorPlugin)
