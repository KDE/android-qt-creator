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

#include "completionwidget.h"
#include "completionsupport.h"
#include "icompletioncollector.h"

#include <texteditor/itexteditor.h>

#include <utils/faketooltip.h>
#include <utils/qtcassert.h>

#include <QtCore/QEvent>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QVBoxLayout>
#include <QtGui/QScrollBar>
#include <QtGui/QLabel>
#include <QtGui/QStylePainter>
#include <QtGui/QToolTip>

#include <limits.h>

using namespace TextEditor;
using namespace TextEditor::Internal;

#define NUMBER_OF_VISIBLE_ITEMS 10

namespace TextEditor {
namespace Internal {

class AutoCompletionModel : public QAbstractListModel
{
public:
    AutoCompletionModel(QObject *parent);

    inline const CompletionItem &itemAt(const QModelIndex &index) const
    { return m_items.at(index.row()); }

    void setItems(const QList<CompletionItem> &items);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private:
    QList<CompletionItem> m_items;
};


class CompletionInfoFrame : public Utils::FakeToolTip
{
public:
    CompletionInfoFrame(QWidget *parent = 0) :
        Utils::FakeToolTip(parent),
        m_label(new QLabel(this))
    {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->setSpacing(0);
        layout->addWidget(m_label);

        m_label->setForegroundRole(QPalette::ToolTipText);
        m_label->setBackgroundRole(QPalette::ToolTipBase);
    }

    void setText(const QString &text)
    {
        m_label->setText(text);
    }

private:
    QLabel *m_label;
};


} // namespace Internal
} // namespace TextEditor


AutoCompletionModel::AutoCompletionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void AutoCompletionModel::setItems(const QList<CompletionItem> &items)
{
    m_items = items;
    reset();
}

int AutoCompletionModel::rowCount(const QModelIndex &) const
{
    return m_items.count();
}

QVariant AutoCompletionModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= m_items.count())
        return QVariant();

    if (role == Qt::DisplayRole) {
        return itemAt(index).text;
    } else if (role == Qt::DecorationRole) {
        return itemAt(index).icon;
    } else if (role == Qt::WhatsThisRole) {
        return itemAt(index).details;
    }

    return QVariant();
}


CompletionWidget::CompletionWidget(CompletionSupport *support,
        ITextEditor *editor)
    : QFrame(0, Qt::Popup),
      m_support(support),
      m_editor(editor),
      m_completionListView(new CompletionListView(support, editor, this))
{
    // We disable the frame on this list view and use a QFrame around it instead.
    // This improves the look with QGTKStyle.
#ifndef Q_WS_MAC
    setFrameStyle(m_completionListView->frameStyle());
#endif
    m_completionListView->setFrameStyle(QFrame::NoFrame);

    setObjectName(QLatin1String("m_popupFrame"));
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumSize(1, 1);
    setFont(editor->widget()->font());

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

    layout->addWidget(m_completionListView);
    setFocusProxy(m_completionListView);

    connect(m_completionListView, SIGNAL(itemSelected(TextEditor::CompletionItem)),
            this, SIGNAL(itemSelected(TextEditor::CompletionItem)));
    connect(m_completionListView, SIGNAL(completionListClosed()),
            this, SIGNAL(completionListClosed()));
    connect(m_completionListView, SIGNAL(activated(QModelIndex)),
            SLOT(closeList(QModelIndex)));
    connect(editor, SIGNAL(contentsChangedBecauseOfUndo()),
            this, SLOT(closeList()));
}

CompletionWidget::~CompletionWidget()
{
}

void CompletionWidget::setCompletionItems(const QList<TextEditor::CompletionItem> &completionitems)
{
    m_completionListView->setCompletionItems(completionitems);
}

void CompletionWidget::closeList(const QModelIndex &index)
{
    m_completionListView->closeList(index);
    close();
}

void CompletionWidget::showCompletions(int startPos)
{
    ensurePolished();
    updatePositionAndSize(startPos);
    show();
    setFocus();
}

QChar CompletionWidget::typedChar() const
{
    return m_completionListView->m_typedChar;
}

CompletionItem CompletionWidget::currentCompletionItem() const
{
    return m_completionListView->currentCompletionItem();
}

bool CompletionWidget::explicitlySelected() const
{
    return m_completionListView->explicitlySelected();
}

