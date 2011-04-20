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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include <qglobal.h>

#include "locatorwidget.h"
#include "locatorplugin.h"
#include "locatorconstants.h"
#include "ilocatorfilter.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/fileiconprovider.h>
#include <utils/filterlineedit.h>
#include <utils/qtcassert.h>
#include <qtconcurrent/runextensions.h>

#include <QtCore/QtConcurrentRun>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtCore/QEvent>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>
#include <QtGui/QMouseEvent>
#include <QtGui/QPushButton>
#include <QtGui/QScrollBar>
#include <QtGui/QTreeView>

Q_DECLARE_METATYPE(Locator::ILocatorFilter*)
Q_DECLARE_METATYPE(Locator::FilterEntry)

namespace Locator {
namespace Internal {

/* A model to represent the Locator results. */
class LocatorModel : public QAbstractListModel
{
public:
    LocatorModel(QObject *parent = 0)
        : QAbstractListModel(parent)
//        , mDisplayCount(64)
    {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void setEntries(const QList<FilterEntry> &entries);
    //void setDisplayCount(int count);

private:
    mutable QList<FilterEntry> mEntries;
    //int mDisplayCount;
};

class CompletionList : public QTreeView
{
public:
    CompletionList(QWidget *parent = 0);

    void updatePreferredSize();
    QSize preferredSize() const { return m_preferredSize; }

#if defined(Q_OS_WIN)
    void focusOutEvent (QFocusEvent * event)  {
        if (event->reason() == Qt::ActiveWindowFocusReason)
            hide();
    }
#endif

    void next() {
        int index = currentIndex().row();
        ++index;
        if (index >= model()->rowCount(QModelIndex())) {
            // wrap
            index = 0;
        }
        setCurrentIndex(model()->index(index, 0));
    }

    void previous() {
        int index = currentIndex().row();
        --index;
        if (index < 0) {
            // wrap
            index = model()->rowCount(QModelIndex()) - 1;
        }
        setCurrentIndex(model()->index(index, 0));
    }

private:
    QSize m_preferredSize;
};

} // namespace Internal

uint qHash(const FilterEntry &entry)
{
    if (entry.internalData.canConvert(QVariant::String))
        return QT_PREPEND_NAMESPACE(qHash)(entry.internalData.toString());
    return QT_PREPEND_NAMESPACE(qHash)(entry.internalData.constData());
}

} // namespace Locator

using namespace Locator;
using namespace Locator::Internal;

// =========== LocatorModel ===========

int LocatorModel::rowCount(const QModelIndex & /* parent */) const
{
    return mEntries.size();
}

int LocatorModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

/*!
 * When asked for the icon via Qt::DecorationRole, the LocatorModel lazily
 * resolves and caches the Greehouse-specific file icon when
 * FilterEntry::resolveFileIcon is true. FilterEntry::internalData is assumed
 * to be the filename.
 */
QVariant LocatorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= mEntries.size())
        return QVariant();

    if (role == Qt::DisplayRole  || role == Qt::ToolTipRole) {
        if (index.column() == 0) {
            return mEntries.at(index.row()).displayName;
        } else if (index.column() == 1) {
            return mEntries.at(index.row()).extraInfo;
        }
    } else if (role == Qt::DecorationRole && index.column() == 0) {
        FilterEntry &entry = mEntries[index.row()];
        if (entry.resolveFileIcon && entry.displayIcon.isNull()) {
            entry.resolveFileIcon = false;
            entry.displayIcon =
                 Core::FileIconProvider::instance()->icon(QFileInfo(entry.internalData.toString()));
        }
        return entry.displayIcon;
    } else if (role == Qt::ForegroundRole && index.column() == 1) {
        return Qt::darkGray;
    } else if (role == Qt::UserRole) {
        return qVariantFromValue(mEntries.at(index.row()));
    }

    return QVariant();
}

void LocatorModel::setEntries(const QList<FilterEntry> &entries)
{
    mEntries = entries;
    reset();
}
#if 0
void LocatorModel::setDisplayCount(int count)
{
    // TODO: This method is meant to be used for increasing the number of items displayed at the
    // user's request. There is however no way yet for the user to request this.
    if (count == mDisplayCount)
        return;

    const int displayedOld = qMin(mDisplayCount, mEntries.size());
    const int displayedNew = qMin(count, mEntries.size());

    if (displayedNew < displayedOld)
        beginRemoveRows(QModelIndex(), displayedNew - 1, displayedOld - 1);
    else if (displayedNew > displayedOld)
        beginInsertRows(QModelIndex(), displayedOld - 1, displayedNew - 1);

    mDisplayCount = count;

    if (displayedNew < displayedOld)
        endRemoveRows();
    else if (displayedNew > displayedOld)
        endInsertRows();
}
#endif

