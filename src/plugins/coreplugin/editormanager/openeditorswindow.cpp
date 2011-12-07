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

#include "openeditorswindow.h"
#include "openeditorsmodel.h"
#include "editormanager.h"
#include "editorview.h"
#include "ifile.h"

#include <utils/qtcassert.h>

#include <QtGui/QFocusEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QTreeWidget>
#include <QtGui/QVBoxLayout>

Q_DECLARE_METATYPE(Core::Internal::EditorView*)
Q_DECLARE_METATYPE(Core::IFile *)

using namespace Core;
using namespace Core::Internal;

const int WIDTH = 300;
const int HEIGHT = 200;

OpenEditorsWindow::OpenEditorsWindow(QWidget *parent) :
    QFrame(parent, Qt::Popup),
    m_emptyIcon(QLatin1String(":/core/images/empty14.png")),
    m_editorList(new QTreeWidget(this))
{
    resize(QSize(WIDTH, HEIGHT));
    m_editorList->setColumnCount(1);
    m_editorList->header()->hide();
    m_editorList->setIndentation(0);
    m_editorList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_editorList->setTextElideMode(Qt::ElideMiddle);
#ifdef Q_WS_MAC
    m_editorList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
#endif
    m_editorList->installEventFilter(this);

    // We disable the frame on this list view and use a QFrame around it instead.
    // This improves the look with QGTKStyle.
#ifndef Q_WS_MAC
    setFrameStyle(m_editorList->frameStyle());
#endif
    m_editorList->setFrameStyle(QFrame::NoFrame);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_editorList);

    connect(m_editorList, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
            this, SLOT(editorClicked(QTreeWidgetItem*)));
}

void OpenEditorsWindow::selectAndHide()
{
    setVisible(false);
    selectEditor(m_editorList->currentItem());
}

void OpenEditorsWindow::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if (visible) {
        setFocus();
    }
}

bool OpenEditorsWindow::isCentering()
{
    int internalMargin = m_editorList->viewport()->mapTo(m_editorList, QPoint(0,0)).y();
    QRect rect0 = m_editorList->visualItemRect(m_editorList->topLevelItem(0));
    QRect rect1 = m_editorList->visualItemRect(m_editorList->topLevelItem(m_editorList->topLevelItemCount()-1));
    int height = rect1.y() + rect1.height() - rect0.y();
    height += 2 * internalMargin;
    if (height > HEIGHT)
        return true;
    return false;
}


bool OpenEditorsWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_editorList) {
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->key() == Qt::Key_Escape) {
                setVisible(false);
                return true;
            }
            if (ke->key() == Qt::Key_Return) {
                selectEditor(m_editorList->currentItem());
                return true;
            }
        } else if (e->type() == QEvent::KeyRelease) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->modifiers() == 0
                    /*HACK this is to overcome some event inconsistencies between platforms*/
                    || (ke->modifiers() == Qt::AltModifier
                    && (ke->key() == Qt::Key_Alt || ke->key() == -1))) {
                selectAndHide();
            }
        }
    }
    return QWidget::eventFilter(obj, e);
}

void OpenEditorsWindow::focusInEvent(QFocusEvent *)
{
    m_editorList->setFocus();
}

void OpenEditorsWindow::selectUpDown(bool up)
{
    int itemCount = m_editorList->topLevelItemCount();
    if (itemCount < 2)
        return;
    int index = m_editorList->indexOfTopLevelItem(m_editorList->currentItem());
    if (index < 0)
        return;
    QTreeWidgetItem *editor = 0;
    int count = 0;
    while (!editor && count < itemCount) {
        if (up) {
            index--;
            if (index < 0)
                index = itemCount-1;
        } else {
            index++;
            if (index >= itemCount)
                index = 0;
        }
        editor = m_editorList->topLevelItem(index);
        count++;
    }
    if (editor) {
        m_editorList->setCurrentItem(editor);
        ensureCurrentVisible();
    }
}

void OpenEditorsWindow::selectPreviousEditor()
{
    selectUpDown(false);
}

void OpenEditorsWindow::selectNextEditor()
{
    selectUpDown(true);
}