void CompletionWidget::setCurrentIndex(int index)
{
    m_completionListView->setCurrentIndex(m_completionListView->model()->index(index, 0));
}

void CompletionWidget::updatePositionAndSize(int startPos)
{
    // Determine size by calculating the space of the visible items
    QAbstractItemModel *model = m_completionListView->model();
    int visibleItems = model->rowCount();
    if (visibleItems > NUMBER_OF_VISIBLE_ITEMS)
        visibleItems = NUMBER_OF_VISIBLE_ITEMS;

    const QStyleOptionViewItem &option = m_completionListView->viewOptions();

    QSize shint;
    for (int i = 0; i < visibleItems; ++i) {
        QSize tmp = m_completionListView->itemDelegate()->sizeHint(option, model->index(i, 0));
        if (shint.width() < tmp.width())
            shint = tmp;
    }

    const int fw = frameWidth();
    const int width = shint.width() + fw * 2 + 30;
    const int height = shint.height() * visibleItems + fw * 2;

    // Determine the position, keeping the popup on the screen
    const QRect cursorRect = m_editor->cursorRect(startPos);
    const QDesktopWidget *desktop = QApplication::desktop();

    QWidget *editorWidget = m_editor->widget();

#ifdef Q_WS_MAC
    const QRect screen = desktop->availableGeometry(desktop->screenNumber(editorWidget));
#else
    const QRect screen = desktop->screenGeometry(desktop->screenNumber(editorWidget));
#endif

    QPoint pos = cursorRect.bottomLeft();
    pos.rx() -= 16 + fw;    // Space for the icons

    if (pos.y() + height > screen.bottom())
        pos.setY(cursorRect.top() - height);

    if (pos.x() + width > screen.right())
        pos.setX(screen.right() - width);

    setGeometry(pos.x(), pos.y(), width, height);
}

CompletionListView::CompletionListView(CompletionSupport *support,
        ITextEditor *editor, CompletionWidget *completionWidget)
    : QListView(completionWidget),
      m_blockFocusOut(false),
      m_editor(editor),
      m_editorWidget(editor->widget()),
      m_completionWidget(completionWidget),
      m_model(new AutoCompletionModel(this)),
      m_support(support),
      m_explicitlySelected(false)
{
    QTC_ASSERT(m_editorWidget, return);

    m_infoTimer.setInterval(1000);
    m_infoTimer.setSingleShot(true);
    connect(&m_infoTimer, SIGNAL(timeout()), SLOT(maybeShowInfoTip()));

    setAttribute(Qt::WA_MacShowFocusRect, false);
    setUniformItemSizes(true);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setMinimumSize(1, 1);
    setModel(m_model);
#ifdef Q_WS_MAC
    if (horizontalScrollBar())
        horizontalScrollBar()->setAttribute(Qt::WA_MacMiniSize);
    if (verticalScrollBar())
        verticalScrollBar()->setAttribute(Qt::WA_MacMiniSize);
#endif
}

CompletionListView::~CompletionListView()
{
}

CompletionItem CompletionListView::currentCompletionItem() const
{
    int row = currentIndex().row();
    if (row >= 0 && row < m_model->rowCount())
        return m_model->itemAt(currentIndex());

    return CompletionItem();
}

bool CompletionListView::explicitlySelected() const
{
    return m_explicitlySelected;
}

void CompletionListView::maybeShowInfoTip()
{
    QModelIndex current = currentIndex();
    if (!current.isValid())
        return;
    QString infoTip = current.data(Qt::WhatsThisRole).toString();

    if (infoTip.isEmpty()) {
        delete m_infoFrame.data();
        m_infoTimer.setInterval(200);
        return;
    }

    if (m_infoFrame.isNull())
        m_infoFrame = new CompletionInfoFrame(this);


    QRect r = rectForIndex(current);
    m_infoFrame->move(
            (parentWidget()->mapToGlobal(
                    parentWidget()->rect().topRight())).x() + 3,
            mapToGlobal(r.topRight()).y() - verticalOffset()
            );
    m_infoFrame->setText(infoTip);
    m_infoFrame->adjustSize();
    m_infoFrame->show();
    m_infoFrame->raise();

    m_infoTimer.setInterval(0);
}

void CompletionListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QListView::currentChanged(current, previous);
    m_infoTimer.start();
}


