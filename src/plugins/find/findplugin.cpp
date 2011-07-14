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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "findplugin.h"

#include "textfindconstants.h"
#include "currentdocumentfind.h"
#include "findtoolbar.h"
#include "findtoolwindow.h"
#include "searchresultwindow.h"
#include "ifindfilter.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/uniqueidmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QtGui/QMenu>
#include <QtGui/QStringListModel>
#include <QtGui/QAction>

#include <QtCore/QtPlugin>
#include <QtCore/QSettings>

/*!
    \namespace Find
    The Find namespace provides everything that has to do with search term based searches.
*/

/*!
    \namespace Find::Internal
    \internal
*/
/*!
    \namespace Find::Internal::ItemDataRoles
    \internal
*/

Q_DECLARE_METATYPE(Find::IFindFilter*)

namespace {
    const int MAX_COMPLETIONS = 50;
}

namespace Find {

struct FindPluginPrivate {
    explicit FindPluginPrivate(FindPlugin *q);

    //variables
    static FindPlugin *m_instance;

    QHash<IFindFilter *, QAction *> m_filterActions;

    Internal::CurrentDocumentFind *m_currentDocumentFind;
    Internal::FindToolBar *m_findToolBar;
    Internal::FindToolWindow *m_findDialog;
    Find::FindFlags m_findFlags;
    QStringListModel *m_findCompletionModel;
    QStringListModel *m_replaceCompletionModel;
    QStringList m_findCompletions;
    QStringList m_replaceCompletions;
    QAction *m_openFindDialog;
};

FindPluginPrivate::FindPluginPrivate(FindPlugin *q) :
    m_currentDocumentFind(0), m_findToolBar(0), m_findDialog(0),
    m_findCompletionModel(new QStringListModel(q)),
    m_replaceCompletionModel(new QStringListModel(q))
{
}

FindPlugin *FindPluginPrivate::m_instance = 0;

FindPlugin::FindPlugin() : d(new FindPluginPrivate(this))
{
    QTC_ASSERT(!FindPluginPrivate::m_instance, return);
    FindPluginPrivate::m_instance = this;
}

FindPlugin::~FindPlugin()
{
    FindPluginPrivate::m_instance = 0;
    delete d->m_currentDocumentFind;
    delete d->m_findToolBar;
    delete d->m_findDialog;
    delete d;
}

FindPlugin *FindPlugin::instance()
{
    return FindPluginPrivate::m_instance;
}

bool FindPlugin::initialize(const QStringList &, QString *)
{
    setupMenu();

    d->m_currentDocumentFind = new Internal::CurrentDocumentFind;

    d->m_findToolBar = new Internal::FindToolBar(this, d->m_currentDocumentFind);
    d->m_findDialog = new Internal::FindToolWindow(this);
    SearchResultWindow *searchResultWindow = new SearchResultWindow;
    addAutoReleasedObject(searchResultWindow);
    return true;
}

void FindPlugin::extensionsInitialized()
{
    setupFilterMenuItems();
    readSettings();
}

ExtensionSystem::IPlugin::ShutdownFlag FindPlugin::aboutToShutdown()
{
    d->m_findToolBar->setVisible(false);
    d->m_findToolBar->setParent(0);
    d->m_currentDocumentFind->removeConnections();
    writeSettings();
    return SynchronousShutdown;
}

void FindPlugin::filterChanged()
{
    IFindFilter *changedFilter = qobject_cast<IFindFilter *>(sender());
    QAction *action = d->m_filterActions.value(changedFilter);
    QTC_ASSERT(changedFilter, return);
    QTC_ASSERT(action, return);
    action->setEnabled(changedFilter->isEnabled());
    bool haveEnabledFilters = false;
    foreach (const IFindFilter *filter, d->m_filterActions.keys()) {
        if (filter->isEnabled()) {
            haveEnabledFilters = true;
            break;
        }
    }
    d->m_openFindDialog->setEnabled(haveEnabledFilters);
}

void FindPlugin::openFindFilter()
{
    QAction *action = qobject_cast<QAction*>(sender());
    QTC_ASSERT(action, return);
    IFindFilter *filter = action->data().value<IFindFilter *>();
    if (d->m_currentDocumentFind->candidateIsEnabled())
        d->m_currentDocumentFind->acceptCandidate();
    QString currentFindString = (d->m_currentDocumentFind->isEnabled() ? d->m_currentDocumentFind->currentFindString() : "");
    if (!currentFindString.isEmpty())
        d->m_findDialog->setFindText(currentFindString);
    d->m_findDialog->open(filter);
}

void FindPlugin::setupMenu()
{
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::ActionContainer *medit = am->actionContainer(Core::Constants::M_EDIT);
    Core::ActionContainer *mfind = am->createMenu(Constants::M_FIND);
    medit->addMenu(mfind, Core::Constants::G_EDIT_FIND);
    mfind->menu()->setTitle(tr("&Find/Replace"));
    mfind->appendGroup(Constants::G_FIND_CURRENTDOCUMENT);
    mfind->appendGroup(Constants::G_FIND_FILTERS);
    mfind->appendGroup(Constants::G_FIND_FLAGS);
    mfind->appendGroup(Constants::G_FIND_ACTIONS);
    Core::Context globalcontext(Core::Constants::C_GLOBAL);
    Core::Command *cmd;
    QAction *separator;
    separator = new QAction(this);
    separator->setSeparator(true);
    cmd = am->registerAction(separator, "Find.Sep.Flags", globalcontext);
    mfind->addAction(cmd, Constants::G_FIND_FLAGS);
    separator = new QAction(this);
    separator->setSeparator(true);
    cmd = am->registerAction(separator, "Find.Sep.Actions", globalcontext);
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);