void OpenEditorsWindow::centerOnItem(int selectedIndex)
{
    if (selectedIndex >= 0) {
        QTreeWidgetItem *item;
        int num = m_editorList->topLevelItemCount();
        int rotate = selectedIndex-(num-1)/2;
        for (int i = 0; i < rotate; ++i) {
            item = m_editorList->takeTopLevelItem(0);
            m_editorList->addTopLevelItem(item);
        }
        rotate = -rotate;
        for (int i = 0; i < rotate; ++i) {
            item = m_editorList->takeTopLevelItem(num-1);
            m_editorList->insertTopLevelItem(0, item);
        }
    }
}

void OpenEditorsWindow::setEditors(EditorView *mainView, EditorView *view, OpenEditorsModel *model)
{
    m_editorList->clear();
    bool first = true;

    QSet<IFile*> filesDone;
    foreach (const EditLocation &hi, view->editorHistory()) {
        if (hi.file.isNull() || filesDone.contains(hi.file))
            continue;
        QString title = model->displayNameForFile(hi.file);
        QTC_ASSERT(!title.isEmpty(), continue;)
        filesDone.insert(hi.file.data());
        QTreeWidgetItem *item = new QTreeWidgetItem();
        if (hi.file->isModified())
            title += tr("*");
        item->setIcon(0, hi.file->isReadOnly() ? model->lockedIcon() : m_emptyIcon);
        item->setText(0, title);
        item->setToolTip(0, hi.file->fileName());
        item->setData(0, Qt::UserRole, QVariant::fromValue(hi.file.data()));
        item->setData(0, Qt::UserRole+1, QVariant::fromValue(view));
        item->setTextAlignment(0, Qt::AlignLeft);

        m_editorList->addTopLevelItem(item);

        if (first){
            m_editorList->setCurrentItem(item);
            first = false;
        }
    }

    // add missing editors from the main view
    if (mainView != view) {
        foreach (const EditLocation &hi, mainView->editorHistory()) {
            if (hi.file.isNull() || filesDone.contains(hi.file))
                continue;
            filesDone.insert(hi.file.data());

            QTreeWidgetItem *item = new QTreeWidgetItem();

            QString title = model->displayNameForFile(hi.file);
            if (hi.file->isModified())
                title += tr("*");
            item->setIcon(0, hi.file->isReadOnly() ? model->lockedIcon() : m_emptyIcon);
            item->setText(0, title);
            item->setToolTip(0, hi.file->fileName());
            item->setData(0, Qt::UserRole, QVariant::fromValue(hi.file.data()));
            item->setData(0, Qt::UserRole+1, QVariant::fromValue(view));
            item->setData(0, Qt::UserRole+2, QVariant::fromValue(hi.id));
            item->setTextAlignment(0, Qt::AlignLeft);

            m_editorList->addTopLevelItem(item);

            if (first){
                m_editorList->setCurrentItem(item);
                first = false;
            }
        }
    }

    // add purely restored editors which are not initialised yet
    foreach (const OpenEditorsModel::Entry &entry, model->entries()) {
        if (entry.editor)
            continue;
        QTreeWidgetItem *item = new QTreeWidgetItem();
        QString title = entry.displayName();
        item->setIcon(0, m_emptyIcon);
        item->setText(0, title);
        item->setToolTip(0, entry.fileName());
        item->setData(0, Qt::UserRole+2, QVariant::fromValue(entry.id()));
        item->setTextAlignment(0, Qt::AlignLeft);

        m_editorList->addTopLevelItem(item);
    }
}


void OpenEditorsWindow::selectEditor(QTreeWidgetItem *item)
{
    if (!item)
        return;
    if (IFile *file = item->data(0, Qt::UserRole).value<IFile*>()) {
        EditorView *view = item->data(0, Qt::UserRole+1).value<EditorView*>();
        EditorManager::instance()->activateEditorForFile(view, file, EditorManager::ModeSwitch);
    } else {
        if (!EditorManager::instance()->openEditor(
                    item->toolTip(0), item->data(0, Qt::UserRole+2).value<Core::Id>(),
                    Core::EditorManager::ModeSwitch)) {
            EditorManager::instance()->openedEditorsModel()->removeEditor(item->toolTip(0));
            delete item;
        }
    }
}

void OpenEditorsWindow::editorClicked(QTreeWidgetItem *item)
{
    selectEditor(item);
    setFocus();
}


void OpenEditorsWindow::ensureCurrentVisible()
{
    m_editorList->scrollTo(m_editorList->currentIndex(), QAbstractItemView::PositionAtCenter);
}