bool CompletionListView::event(QEvent *e)
{
    if (m_blockFocusOut)
        return QListView::event(e);

    bool forwardKeys = true;
    if (e->type() == QEvent::FocusOut) {
        QModelIndex index;
#if defined(Q_OS_DARWIN) && ! defined(QT_MAC_USE_COCOA)
        QFocusEvent *fe = static_cast<QFocusEvent *>(e);
        if (fe->reason() == Qt::OtherFocusReason) {
            // Qt/carbon workaround
            // focus out is received before the key press event.
            index = currentIndex();
        }
#endif
        m_completionWidget->closeList(index);
        if (m_infoFrame)
            m_infoFrame->close();
        return true;
    } else if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        switch (ke->key()) {
        case Qt::Key_N:
        case Qt::Key_P:
            // select next/previous completion
            if (ke->modifiers() == Qt::ControlModifier)
            {
                e->accept();
                int change = (ke->key() == Qt::Key_N) ? 1 : -1;
                int nrows = model()->rowCount();
                int row = currentIndex().row();
                int newRow = (row + change + nrows) % nrows;
                if (newRow == row + change || !ke->isAutoRepeat())
                    setCurrentIndex(m_model->index(newRow));
                return true;
            }
        }
    } else if (e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        switch (ke->key()) {
        case Qt::Key_N:
        case Qt::Key_P:
            // select next/previous completion - so don't pass on to editor
            if (ke->modifiers() == Qt::ControlModifier)
                forwardKeys = false;
            break;

        case Qt::Key_Escape:
            m_completionWidget->closeList();
            return true;

        case Qt::Key_Right:
        case Qt::Key_Left:
        case Qt::Key_Home:
        case Qt::Key_End:
            // We want these navigation keys to work in the editor, so forward them
            break;

        case Qt::Key_Tab:
        case Qt::Key_Return:
            //independently from style, accept current entry if return is pressed
            if (qApp->focusWidget() == this)
                m_completionWidget->closeList(currentIndex());
            return true;

        case Qt::Key_Up:
            m_explicitlySelected = true;
            if (!ke->isAutoRepeat()
                && currentIndex().row() == 0) {
                setCurrentIndex(model()->index(model()->rowCount()-1, 0));
                return true;
            }
            forwardKeys = false;
            break;

        case Qt::Key_Down:
            m_explicitlySelected = true;
            if (!ke->isAutoRepeat()
                && currentIndex().row() == model()->rowCount()-1) {
                setCurrentIndex(model()->index(0, 0));
                return true;
            }
            forwardKeys = false;
            break;

        case Qt::Key_Enter:
        case Qt::Key_PageDown:
        case Qt::Key_PageUp:
            forwardKeys = false;
            break;

        default:
            // if a key is forwarded, completion widget is re-opened and selected item is reset to first,
            // so only forward keys that insert text and refine the completed item
            forwardKeys = !ke->text().isEmpty();
            break;
        }

        const CompletionPolicy policy = m_support->policy();
        if (forwardKeys && policy != QuickFixCompletion) {
            if (ke->text().length() == 1 && currentIndex().isValid() && qApp->focusWidget() == this) {
                QChar typedChar = ke->text().at(0);
                const CompletionItem &item = m_model->itemAt(currentIndex());
                if (item.collector->typedCharCompletes(item, typedChar)) {
                    m_typedChar = typedChar;
                    m_completionWidget->closeList(currentIndex());
                    return true;
                }
            }

            m_blockFocusOut = true;
            QApplication::sendEvent(m_editorWidget, e);
            m_blockFocusOut = false;

            // Have the completion support update the list of items
            m_support->complete(m_editor, policy, false);

            return true;
        }
    }
    return QListView::event(e);
}

void CompletionListView::keyboardSearch(const QString &search)
{
    Q_UNUSED(search)
}

void CompletionListView::setCompletionItems(const QList<TextEditor::CompletionItem> &completionItems)
{
    m_model->setItems(completionItems);
    setCurrentIndex(m_model->index(0)); // Select the first item
}

void CompletionListView::closeList(const QModelIndex &index)
{
    m_blockFocusOut = true;

    if (index.isValid())
        emit itemSelected(m_model->itemAt(index));

    emit completionListClosed();

    m_blockFocusOut = false;
}