    Core::ActionContainer *mfindadvanced = am->createMenu(Constants::M_FIND_ADVANCED);
    mfindadvanced->menu()->setTitle(tr("Advanced Find"));
    mfind->addMenu(mfindadvanced, Constants::G_FIND_FILTERS);
    d->m_openFindDialog = new QAction(tr("Open Advanced Find..."), this);
    d->m_openFindDialog->setIconText(tr("Advanced..."));
    cmd = am->registerAction(d->m_openFindDialog, Constants::ADVANCED_FIND, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+F")));
    mfindadvanced->addAction(cmd);
    connect(d->m_openFindDialog, SIGNAL(triggered()), this, SLOT(openFindFilter()));
}

void FindPlugin::setupFilterMenuItems()
{
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    QList<IFindFilter*> findInterfaces =
        ExtensionSystem::PluginManager::instance()->getObjects<IFindFilter>();
    Core::Command *cmd;
    Core::Context globalcontext(Core::Constants::C_GLOBAL);

    Core::ActionContainer *mfindadvanced = am->actionContainer(Constants::M_FIND_ADVANCED);
    d->m_filterActions.clear();
    bool haveEnabledFilters = false;
    foreach (IFindFilter *filter, findInterfaces) {
        QAction *action = new QAction(QLatin1String("    ") + filter->displayName(), this);
        bool isEnabled = filter->isEnabled();
        if (isEnabled)
            haveEnabledFilters = true;
        action->setEnabled(isEnabled);
        action->setData(qVariantFromValue(filter));
        cmd = am->registerAction(action, QString(QLatin1String("FindFilter.")+filter->id()), globalcontext);
        cmd->setDefaultKeySequence(filter->defaultShortcut());
        mfindadvanced->addAction(cmd);
        d->m_filterActions.insert(filter, action);
        connect(action, SIGNAL(triggered(bool)), this, SLOT(openFindFilter()));
        connect(filter, SIGNAL(changed()), this, SLOT(filterChanged()));
    }
    d->m_findDialog->setFindFilters(findInterfaces);
    d->m_openFindDialog->setEnabled(haveEnabledFilters);
}

Find::FindFlags FindPlugin::findFlags() const
{
    return d->m_findFlags;
}

void FindPlugin::setCaseSensitive(bool sensitive)
{
    setFindFlag(Find::FindCaseSensitively, sensitive);
}

void FindPlugin::setWholeWord(bool wholeOnly)
{
    setFindFlag(Find::FindWholeWords, wholeOnly);
}

void FindPlugin::setBackward(bool backward)
{
    setFindFlag(Find::FindBackward, backward);
}

void FindPlugin::setRegularExpression(bool regExp)
{
    setFindFlag(Find::FindRegularExpression, regExp);
}

void FindPlugin::setFindFlag(Find::FindFlag flag, bool enabled)
{
    bool hasFlag = hasFindFlag(flag);
    if ((hasFlag && enabled) || (!hasFlag && !enabled))
        return;
    if (enabled)
        d->m_findFlags |= flag;
    else
        d->m_findFlags &= ~flag;
    if (flag != Find::FindBackward)
        emit findFlagsChanged();
}

bool FindPlugin::hasFindFlag(Find::FindFlag flag)
{
    return d->m_findFlags & flag;
}

void FindPlugin::writeSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup("Find");
    settings->setValue("Backward", hasFindFlag(Find::FindBackward));
    settings->setValue("CaseSensitively", hasFindFlag(Find::FindCaseSensitively));
    settings->setValue("WholeWords", hasFindFlag(Find::FindWholeWords));
    settings->setValue("RegularExpression", hasFindFlag(Find::FindRegularExpression));
    settings->setValue("FindStrings", d->m_findCompletions);
    settings->setValue("ReplaceStrings", d->m_replaceCompletions);
    settings->endGroup();
    d->m_findToolBar->writeSettings();
    d->m_findDialog->writeSettings();
}

