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

#include "topicchooser.h"

#include <QtCore/QMap>
#include <QtCore/QUrl>

#include <QtGui/QKeyEvent>
#include <QtGui/QStandardItemModel>
#include <QtGui/QSortFilterProxyModel>

TopicChooser::TopicChooser(QWidget *parent, const QString &keyword,
        const QMap<QString, QUrl> &links)
    : QDialog(parent)
    , m_filterModel(new QSortFilterProxyModel(this))
{
    ui.setupUi(this);

    setFocusProxy(ui.lineEdit);
    ui.lineEdit->installEventFilter(this);
    ui.lineEdit->setPlaceholderText(tr("Filter"));
    ui.label->setText(tr("Choose a topic for <b>%1</b>:").arg(keyword));

    QStandardItemModel *model = new QStandardItemModel(this);
    m_filterModel->setSourceModel(model);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    QMap<QString, QUrl>::const_iterator it = links.constBegin();
    for (; it != links.constEnd(); ++it) {
        m_links.append(it.value());
        model->appendRow(new QStandardItem(it.key()));
    }

    ui.listWidget->setModel(m_filterModel);
    ui.listWidget->setUniformItemSizes(true);
    ui.listWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    if (m_filterModel->rowCount() != 0)
        ui.listWidget->setCurrentIndex(m_filterModel->index(0, 0));

    connect(ui.buttonDisplay, SIGNAL(clicked()), this, SLOT(acceptDialog()));
    connect(ui.buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui.listWidget, SIGNAL(activated(QModelIndex)), this,
        SLOT(activated(QModelIndex)));
    connect(ui.lineEdit, SIGNAL(filterChanged(QString)), this,
        SLOT(setFilter(QString)));
}

QUrl TopicChooser::link() const
{
    if (m_activedIndex.isValid())
        return m_links.at(m_filterModel->mapToSource(m_activedIndex).row());
    return QUrl();
}

void TopicChooser::acceptDialog()
{
    m_activedIndex = ui.listWidget->currentIndex();
    accept();
}

void TopicChooser::setFilter(const QString &pattern)
{
    m_filterModel->setFilterFixedString(pattern);
    if (m_filterModel->rowCount() != 0 && !ui.listWidget->currentIndex().isValid())
        ui.listWidget->setCurrentIndex(m_filterModel->index(0, 0));
}

void TopicChooser::activated(const QModelIndex &index)
{
    m_activedIndex = index;
    accept();
}

bool TopicChooser::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui.lineEdit && event->type() == QEvent::KeyPress) {
        QModelIndex idx = ui.listWidget->currentIndex();
        switch ((static_cast<QKeyEvent*>(event)->key())) {
            case Qt::Key_Up:
                idx = m_filterModel->index(idx.row() - 1, idx.column(),
                    idx.parent());
                if (idx.isValid())
                    ui.listWidget->setCurrentIndex(idx);
                break;

            case Qt::Key_Down:
                idx = m_filterModel->index(idx.row() + 1, idx.column(),
                    idx.parent());
                if (idx.isValid())
                    ui.listWidget->setCurrentIndex(idx);
                break;

            default: ;
        }
    } else if (ui.lineEdit && event->type() == QEvent::FocusIn
        && static_cast<QFocusEvent *>(event)->reason() != Qt::MouseFocusReason) {
        ui.lineEdit->selectAll();
        ui.lineEdit->setFocus();
    }
    return QDialog::eventFilter(object, event);
}