// =========== CompletionList ===========

CompletionList::CompletionList(QWidget *parent)
    : QTreeView(parent)
{
    setRootIsDecorated(false);
    setUniformRowHeights(true);
    setMaximumWidth(900);
    header()->hide();
    header()->setStretchLastSection(true);
    // This is too slow when done on all results
    //header()->setResizeMode(QHeaderView::ResizeToContents);
    setWindowFlags(Qt::ToolTip);
#ifdef Q_WS_MAC
    if (horizontalScrollBar())
        horizontalScrollBar()->setAttribute(Qt::WA_MacMiniSize);
    if (verticalScrollBar())
        verticalScrollBar()->setAttribute(Qt::WA_MacMiniSize);
#endif
}

void CompletionList::updatePreferredSize()
{
    //header()->setStretchLastSection(false);
    //updateGeometries();

    const QStyleOptionViewItem &option = viewOptions();
    const QSize shint = itemDelegate()->sizeHint(option, model()->index(0, 0));
#if 0
    const int visibleItems = model()->rowCount();

    // TODO: Look into enabling preferred height as well. Current problem is that this doesn't
    // work nicely from the user perspective if the list is popping up instead of down.
    //const int h = shint.height() * visibleItems;

    const QScrollBar *vscroll = verticalScrollBar();
    int preferredWidth = header()->length() + frameWidth() * 2
                         + (vscroll->isVisibleTo(this) ? vscroll->width() : 0);
    const int diff = preferredWidth - width();

    // Avoid resizing the list widget when there are no results or when the preferred size is
    // only a little smaller than our current size
    if (visibleItems == 0 || (diff > -100 && diff < 0))
        preferredWidth = width();
#endif

    m_preferredSize = QSize(730, //qMax(600, preferredWidth),
                            shint.height() * 17 + frameWidth() * 2);
    //header()->setStretchLastSection(true);
}

// =========== LocatorWidget ===========

LocatorWidget::LocatorWidget(LocatorPlugin *qop) :
     m_locatorPlugin(qop),
     m_locatorModel(new LocatorModel(this)),
     m_completionList(new CompletionList(this)),
     m_filterMenu(new QMenu(this)),
     m_refreshAction(new QAction(tr("Refresh"), this)),
     m_configureAction(new QAction(tr("Configure..."), this)),
     m_fileLineEdit(new Utils::FilterLineEdit)
{
    // Explicitly hide the completion list popup.
    m_completionList->hide();

    setFocusProxy(m_fileLineEdit);
    setWindowTitle(tr("Locate..."));
    resize(200, 90);
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    setSizePolicy(sizePolicy);
    setMinimumSize(QSize(200, 0));

    QHBoxLayout *layout = new QHBoxLayout(this);
    setLayout(layout);
    layout->setMargin(0);
    layout->addWidget(m_fileLineEdit);

    setWindowIcon(QIcon(QLatin1String(":/locator/images/locator.png")));
    QPixmap image(Core::Constants::ICON_MAGNIFIER);
    m_fileLineEdit->setButtonPixmap(Utils::FancyLineEdit::Left, image);
    m_fileLineEdit->setPlaceholderText(tr("Type to locate"));
    m_fileLineEdit->setButtonToolTip(Utils::FancyLineEdit::Left, tr("Options"));
    m_fileLineEdit->setFocusPolicy(Qt::ClickFocus);
    m_fileLineEdit->setButtonVisible(Utils::FancyLineEdit::Left, true);
    // We set click focus since otherwise you will always get two popups
    m_fileLineEdit->setButtonFocusPolicy(Utils::FancyLineEdit::Left, Qt::ClickFocus);
    m_fileLineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

    m_fileLineEdit->installEventFilter(this);
    this->installEventFilter(this);

    m_completionList->setModel(m_locatorModel);
    m_completionList->header()->resizeSection(0, 300);
    m_completionList->updatePreferredSize();
    m_completionList->resize(m_completionList->preferredSize());

    m_filterMenu->addAction(m_refreshAction);
    m_filterMenu->addAction(m_configureAction);

    m_fileLineEdit->setButtonMenu(Utils::FancyLineEdit::Left, m_filterMenu);

    connect(m_refreshAction, SIGNAL(triggered()), m_locatorPlugin, SLOT(refresh()));
    connect(m_configureAction, SIGNAL(triggered()), this, SLOT(showConfigureDialog()));
    connect(m_fileLineEdit, SIGNAL(textChanged(const QString&)),
        this, SLOT(showPopup()));
    connect(m_completionList, SIGNAL(activated(QModelIndex)),
            this, SLOT(acceptCurrentEntry()));

    m_entriesWatcher = new QFutureWatcher<FilterEntry>(this);
    connect(m_entriesWatcher, SIGNAL(finished()), SLOT(updateEntries()));

    m_showPopupTimer = new QTimer(this);
    m_showPopupTimer->setInterval(100);
    m_showPopupTimer->setSingleShot(true);
    connect(m_showPopupTimer, SIGNAL(timeout()), SLOT(showPopupNow()));
}