void FindPlugin::readSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup("Find");
    bool block = blockSignals(true);
    setBackward(settings->value("Backward", false).toBool());
    setCaseSensitive(settings->value("CaseSensitively", false).toBool());
    setWholeWord(settings->value("WholeWords", false).toBool());
    setRegularExpression(settings->value("RegularExpression", false).toBool());
    blockSignals(block);
    d->m_findCompletions = settings->value("FindStrings").toStringList();
    d->m_replaceCompletions = settings->value("ReplaceStrings").toStringList();
    d->m_findCompletionModel->setStringList(d->m_findCompletions);
    d->m_replaceCompletionModel->setStringList(d->m_replaceCompletions);
    settings->endGroup();
    d->m_findToolBar->readSettings();
    d->m_findDialog->readSettings();
    emit findFlagsChanged(); // would have been done in the setXXX methods above
}

void FindPlugin::updateFindCompletion(const QString &text)
{
    updateCompletion(text, d->m_findCompletions, d->m_findCompletionModel);
}

void FindPlugin::updateReplaceCompletion(const QString &text)
{
    updateCompletion(text, d->m_replaceCompletions, d->m_replaceCompletionModel);
}

void FindPlugin::updateCompletion(const QString &text, QStringList &completions, QStringListModel *model)
{
    if (text.isEmpty())
        return;
    completions.removeAll(text);
    completions.prepend(text);
    while (completions.size() > MAX_COMPLETIONS)
        completions.removeLast();
    model->setStringList(completions);
}

void FindPlugin::setUseFakeVim(bool on)
{
    if (d->m_findToolBar)
        d->m_findToolBar->setUseFakeVim(on);
}

void FindPlugin::openFindToolBar(FindDirection direction)
{
    if (d->m_findToolBar) {
        d->m_findToolBar->setBackward(direction == FindBackward);
        d->m_findToolBar->openFindToolBar();
    }
}

QStringListModel *FindPlugin::findCompletionModel() const
{
    return d->m_findCompletionModel;
}

QStringListModel *FindPlugin::replaceCompletionModel() const
{
    return d->m_replaceCompletionModel;
}

QKeySequence IFindFilter::defaultShortcut() const
{
    return QKeySequence();
}

} // namespace Find

// declared in textfindconstants.h
QTextDocument::FindFlags Find::textDocumentFlagsForFindFlags(Find::FindFlags flags)
{
    QTextDocument::FindFlags textDocFlags;
    if (flags & Find::FindBackward)
        textDocFlags |= QTextDocument::FindBackward;
    if (flags & Find::FindCaseSensitively)
        textDocFlags |= QTextDocument::FindCaseSensitively;
    if (flags & Find::FindWholeWords)
        textDocFlags |= QTextDocument::FindWholeWords;
    return textDocFlags;
}

Q_EXPORT_PLUGIN(Find::FindPlugin)
