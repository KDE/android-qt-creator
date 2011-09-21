/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
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

#include "classviewnavigationwidget.h"
#include "ui_classviewnavigationwidget.h"
#include "classviewtreeitemmodel.h"
#include "classviewmanager.h"
#include "classviewsymbollocation.h"
#include "classviewsymbolinformation.h"
#include "classviewutils.h"
#include "classviewconstants.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QPointer>

enum { debug = false };

namespace ClassView {
namespace Internal {

///////////////////////////////// NavigationWidgetPrivate //////////////////////////////////

/*!
   \struct NavigationWidgetPrivate
   \brief Internal data structures / methods for NavigationWidget
 */

class NavigationWidgetPrivate
{
public:
    NavigationWidgetPrivate() : ui(0) {}

    //! Ui generated by Designer
    Ui::NavigationWidget *ui;

    //! current tree model
    QPointer<TreeItemModel> treeModel;

    //! full projects mode
    QPointer<QToolButton> fullProjectsModeButton;
};

///////////////////////////////// NavigationWidget //////////////////////////////////


NavigationWidget::NavigationWidget(QWidget *parent) :
    QWidget(parent),
    d(new NavigationWidgetPrivate())
{
    d->ui = new Ui::NavigationWidget;
    d->ui->setupUi(this);

    // tree model
    d->treeModel = new TreeItemModel(this);
    d->ui->treeView->setModel(d->treeModel);

    // connect signal/slots
    // selected item
    connect(d->ui->treeView, SIGNAL(activated(QModelIndex)), SLOT(onItemActivated(QModelIndex)));

    // connections to the manager
    Manager *manager = Manager::instance();

    connect(this, SIGNAL(visibilityChanged(bool)),
            manager, SLOT(onWidgetVisibilityIsChanged(bool)));

    connect(this, SIGNAL(requestGotoLocation(QString,int,int)),
            manager, SLOT(gotoLocation(QString,int,int)));

    connect(this, SIGNAL(requestGotoLocations(QList<QVariant>)),
            manager, SLOT(gotoLocations(QList<QVariant>)));

    connect(manager, SIGNAL(treeDataUpdate(QSharedPointer<QStandardItem>)),
            this, SLOT(onDataUpdate(QSharedPointer<QStandardItem>)));

    connect(this, SIGNAL(requestTreeDataUpdate()),
            manager, SLOT(onRequestTreeDataUpdate()));
}

NavigationWidget::~NavigationWidget()
{
    delete d->fullProjectsModeButton;
    delete d->ui;
    delete d;
}

void NavigationWidget::hideEvent(QHideEvent *event)
{
    emit visibilityChanged(false);
    QWidget::hideEvent(event);
}

void NavigationWidget::showEvent(QShowEvent *event)
{
    emit visibilityChanged(true);

    // request to update to the current state - to be sure
    emit requestTreeDataUpdate();

    QWidget::showEvent(event);
}

QList<QToolButton *> NavigationWidget::createToolButtons()
{
    QList<QToolButton *> list;

    // full projects mode
    if (!d->fullProjectsModeButton) {
        // create a button
        d->fullProjectsModeButton = new QToolButton();
        d->fullProjectsModeButton->setIcon(
                QIcon(QLatin1String(":/classview/images/hierarchicalmode.png")));
        d->fullProjectsModeButton->setCheckable(true);
        d->fullProjectsModeButton->setToolTip(tr("Show Subprojects"));

        // by default - not a flat mode
        setFlatMode(false);

        // connections
        connect(d->fullProjectsModeButton, SIGNAL(toggled(bool)),
                this, SLOT(onFullProjectsModeToggled(bool)));
    }

    list << d->fullProjectsModeButton;

    return list;
}

bool NavigationWidget::flatMode() const
{
    QTC_ASSERT(d->fullProjectsModeButton, return false);

    // button is 'full projects mode' - so it has to be inverted
    return !d->fullProjectsModeButton->isChecked();
}

void NavigationWidget::setFlatMode(bool flatMode)
{
    QTC_ASSERT(d->fullProjectsModeButton, return);

    // button is 'full projects mode' - so it has to be inverted
    d->fullProjectsModeButton->setChecked(!flatMode);
}

void NavigationWidget::onFullProjectsModeToggled(bool state)
{
    // button is 'full projects mode' - so it has to be inverted
    Manager::instance()->setFlatMode(!state);
}

void NavigationWidget::onItemActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QList<QVariant> list = d->treeModel->data(index, Constants::SymbolLocationsRole).toList();

    emit requestGotoLocations(list);
}

void NavigationWidget::onDataUpdate(QSharedPointer<QStandardItem> result)
{
    if (result.isNull())
        return;

    QTime timer;
    if (debug)
        timer.start();
    // update is received. root item must be updated - and received information
    // might be just a root - if a lazy data population is enabled.
    // so expanded items must be parsed and 'fetched'

    fetchExpandedItems(result.data(), d->treeModel->invisibleRootItem());

    d->treeModel->moveRootToTarget(result.data());

    // expand top level projects
    QModelIndex sessionIndex;

    for (int i = 0; i < d->treeModel->rowCount(sessionIndex); ++i)
        d->ui->treeView->expand(d->treeModel->index(i, 0, sessionIndex));

    if (debug)
        qDebug() << "Class View:" << QDateTime::currentDateTime().toString()
            << "TreeView is updated in" << timer.elapsed() << "msecs";
}

void NavigationWidget::fetchExpandedItems(QStandardItem *item, const QStandardItem *target) const
{
    if (!item || !target)
        return;

    const QModelIndex &parent = d->treeModel->indexFromItem(target);
    if (d->ui->treeView->isExpanded(parent))
        Manager::instance()->fetchMore(item, true);

    int itemIndex = 0;
    int targetIndex = 0;
    int itemRows = item->rowCount();
    int targetRows = target->rowCount();

    while (itemIndex < itemRows && targetIndex < targetRows) {
        QStandardItem *itemChild = item->child(itemIndex);
        const QStandardItem *targetChild = target->child(targetIndex);

        const SymbolInformation &itemInf = Utils::symbolInformationFromItem(itemChild);
        const SymbolInformation &targetInf = Utils::symbolInformationFromItem(targetChild);

        if (itemInf < targetInf) {
            ++itemIndex;
        } else if (itemInf == targetInf) {
            fetchExpandedItems(itemChild, targetChild);
            ++itemIndex;
            ++targetIndex;
        } else {
            ++targetIndex;
        }
    }
}

} // namespace Internal
} // namespace ClassView
