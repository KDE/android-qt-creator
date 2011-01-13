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

#include "navigationwidget.h"
#include "navigationsubwidget.h"
#include "icore.h"
#include "icontext.h"
#include "coreconstants.h"
#include "inavigationwidgetfactory.h"
#include "modemanager.h"
#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"
#include "uniqueidmanager.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/styledbar.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <QtGui/QAction>
#include <QtGui/QHBoxLayout>
#include <QtGui/QResizeEvent>
#include <QtGui/QToolButton>
#include <QtGui/QShortcut>
#include <QtGui/QStandardItemModel>

Q_DECLARE_METATYPE(Core::INavigationWidgetFactory *)

namespace Core {

NavigationWidgetPlaceHolder *NavigationWidgetPlaceHolder::m_current = 0;

NavigationWidgetPlaceHolder* NavigationWidgetPlaceHolder::current()
{
    return m_current;
}

NavigationWidgetPlaceHolder::NavigationWidgetPlaceHolder(Core::IMode *mode, QWidget *parent)
    :QWidget(parent), m_mode(mode)
{
    setLayout(new QVBoxLayout);
    layout()->setMargin(0);
    connect(Core::ModeManager::instance(), SIGNAL(currentModeAboutToChange(Core::IMode *)),
            this, SLOT(currentModeAboutToChange(Core::IMode *)));
}

NavigationWidgetPlaceHolder::~NavigationWidgetPlaceHolder()
{
    if (m_current == this) {
        NavigationWidget::instance()->setParent(0);
        NavigationWidget::instance()->hide();
    }
}

void NavigationWidgetPlaceHolder::applyStoredSize(int width)
{
    if (width) {
        QSplitter *splitter = qobject_cast<QSplitter *>(parentWidget());
        if (splitter) {
            // A splitter we need to resize the splitter sizes
            QList<int> sizes = splitter->sizes();
            int index = splitter->indexOf(this);
            int diff = width - sizes.at(index);
            int adjust = sizes.count() > 1 ? (diff / (sizes.count() - 1)) : 0;
            for (int i = 0; i < sizes.count(); ++i) {
                if (i != index)
                    sizes[i] += adjust;
            }
            sizes[index]= width;
            splitter->setSizes(sizes);
        } else {
            QSize s = size();
            s.setWidth(width);
            resize(s);
        }
    }
}

// This function does work even though the order in which
// the placeHolder get the signal is undefined.
// It does ensure that after all PlaceHolders got the signal
// m_current points to the current PlaceHolder, or zero if there
// is no PlaceHolder in this mode
// And that the parent of the NavigationWidget gets the correct parent
void NavigationWidgetPlaceHolder::currentModeAboutToChange(Core::IMode *mode)
{
    NavigationWidget *navigationWidget = NavigationWidget::instance();

    if (m_current == this) {
        m_current = 0;
        navigationWidget->setParent(0);
        navigationWidget->hide();
        navigationWidget->placeHolderChanged(m_current);
    }
    if (m_mode == mode) {
        m_current = this;

        int width = navigationWidget->storedWidth();

        layout()->addWidget(navigationWidget);
        navigationWidget->show();

        applyStoredSize(width);
        setVisible(navigationWidget->isShown());
        navigationWidget->placeHolderChanged(m_current);
    }
}

struct NavigationWidgetPrivate {
    explicit NavigationWidgetPrivate(QAction *toggleSideBarAction);

    QList<Internal::NavigationSubWidget *> m_subWidgets;
    QHash<QShortcut *, QString> m_shortcutMap;
    QHash<QString, Core::Command*> m_commandMap;
    QStandardItemModel *m_factoryModel;

