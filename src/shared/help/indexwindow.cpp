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

#include "centralwidget.h"

#include "helpviewer.h"
#include "indexwindow.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"
#include "topicchooser.h"

#include <utils/filterlineedit.h>
#include <utils/styledbar.h>

#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QListWidgetItem>

#include <QtHelp/QHelpEngine>
#include <QtHelp/QHelpIndexWidget>

using namespace Help::Internal;

IndexWindow::IndexWindow()
    : m_searchLineEdit(0)
    , m_indexWidget(0)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_searchLineEdit = new Utils::FilterLineEdit();
    m_searchLineEdit->setPlaceholderText(QString());
    setFocusProxy(m_searchLineEdit);
    connect(m_searchLineEdit, SIGNAL(textChanged(QString)), this,
        SLOT(filterIndices(QString)));
    m_searchLineEdit->installEventFilter(this);

    QLabel *l = new QLabel(tr("&Look for:"));
    l->setBuddy(m_searchLineEdit);
    layout->addWidget(l);
    layout->setMargin(0);
    layout->setSpacing(0);

    Utils::StyledBar *toolbar = new Utils::StyledBar(this);
    toolbar->setSingleRow(false);
    QLayout *tbLayout = new QHBoxLayout();
    tbLayout->setSpacing(6);
    tbLayout->setMargin(4);
    tbLayout->addWidget(l);
    tbLayout->addWidget(m_searchLineEdit);
    toolbar->setLayout(tbLayout);
    layout->addWidget(toolbar);

    QHelpEngine *engine = &LocalHelpManager::helpEngine();
    m_indexWidget = engine->indexWidget();
    m_indexWidget->installEventFilter(this);
    connect(engine->indexModel(), SIGNAL(indexCreationStarted()), this,
        SLOT(disableSearchLineEdit()));
    connect(engine->indexModel(), SIGNAL(indexCreated()), this,
        SLOT(enableSearchLineEdit()));
    connect(m_indexWidget, SIGNAL(linkActivated(QUrl, QString)), this,
        SIGNAL(linkActivated(QUrl)));
    connect(m_indexWidget, SIGNAL(linksActivated(QMap<QString, QUrl>, QString)),
        this, SIGNAL(linksActivated(QMap<QString, QUrl>, QString)));
    connect(m_searchLineEdit, SIGNAL(returnPressed()), m_indexWidget,
        SLOT(activateCurrentItem()));
    m_indexWidget->setFrameStyle(QFrame::NoFrame);
    layout->addWidget(m_indexWidget);

    m_indexWidget->viewport()->installEventFilter(this);
}

IndexWindow::~IndexWindow()
{
}

void IndexWindow::filterIndices(const QString &filter)
{
    if (filter.contains(QLatin1Char('*')))
        m_indexWidget->filterIndices(filter, filter);
    else
        m_indexWidget->filterIndices(filter, QString());
}

bool IndexWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_searchLineEdit && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);
        QModelIndex idx = m_indexWidget->currentIndex();
        switch (ke->key()) {
        case Qt::Key_Up:
            idx = m_indexWidget->model()->index(idx.row()-1,
                idx.column(), idx.parent());
            if (idx.isValid())
                m_indexWidget->setCurrentIndex(idx);
            break;
        case Qt::Key_Down:
            idx = m_indexWidget->model()->index(idx.row()+1,
                idx.column(), idx.parent());
            if (idx.isValid())
                m_indexWidget->setCurrentIndex(idx);
            break;
        default: ; // stop complaining
        }
    } else if (obj == m_searchLineEdit
            && e->type() == QEvent::FocusIn
            && static_cast<QFocusEvent *>(e)->reason() != Qt::MouseFocusReason) {
        m_searchLineEdit->selectAll();
        m_searchLineEdit->setFocus();
    } else if (obj == m_indexWidget && e->type() == QEvent::ContextMenu) {
        QContextMenuEvent *ctxtEvent = static_cast<QContextMenuEvent*>(e);
        QModelIndex idx = m_indexWidget->indexAt(ctxtEvent->pos());
        if (idx.isValid()) {
            QMenu menu;
            QAction *curTab = menu.addAction(tr("Open Link"));
            QAction *newTab = menu.addAction(tr("Open Link as New Page"));
            menu.move(m_indexWidget->mapToGlobal(ctxtEvent->pos()));

            QAction *action = menu.exec();
            if (curTab == action)
                m_indexWidget->activateCurrentItem();
            else if (newTab == action) {
                open(m_indexWidget, idx);
            }
        }
    } else if (m_indexWidget && obj == m_indexWidget->viewport()
        && e->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(e);
        QModelIndex idx = m_indexWidget->indexAt(mouseEvent->pos());
        if (idx.isValid()) {
            Qt::MouseButtons button = mouseEvent->button();
            if (((button == Qt::LeftButton) && (mouseEvent->modifiers() & Qt::ControlModifier))
                || (button == Qt::MidButton)) {
                open(m_indexWidget, idx);
            }
        }
    }
#ifdef Q_WS_MAC
    else if (obj == m_indexWidget && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
           m_indexWidget->activateCurrentItem();
    }
#endif

    return QWidget::eventFilter(obj, e);
}

void IndexWindow::enableSearchLineEdit()
{
    m_searchLineEdit->setDisabled(false);
    filterIndices(m_searchLineEdit->text());
}

void IndexWindow::disableSearchLineEdit()
{
    m_searchLineEdit->setDisabled(true);
}

void IndexWindow::setSearchLineEditText(const QString &text)
{
    m_searchLineEdit->setText(text);
}

void IndexWindow::open(QHelpIndexWidget* indexWidget, const QModelIndex &index)
{
    QHelpIndexModel *model = qobject_cast<QHelpIndexModel*>(indexWidget->model());
    if (model) {
        QString keyword = model->data(index, Qt::DisplayRole).toString();
        QMap<QString, QUrl> links = model->linksForKeyword(keyword);

        QUrl url;
        if (links.count() > 1) {
            TopicChooser tc(this, keyword, links);
            if (tc.exec() == QDialog::Accepted)
                url = tc.link();
        } else if (links.count() == 1) {
            url = links.constBegin().value();
        } else {
            return;
        }

        if (!HelpViewer::canOpenPage(url.path()))
            CentralWidget::instance()->setSource(url);
        else
            OpenPagesManager::instance().createPage(url);
    }
}