void LocatorWidget::updateFilterList()
{
    m_filterMenu->clear();

    // update actions and menu
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    QMap<QString, QAction *> actionCopy = m_filterActionMap;
    m_filterActionMap.clear();
    // register new actions, update existent
    foreach (ILocatorFilter *filter, m_locatorPlugin->filters()) {
        if (filter->shortcutString().isEmpty() || filter->isHidden())
            continue;
        QString locatorId = QLatin1String("Locator.") + filter->id();
        QAction *action = 0;
        Core::Command *cmd = 0;
        if (!actionCopy.contains(filter->id())) {
            // register new action
            action = new QAction(filter->displayName(), this);
            cmd = am->registerAction(action, locatorId,
                               Core::Context(Core::Constants::C_GLOBAL));
            cmd->setAttribute(Core::Command::CA_UpdateText);
            connect(action, SIGNAL(triggered()), this, SLOT(filterSelected()));
            action->setData(qVariantFromValue(filter));
        } else {
            action = actionCopy.take(filter->id());
            action->setText(filter->displayName());
            cmd = am->command(locatorId);
        }
        m_filterActionMap.insert(filter->id(), action);
        m_filterMenu->addAction(cmd->action());
    }

    // unregister actions that are deleted now
    foreach (const QString &id, actionCopy.keys()) {
        am->unregisterAction(actionCopy.value(id), QString(QLatin1String("Locator.") + id));
    }
    qDeleteAll(actionCopy);

    m_filterMenu->addSeparator();
    m_filterMenu->addAction(m_refreshAction);
    m_filterMenu->addAction(m_configureAction);
}