    bool m_shown;
    bool m_suppressed;
    int m_width;
    static NavigationWidget* m_instance;
    QAction *m_toggleSideBarAction;
};

NavigationWidgetPrivate::NavigationWidgetPrivate(QAction *toggleSideBarAction) :
    m_factoryModel(new QStandardItemModel),
    m_shown(true),
    m_suppressed(false),
    m_width(0),
    m_toggleSideBarAction(toggleSideBarAction)
{
}

NavigationWidget *NavigationWidgetPrivate::m_instance = 0;

NavigationWidget::NavigationWidget(QAction *toggleSideBarAction) :
    d(new NavigationWidgetPrivate(toggleSideBarAction))
{
    d->m_factoryModel->setSortRole(FactoryPriorityRole);
    setOrientation(Qt::Vertical);
    d->m_instance = this;
}

NavigationWidget::~NavigationWidget()
{
    NavigationWidgetPrivate::m_instance = 0;
}

NavigationWidget *NavigationWidget::instance()
{
    return NavigationWidgetPrivate::m_instance;
}

void NavigationWidget::setFactories(const QList<INavigationWidgetFactory *> factories)
{
    ICore *core = ICore::instance();
    ActionManager *am = core->actionManager();
    Context navicontext(Core::Constants::C_NAVIGATION_PANE);

    foreach (INavigationWidgetFactory *factory, factories) {
        const QString id = factory->id();

        QShortcut *shortcut = new QShortcut(this);
        shortcut->setWhatsThis(tr("Activate %1 Pane").arg(factory->displayName()));
        connect(shortcut, SIGNAL(activated()), this, SLOT(activateSubWidget()));
        d->m_shortcutMap.insert(shortcut, id);

        Command *cmd = am->registerShortcut(shortcut,
            Id(QLatin1String("QtCreator.Sidebar.") + id), navicontext);
        cmd->setDefaultKeySequence(factory->activationSequence());
        d->m_commandMap.insert(id, cmd);

        QStandardItem *newRow = new QStandardItem(factory->displayName());
        newRow->setData(qVariantFromValue(factory), FactoryObjectRole);
        newRow->setData(factory->id(), FactoryIdRole);
        newRow->setData(factory->priority(), FactoryPriorityRole);
        d->m_factoryModel->appendRow(newRow);
    }
    d->m_factoryModel->sort(0);
}

int NavigationWidget::storedWidth()
{
    return d->m_width;
}

QAbstractItemModel *NavigationWidget::factoryModel() const
{
    return d->m_factoryModel;
}

void NavigationWidget::updateToggleText()
{
    bool haveData = d->m_factoryModel->rowCount();
    d->m_toggleSideBarAction->setVisible(haveData);
    d->m_toggleSideBarAction->setEnabled(haveData);

    if (isShown())
        d->m_toggleSideBarAction->setToolTip(tr("Hide Sidebar"));
    else
        d->m_toggleSideBarAction->setToolTip(tr("Show Sidebar"));
}

void NavigationWidget::placeHolderChanged(NavigationWidgetPlaceHolder *holder)
{
    d->m_toggleSideBarAction->setEnabled(holder);
    d->m_toggleSideBarAction->setChecked(holder && isShown());
    updateToggleText();
}

void NavigationWidget::resizeEvent(QResizeEvent *re)
{
    if (d->m_width && re->size().width())
        d->m_width = re->size().width();
    MiniSplitter::resizeEvent(re);
}

Internal::NavigationSubWidget *NavigationWidget::insertSubItem(int position,int index)
{
    for (int pos = position + 1; pos < d->m_subWidgets.size(); ++pos) {
        d->m_subWidgets.at(pos)->setPosition(pos + 1);
    }

    Internal::NavigationSubWidget *nsw = new Internal::NavigationSubWidget(this, position, index);
    connect(nsw, SIGNAL(splitMe()), this, SLOT(splitSubWidget()));
    connect(nsw, SIGNAL(closeMe()), this, SLOT(closeSubWidget()));
    insertWidget(position, nsw);
    d->m_subWidgets.insert(position, nsw);

    return nsw;
}

void NavigationWidget::activateSubWidget()
{
    QShortcut *original = qobject_cast<QShortcut *>(sender());
    QString id = d->m_shortcutMap[original];
    activateSubWidget(id);
}

void NavigationWidget::activateSubWidget(const QString &factoryId)
{
    setShown(true);
    foreach (Internal::NavigationSubWidget *subWidget, d->m_subWidgets) {
        if (subWidget->factory()->id() == factoryId) {
            subWidget->setFocusWidget();
            return;
        }
    }

    int index = factoryIndex(factoryId);
    if (index >= 0) {
        d->m_subWidgets.first()->setFactoryIndex(index);
        d->m_subWidgets.first()->setFocusWidget();
    }
}

void NavigationWidget::splitSubWidget()
{
    Internal::NavigationSubWidget *original = qobject_cast<Internal::NavigationSubWidget *>(sender());
    int pos = indexOf(original) + 1;
    insertSubItem(pos, original->factoryIndex());
}

void NavigationWidget::closeSubWidget()
{
    if (d->m_subWidgets.count() != 1) {
        Internal::NavigationSubWidget *subWidget = qobject_cast<Internal::NavigationSubWidget *>(sender());
        subWidget->saveSettings();
        d->m_subWidgets.removeOne(subWidget);
        subWidget->hide();
        subWidget->deleteLater();
    } else {
        setShown(false);
    }
}

void NavigationWidget::saveSettings(QSettings *settings)
{
    QStringList viewIds;
    for (int i=0; i<d->m_subWidgets.count(); ++i) {
        d->m_subWidgets.at(i)->saveSettings();
        viewIds.append(d->m_subWidgets.at(i)->factory()->id());
    }
    settings->setValue(QLatin1String("Navigation/Views"), viewIds);
    settings->setValue(QLatin1String("Navigation/Visible"), isShown());
    settings->setValue(QLatin1String("Navigation/VerticalPosition"), saveState());
    settings->setValue(QLatin1String("Navigation/Width"), d->m_width);
}

void NavigationWidget::restoreSettings(QSettings *settings)
{
    if (!d->m_factoryModel->rowCount()) {
        // We have no widgets to show!
        setShown(false);
        return;
    }

    int version = settings->value(QLatin1String("Navigation/Version"), 1).toInt();
    QStringList viewIds = settings->value(QLatin1String("Navigation/Views"),
                                          QStringList("Projects")).toStringList();

    bool restoreSplitterState = true;
    if (version == 1) {
        if (!viewIds.contains(QLatin1String("Open Documents"))) {
            viewIds += QLatin1String("Open Documents");
            restoreSplitterState = false;
        }
        settings->setValue(QLatin1String("Navigation/Version"), 2);
    }

    int position = 0;
    foreach (const QString &id, viewIds) {
        int index = factoryIndex(id);
        if (index >= 0) {
            // Only add if the id was actually found!
            insertSubItem(position, index);
            ++position;
        } else {
            restoreSplitterState = false;
        }
    }

    if (d->m_subWidgets.isEmpty())
        // Make sure we have at least the projects widget
        insertSubItem(0, qMax(0, factoryIndex(QLatin1String("Projects"))));

    setShown(settings->value(QLatin1String("Navigation/Visible"), true).toBool());

    if (restoreSplitterState && settings->contains(QLatin1String("Navigation/VerticalPosition"))) {
        restoreState(settings->value(QLatin1String("Navigation/VerticalPosition")).toByteArray());
    } else {
        QList<int> sizes;
        sizes += 256;
        for (int i = viewIds.size()-1; i > 0; --i)
            sizes.prepend(512);
        setSizes(sizes);
    }

    d->m_width = settings->value(QLatin1String("Navigation/Width"), 240).toInt();
    if (d->m_width < 40)
        d->m_width = 40;

    // Apply
    if (NavigationWidgetPlaceHolder::m_current)
        NavigationWidgetPlaceHolder::m_current->applyStoredSize(d->m_width);
}

void NavigationWidget::closeSubWidgets()
{
    foreach (Internal::NavigationSubWidget *subWidget, d->m_subWidgets) {
        subWidget->saveSettings();
        delete subWidget;
    }
    d->m_subWidgets.clear();
}

void NavigationWidget::setShown(bool b)
{
    if (d->m_shown == b)
        return;
    bool haveData = d->m_factoryModel->rowCount();
    d->m_shown = b;
    if (NavigationWidgetPlaceHolder::m_current) {
        NavigationWidgetPlaceHolder::m_current->setVisible(d->m_shown && !d->m_suppressed && haveData);
        d->m_toggleSideBarAction->setChecked(d->m_shown && !d->m_suppressed && haveData);
    } else {
        d->m_toggleSideBarAction->setChecked(false);
    }
    updateToggleText();
}

bool NavigationWidget::isShown() const
{
    return d->m_shown;
}

bool NavigationWidget::isSuppressed() const
{
    return d->m_suppressed;
}

void NavigationWidget::setSuppressed(bool b)
{
    if (d->m_suppressed == b)
        return;
    d->m_suppressed = b;
    if (NavigationWidgetPlaceHolder::m_current)
        NavigationWidgetPlaceHolder::m_current->setVisible(d->m_shown && !d->m_suppressed);
}

int NavigationWidget::factoryIndex(const QString &id)
{
    for (int row = 0; row < d->m_factoryModel->rowCount(); ++row) {
        if (d->m_factoryModel->data(d->m_factoryModel->index(row, 0), FactoryIdRole).toString() == id) {
            return row;
        }
    }
    return -1;
}

QHash<QString, Core::Command*> NavigationWidget::commandMap() const
{
    return d->m_commandMap;
}

} // namespace Core
