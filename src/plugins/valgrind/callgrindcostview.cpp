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

#include "callgrindcostview.h"

#include "callgrindnamedelegate.h"

#include <valgrind/callgrind/callgrindabstractmodel.h>
#include <valgrind/callgrind/callgrinddatamodel.h>
#include <valgrind/callgrind/callgrindfunction.h>
#include <valgrind/callgrind/callgrindcallmodel.h>

#include <QtGui/QAbstractProxyModel>
#include <QtGui/QHeaderView>
#include <QtCore/QDebug>

using namespace Valgrind::Callgrind;

namespace Valgrind {
namespace Internal {

class CostView::Private
{
public:
    explicit Private(CostView *qq);

    CostDelegate *m_costDelegate;
    NameDelegate *m_nameDelegate;
};

CostView::Private::Private(CostView *qq)
    : m_costDelegate(new CostDelegate(qq))
    , m_nameDelegate(new NameDelegate(qq))
{}


CostView::CostView(QWidget *parent)
    : QTreeView(parent)
    , d(new Private(this))
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setUniformRowHeights(true);
    setAutoScroll(false);
    setSortingEnabled(true);
    setRootIsDecorated(false);
}

CostView::~CostView()
{
    delete d;
}

void CostView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);

    forever {
        QAbstractProxyModel *proxy = qobject_cast<QAbstractProxyModel *>(model);
        if (proxy)
            model = proxy->sourceModel();
        else
            break;
    }

    QHeaderView *headerView = header();
    setItemDelegate(new QStyledItemDelegate(this));
    headerView->setResizeMode(QHeaderView::Interactive);
    headerView->setStretchLastSection(false);

    if (qobject_cast<CallModel *>(model)) {
        headerView->setResizeMode(CallModel::CalleeColumn, QHeaderView::Stretch);
        headerView->setResizeMode(CallModel::CallerColumn, QHeaderView::Stretch);
        headerView->setResizeMode(CallModel::CallsColumn, QHeaderView::ResizeToContents);
        headerView->setResizeMode(CallModel::CostColumn, QHeaderView::ResizeToContents);
        setItemDelegateForColumn(CallModel::CalleeColumn, d->m_nameDelegate);
        setItemDelegateForColumn(CallModel::CallerColumn, d->m_nameDelegate);
        setItemDelegateForColumn(CallModel::CostColumn, d->m_costDelegate);
    } else if (qobject_cast<DataModel *>(model)) {
        headerView->setResizeMode(DataModel::InclusiveCostColumn, QHeaderView::ResizeToContents);
        headerView->setResizeMode(DataModel::LocationColumn, QHeaderView::Stretch);
        headerView->setResizeMode(DataModel::NameColumn, QHeaderView::Stretch);
        headerView->setResizeMode(DataModel::SelfCostColumn, QHeaderView::ResizeToContents);
        setItemDelegateForColumn(DataModel::InclusiveCostColumn, d->m_costDelegate);
        setItemDelegateForColumn(DataModel::NameColumn, d->m_nameDelegate);
        setItemDelegateForColumn(DataModel::SelfCostColumn, d->m_costDelegate);
    }

    d->m_costDelegate->setModel(model);
}

void CostView::setCostFormat(CostDelegate::CostFormat format)
{
    d->m_costDelegate->setFormat(format);
    viewport()->update();
}

CostDelegate::CostFormat CostView::costFormat() const
{
    return d->m_costDelegate->format();
}

} // namespace Internal
} // namespace Valgrind