bool LocatorWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_fileLineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            showCompletionList();
            QApplication::sendEvent(m_completionList, event);
            return true;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            acceptCurrentEntry();
            return true;
        case Qt::Key_Escape:
            m_completionList->hide();
            return true;
        case Qt::Key_Tab:
            m_completionList->next();
            return true;
        case Qt::Key_Backtab:
            m_completionList->previous();
            return true;
        default:
            break;
        }
    } else if (obj == m_fileLineEdit && event->type() == QEvent::FocusOut) {
#if defined(Q_OS_WIN)
        QFocusEvent *fev = static_cast<QFocusEvent*>(event);
        if (fev->reason() != Qt::ActiveWindowFocusReason ||
            (fev->reason() == Qt::ActiveWindowFocusReason && !m_completionList->isActiveWindow()))
#endif
            m_completionList->hide();
    } else if (obj == m_fileLineEdit && event->type() == QEvent::FocusIn) {
        showPopupNow();
    } else if (obj == this && event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            event->accept();
            QTimer::singleShot(0, Core::ModeManager::instance(), SLOT(setFocusToCurrentMode()));
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void LocatorWidget::showCompletionList()
{
    const int border = m_completionList->frameWidth();
    const QSize size = m_completionList->preferredSize();
    const QRect rect(mapToGlobal(QPoint(-border, -size.height() - border)), size);
    m_completionList->setGeometry(rect);
    m_completionList->show();
}

void LocatorWidget::showPopup()
{
    m_showPopupTimer->start();
}

void LocatorWidget::showPopupNow()
{
    m_showPopupTimer->stop();
    updateCompletionList(m_fileLineEdit->text());
    showCompletionList();
}

QList<ILocatorFilter*> LocatorWidget::filtersFor(const QString &text, QString &searchText)
{
    QList<ILocatorFilter*> filters = m_locatorPlugin->filters();
    const int whiteSpace = text.indexOf(QLatin1Char(' '));
    QString prefix;
    if (whiteSpace >= 0)
        prefix = text.left(whiteSpace);
    if (!prefix.isEmpty()) {
        prefix = prefix.toLower();
        QList<ILocatorFilter *> prefixFilters;
        foreach (ILocatorFilter *filter, filters) {
            if (prefix == filter->shortcutString()) {
                searchText = text.mid(whiteSpace+1);
                prefixFilters << filter;
            }
        }
        if (!prefixFilters.isEmpty())
            return prefixFilters;
    }
    searchText = text;
    QList<ILocatorFilter*> activeFilters;
    foreach (ILocatorFilter *filter, filters)
        if (filter->isIncludedByDefault())
            activeFilters << filter;
    return activeFilters;
}

static void filter_helper(QFutureInterface<Locator::FilterEntry> &entries, QList<ILocatorFilter *> filters, QString searchText)
{
    QSet<FilterEntry> alreadyAdded;
    const bool checkDuplicates = (filters.size() > 1);
    foreach (ILocatorFilter *filter, filters) {
        if (entries.isCanceled())
            break;

        foreach (const FilterEntry &entry, filter->matchesFor(entries, searchText)) {
            if (checkDuplicates && alreadyAdded.contains(entry))
                continue;
            entries.reportResult(entry);
            if (checkDuplicates)
                alreadyAdded.insert(entry);
        }
    }
}

void LocatorWidget::updateCompletionList(const QString &text)
{
    QString searchText;
    const QList<ILocatorFilter*> filters = filtersFor(text, searchText);

    // cancel the old future
    m_entriesWatcher->future().cancel();
    m_entriesWatcher->future().waitForFinished();

    QFuture<FilterEntry> future = QtConcurrent::run(filter_helper, filters, searchText);
    m_entriesWatcher->setFuture(future);
}

void LocatorWidget::updateEntries()
{
    if (m_entriesWatcher->future().isCanceled())
        return;

    const QList<FilterEntry> entries = m_entriesWatcher->future().results();
    m_locatorModel->setEntries(entries);
    if (m_locatorModel->rowCount() > 0) {
        m_completionList->setCurrentIndex(m_locatorModel->index(0, 0));
    }
#if 0
    m_completionList->updatePreferredSize();
#endif
}

void LocatorWidget::acceptCurrentEntry()
{
    if (!m_completionList->isVisible())
        return;
    const QModelIndex index = m_completionList->currentIndex();
    if (!index.isValid())
        return;
    const FilterEntry entry = m_locatorModel->data(index, Qt::UserRole).value<FilterEntry>();
    m_completionList->hide();
    entry.filter->accept(entry);
}

void LocatorWidget::show(const QString &text, int selectionStart, int selectionLength)
{
    if (!text.isEmpty())
        m_fileLineEdit->setText(text);
    if (!m_fileLineEdit->hasFocus())
        m_fileLineEdit->setFocus();
    else
        showPopupNow();

    if (selectionStart >= 0) {
        m_fileLineEdit->setSelection(selectionStart, selectionLength);
        if (selectionLength == 0) // make sure the cursor is at the right position (Mac-vs.-rest difference)
            m_fileLineEdit->setCursorPosition(selectionStart);
    } else {
        m_fileLineEdit->selectAll();
    }
}

void LocatorWidget::filterSelected()
{
    QString searchText = tr("<type here>");
    QAction *action = qobject_cast<QAction*>(sender());
    QTC_ASSERT(action, return);
    ILocatorFilter *filter = action->data().value<ILocatorFilter*>();
    QTC_ASSERT(filter, return);
    QString currentText = m_fileLineEdit->text().trimmed();
    // add shortcut string at front or replace existing shortcut string
    if (!currentText.isEmpty()) {
        searchText = currentText;
        foreach (ILocatorFilter *otherfilter, m_locatorPlugin->filters()) {
            if (currentText.startsWith(otherfilter->shortcutString() + QLatin1Char(' '))) {
                searchText = currentText.mid(otherfilter->shortcutString().length()+1);
                break;
            }
        }
    }
    show(filter->shortcutString() + QLatin1Char(' ') + searchText,
         filter->shortcutString().length() + 1,
         searchText.length());
    updateCompletionList(m_fileLineEdit->text());
    m_fileLineEdit->setFocus();
}

void LocatorWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

void LocatorWidget::showConfigureDialog()
{
    Core::ICore::instance()->showOptionsDialog(Constants::LOCATOR_CATEGORY,
          Constants::FILTER_OPTIONS_PAGE);
}
