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

#include "taskwindow.h"

#include "itaskhandler.h"
#include "projectexplorerconstants.h"
#include "task.h"
#include "taskhub.h"
#include "taskmodel.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>
#include <QtGui/QListView>
#include <QtGui/QPainter>
#include <QtGui/QStyledItemDelegate>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QMenu>
#include <QtGui/QToolButton>

namespace {
const int ELLIPSIS_GRADIENT_WIDTH = 16;
}

namespace ProjectExplorer {
namespace Internal {

class TaskView : public QListView
{
public:
    TaskView(QWidget *parent = 0);
    ~TaskView();
    void resizeEvent(QResizeEvent *e);
    void keyPressEvent(QKeyEvent *e);
};

class TaskWindowContext : public Core::IContext
{
public:
    TaskWindowContext(QWidget *widget);
};

class TaskDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    TaskDelegate(QObject * parent = 0);
    ~TaskDelegate();
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    // TaskView uses this method if the size of the taskview changes
    void emitSizeHintChanged(const QModelIndex &index);

public slots:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    void generateGradientPixmap(int width, int height, QColor color, bool selected) const;

    mutable int m_cachedHeight;
    mutable QFont m_cachedFont;

    /*
      Collapsed:
      +----------------------------------------------------------------------------------------------------+
      | TASKICONAREA  TEXTAREA                                                           FILEAREA LINEAREA |
      +----------------------------------------------------------------------------------------------------+

      Expanded:
      +----------------------------------------------------------------------------------------------------+
      | TASKICONICON  TEXTAREA                                                           FILEAREA LINEAREA |
      |               more text -------------------------------------------------------------------------> |
      +----------------------------------------------------------------------------------------------------+
     */
    class Positions
    {
    public:
        Positions(const QStyleOptionViewItemV4 &options, TaskModel *model) :
            m_totalWidth(options.rect.width()),
            m_maxFileLength(model->sizeOfFile(options.font)),
            m_maxLineLength(model->sizeOfLineNumber(options.font)),
            m_realFileLength(m_maxFileLength),
            m_top(options.rect.top()),
            m_bottom(options.rect.bottom())
        {
            int flexibleArea = lineAreaLeft() - textAreaLeft() - ITEM_SPACING;
            if (m_maxFileLength > flexibleArea / 2)
                m_realFileLength = flexibleArea / 2;
            m_fontHeight = QFontMetrics(options.font).height();
        }

        int top() const { return m_top + ITEM_MARGIN; }
        int left() const { return ITEM_MARGIN; }
        int right() const { return m_totalWidth - ITEM_MARGIN; }
        int bottom() const { return m_bottom; }
        int firstLineHeight() const { return m_fontHeight + 1; }
        int minimumHeight() const { return taskIconHeight() + 2 * ITEM_MARGIN; }

        int taskIconLeft() const { return left(); }
        int taskIconWidth() const { return TASK_ICON_SIZE; }
        int taskIconHeight() const { return TASK_ICON_SIZE; }
        int taskIconRight() const { return taskIconLeft() + taskIconWidth(); }
        QRect taskIcon() const { return QRect(taskIconLeft(), top(), taskIconWidth(), taskIconHeight()); }

        int textAreaLeft() const { return taskIconRight() + ITEM_SPACING; }
        int textAreaWidth() const { return textAreaRight() - textAreaLeft(); }
        int textAreaRight() const { return fileAreaLeft() - ITEM_SPACING; }
        QRect textArea() const { return QRect(textAreaLeft(), top(), textAreaWidth(), firstLineHeight()); }

        int fileAreaLeft() const { return fileAreaRight() - fileAreaWidth(); }
        int fileAreaWidth() const { return m_realFileLength; }
        int fileAreaRight() const { return lineAreaLeft() - ITEM_SPACING; }
        QRect fileArea() const { return QRect(fileAreaLeft(), top(), fileAreaWidth(), firstLineHeight()); }

        int lineAreaLeft() const { return lineAreaRight() - lineAreaWidth(); }
        int lineAreaWidth() const { return m_maxLineLength; }
        int lineAreaRight() const { return right(); }
        QRect lineArea() const { return QRect(lineAreaLeft(), top(), lineAreaWidth(), firstLineHeight()); }

    private:
        int m_totalWidth;
        int m_maxFileLength;
        int m_maxLineLength;
        int m_realFileLength;
        int m_top;
        int m_bottom;
        int m_fontHeight;

        static const int TASK_ICON_SIZE = 16;
        static const int ITEM_MARGIN = 2;
        static const int ITEM_SPACING = 2 * ITEM_MARGIN;
    };
};

TaskView::TaskView(QWidget *parent)
    : QListView(parent)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

TaskView::~TaskView()
{

}

void TaskView::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e)
    static_cast<TaskDelegate *>(itemDelegate())->emitSizeHintChanged(selectionModel()->currentIndex());
}

void TaskView::keyPressEvent(QKeyEvent *e)
{
    if (!e->modifiers() && e->key() == Qt::Key_Return) {
        emit activated(currentIndex());
        e->accept();
        return;
    }
    QListView::keyPressEvent(e);
}

/////
// TaskWindow
/////

class TaskWindowPrivate
{
public:
    Internal::TaskModel *m_model;
    Internal::TaskFilterModel *m_filter;
    Internal::TaskView *m_listview;
    Internal::TaskWindowContext *m_taskWindowContext;
    QMenu *m_contextMenu;
    QModelIndex m_contextMenuIndex;
    ITaskHandler *m_defaultHandler;
    QToolButton *m_filterWarningsButton;
    QToolButton *m_categoriesButton;
    QMenu *m_categoriesMenu;
    TaskHub *m_taskHub;
};

static QToolButton *createFilterButton(QIcon icon, const QString &toolTip,
                                       QObject *receiver, const char *slot)
{
    QToolButton *button = new QToolButton;
    button->setIcon(icon);
    button->setToolTip(toolTip);
    button->setCheckable(true);
    button->setChecked(true);
    button->setAutoRaise(true);
    button->setEnabled(true);
    QObject::connect(button, SIGNAL(toggled(bool)), receiver, slot);
    return button;
}

TaskWindow::TaskWindow(TaskHub *taskhub) : d(new TaskWindowPrivate)
{
    d->m_defaultHandler = 0;

    d->m_model = new Internal::TaskModel(this);
    d->m_filter = new Internal::TaskFilterModel(d->m_model);
    d->m_listview = new Internal::TaskView;

    d->m_listview->setModel(d->m_filter);
    d->m_listview->setFrameStyle(QFrame::NoFrame);
    d->m_listview->setWindowTitle(tr("Issues"));
    d->m_listview->setSelectionMode(QAbstractItemView::SingleSelection);
    Internal::TaskDelegate *tld = new Internal::TaskDelegate(this);
    d->m_listview->setItemDelegate(tld);
    d->m_listview->setWindowIcon(QIcon(QLatin1String(Constants::ICON_WINDOW)));
    d->m_listview->setContextMenuPolicy(Qt::ActionsContextMenu);
    d->m_listview->setAttribute(Qt::WA_MacShowFocusRect, false);

    d->m_taskWindowContext = new Internal::TaskWindowContext(d->m_listview);
    d->m_taskHub = taskhub;

    Core::ICore::instance()->addContextObject(d->m_taskWindowContext);

    connect(d->m_listview->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            tld, SLOT(currentChanged(QModelIndex,QModelIndex)));

    connect(d->m_listview, SIGNAL(activated(QModelIndex)),
            this, SLOT(triggerDefaultHandler(QModelIndex)));

    d->m_contextMenu = new QMenu(d->m_listview);
    connect(d->m_contextMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(contextMenuEntryTriggered(QAction*)));

    d->m_listview->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(d->m_listview, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(showContextMenu(QPoint)));

    d->m_filterWarningsButton = createFilterButton(d->m_model->taskTypeIcon(Task::Warning),
                                                   tr("Show Warnings"),
                                                   this, SLOT(setShowWarnings(bool)));

    d->m_categoriesButton = new QToolButton;
    d->m_categoriesButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_FILTER)));
    d->m_categoriesButton->setToolTip(tr("Filter by categories"));
    d->m_categoriesButton->setAutoRaise(true);
    d->m_categoriesButton->setPopupMode(QToolButton::InstantPopup);

    d->m_categoriesMenu = new QMenu(d->m_categoriesButton);
    connect(d->m_categoriesMenu, SIGNAL(aboutToShow()), this, SLOT(updateCategoriesMenu()));
    connect(d->m_categoriesMenu, SIGNAL(triggered(QAction*)), this, SLOT(filterCategoryTriggered(QAction*)));

    d->m_categoriesButton->setMenu(d->m_categoriesMenu);

    connect(d->m_taskHub, SIGNAL(categoryAdded(QString, QString, bool)),
            this, SLOT(addCategory(QString, QString, bool)));
    connect(d->m_taskHub, SIGNAL(taskAdded(ProjectExplorer::Task)),
            this, SLOT(addTask(ProjectExplorer::Task)));
    connect(d->m_taskHub, SIGNAL(taskRemoved(ProjectExplorer::Task)),
            this, SLOT(removeTask(ProjectExplorer::Task)));
    connect(d->m_taskHub, SIGNAL(tasksCleared(QString)),
            this, SLOT(clearTasks(QString)));
    connect(d->m_taskHub, SIGNAL(categoryVisibilityChanged(QString,bool)),
            this, SLOT(setCategoryVisibility(QString,bool)));
    connect(d->m_taskHub, SIGNAL(popupRequested(bool)),
            this, SLOT(popup(bool)));
}

TaskWindow::~TaskWindow()
{
    Core::ICore::instance()->removeContextObject(d->m_taskWindowContext);
    cleanContextMenu();
    delete d->m_filterWarningsButton;
    delete d->m_listview;
    delete d->m_filter;
    delete d->m_model;
    delete d;
}

QList<QWidget*> TaskWindow::toolBarWidgets() const
{
    return QList<QWidget*>() << d->m_filterWarningsButton << d->m_categoriesButton;
}

QWidget *TaskWindow::outputWidget(QWidget *)
{
    return d->m_listview;
}

void TaskWindow::clearTasks(const QString &categoryId)
{
    d->m_model->clearTasks(categoryId);

    emit tasksChanged();
    emit tasksCleared();
    navigateStateChanged();
}

void TaskWindow::setCategoryVisibility(const QString &categoryId, bool visible)
{
    if (categoryId.isEmpty())
        return;

    QStringList categories = d->m_filter->filteredCategories();

    if (visible) {
        categories.removeOne(categoryId);
    } else {
        categories.append(categoryId);
    }

    d->m_filter->setFilteredCategories(categories);
}

void TaskWindow::visibilityChanged(bool /* b */)
{
}

void TaskWindow::addCategory(const QString &categoryId, const QString &displayName, bool visible)
{
    d->m_model->addCategory(categoryId, displayName);
    if (!visible) {
        QStringList filters = d->m_filter->filteredCategories();
        filters += categoryId;
        d->m_filter->setFilteredCategories(filters);
    }
}

void TaskWindow::addTask(const Task &task)
{
    d->m_model->addTask(task);

    emit tasksChanged();
    navigateStateChanged();
}

void TaskWindow::removeTask(const Task &task)
{
    d->m_model->removeTask(task);

    emit tasksChanged();
    navigateStateChanged();
}

void TaskWindow::triggerDefaultHandler(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    // Find a default handler to use:
    if (!d->m_defaultHandler) {
        QList<ITaskHandler *> handlers = ExtensionSystem::PluginManager::instance()->getObjects<ITaskHandler>();
        foreach(ITaskHandler *handler, handlers) {
            if (handler->id() == QLatin1String(Constants::SHOW_TASK_IN_EDITOR)) {
                d->m_defaultHandler = handler;
                break;
            }
        }
    }
    Q_ASSERT(d->m_defaultHandler);
    Task task(d->m_filter->task(index));
    if (task.isNull())
        return;

    if (d->m_defaultHandler->canHandle(task)) {
        d->m_defaultHandler->handle(task);
    } else {
        if (!QFileInfo(task.file).exists())
            d->m_model->setFileNotFound(index, true);
    }
}

void TaskWindow::showContextMenu(const QPoint &position)
{
    QModelIndex index = d->m_listview->indexAt(position);
    if (!index.isValid())
        return;
    d->m_contextMenuIndex = index;
    cleanContextMenu();

    Task task = d->m_filter->task(index);
    if (task.isNull())
        return;

    QList<ITaskHandler *> handlers = ExtensionSystem::PluginManager::instance()->getObjects<ITaskHandler>();
    foreach(ITaskHandler *handler, handlers) {
        if (handler == d->m_defaultHandler)
            continue;
        QAction * action = handler->createAction(d->m_contextMenu);
        action->setEnabled(handler->canHandle(task));
        action->setData(qVariantFromValue(qobject_cast<QObject*>(handler)));
        d->m_contextMenu->addAction(action);
    }
    d->m_contextMenu->popup(d->m_listview->mapToGlobal(position));
}

void TaskWindow::contextMenuEntryTriggered(QAction *action)
{
    if (action->isEnabled()) {
        Task task = d->m_filter->task(d->m_contextMenuIndex);
        if (task.isNull())
            return;

        ITaskHandler *handler = qobject_cast<ITaskHandler*>(action->data().value<QObject*>());
        if (!handler)
            return;
        handler->handle(task);
    }
}

void TaskWindow::cleanContextMenu()
{
    QList<QAction *> actions = d->m_contextMenu->actions();
    qDeleteAll(actions);
    d->m_contextMenu->clear();
}

void TaskWindow::setShowWarnings(bool show)
{
    d->m_filter->setFilterIncludesWarnings(show);
    d->m_filter->setFilterIncludesUnknowns(show); // "Unknowns" are often associated with warnings
}

void TaskWindow::updateCategoriesMenu()
{
    d->m_categoriesMenu->clear();

    const QStringList filteredCategories = d->m_filter->filteredCategories();

    QMap<QString, QString> nameToIds;
    foreach (const QString &categoryId, d->m_model->categoryIds())
        nameToIds.insert(d->m_model->categoryDisplayName(categoryId), categoryId);

    foreach (const QString &displayName, nameToIds.keys()) {
        const QString categoryId = nameToIds.value(displayName);
        QAction *action = new QAction(d->m_categoriesMenu);
        action->setCheckable(true);
        action->setText(displayName);
        action->setData(categoryId);
        action->setChecked(!filteredCategories.contains(categoryId));
        d->m_categoriesMenu->addAction(action);
    }
}

void TaskWindow::filterCategoryTriggered(QAction *action)
{
    QString categoryId = action->data().toString();
    Q_ASSERT(!categoryId.isEmpty());

    setCategoryVisibility(categoryId, action->isChecked());
}

int TaskWindow::taskCount(const QString &category) const
{
    return d->m_model->taskCount(category);
}

int TaskWindow::errorTaskCount(const QString &category) const
{
    return d->m_model->errorTaskCount(category);
}

int TaskWindow::warningTaskCount(const QString &category) const
{
    return d->m_model->warningTaskCount(category);
}

int TaskWindow::priorityInStatusBar() const
{
    return 90;
}

void TaskWindow::clearContents()
{
    // clear all tasks in all displays
    // Yeah we are that special
    d->m_taskHub->clearTasks(QString());
}

bool TaskWindow::hasFocus() const
{
    return d->m_listview->hasFocus();
}

bool TaskWindow::canFocus() const
{
    return d->m_filter->rowCount();
}

void TaskWindow::setFocus()
{
    if (d->m_filter->rowCount()) {
        d->m_listview->setFocus();
        if (d->m_listview->currentIndex() == QModelIndex()) {
            d->m_listview->setCurrentIndex(d->m_filter->index(0,0, QModelIndex()));
        }
    }
}

bool TaskWindow::canNext() const
{
    return d->m_filter->rowCount();
}

bool TaskWindow::canPrevious() const
{
    return d->m_filter->rowCount();
}

void TaskWindow::goToNext()
{
    if (!canNext())
        return;
    QModelIndex startIndex = d->m_listview->currentIndex();
    QModelIndex currentIndex = startIndex;

    if (startIndex.isValid()) {
        do {
            int row = currentIndex.row() + 1;
            if (row == d->m_filter->rowCount())
                row = 0;
            currentIndex = d->m_filter->index(row, 0);
            if (d->m_filter->hasFile(currentIndex))
                break;
        } while (startIndex != currentIndex);
    } else {
        currentIndex = d->m_filter->index(0, 0);
    }
    d->m_listview->setCurrentIndex(currentIndex);
    triggerDefaultHandler(currentIndex);
}

void TaskWindow::goToPrev()
{
    if (!canPrevious())
        return;
    QModelIndex startIndex = d->m_listview->currentIndex();
    QModelIndex currentIndex = startIndex;

    if (startIndex.isValid()) {
        do {
            int row = currentIndex.row() - 1;
            if (row < 0)
                row = d->m_filter->rowCount() - 1;
            currentIndex = d->m_filter->index(row, 0);
            if (d->m_filter->hasFile(currentIndex))
                break;
        } while (startIndex != currentIndex);
    } else {
        currentIndex = d->m_filter->index(0, 0);
    }
    d->m_listview->setCurrentIndex(currentIndex);
    triggerDefaultHandler(currentIndex);
}

bool TaskWindow::canNavigate() const
{
    return true;
}

/////
// Delegate
/////

TaskDelegate::TaskDelegate(QObject *parent) :
    QStyledItemDelegate(parent),
    m_cachedHeight(0)
{ }

TaskDelegate::~TaskDelegate()
{
}

QSize TaskDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);

    const QAbstractItemView * view = qobject_cast<const QAbstractItemView *>(opt.widget);
    const bool selected = (view->selectionModel()->currentIndex() == index);
    QSize s;
    s.setWidth(option.rect.width());

    if (!selected && option.font == m_cachedFont && m_cachedHeight > 0) {
        s.setHeight(m_cachedHeight);
        return s;
    }

    QFontMetrics fm(option.font);
    int fontHeight = fm.height();
    int fontLeading = fm.leading();

    TaskModel *model = static_cast<TaskFilterModel *>(view->model())->taskModel();
    Positions positions(option, model);

    if (selected) {
        QString description = index.data(TaskModel::Description).toString();
        // Layout the description
        int leading = fontLeading;
        int height = 0;
        description.replace(QLatin1Char('\n'), QChar::LineSeparator);
        QTextLayout tl(description);
        tl.beginLayout();
        while (true) {
            QTextLine line = tl.createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(positions.textAreaWidth());
            height += leading;
            line.setPosition(QPoint(0, height));
            height += static_cast<int>(line.height());
        }
        tl.endLayout();

        s.setHeight(height + leading + fontHeight + 3);
    } else {
        s.setHeight(fontHeight + 3);
    }
    if (s.height() < positions.minimumHeight())
        s.setHeight(positions.minimumHeight());

    if (!selected) {
        m_cachedHeight = s.height();
        m_cachedFont = option.font;
    }

    return s;
}

void TaskDelegate::emitSizeHintChanged(const QModelIndex &index)
{
    emit sizeHintChanged(index);
}

void TaskDelegate::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    emit sizeHintChanged(current);
    emit sizeHintChanged(previous);
}

void TaskDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    painter->save();

    QFontMetrics fm(opt.font);
    QColor backgroundColor;
    QColor textColor;

    const QAbstractItemView * view = qobject_cast<const QAbstractItemView *>(opt.widget);
    bool selected = view->selectionModel()->currentIndex() == index;

    if (selected) {
        painter->setBrush(opt.palette.highlight().color());
        backgroundColor = opt.palette.highlight().color();
    } else {
        painter->setBrush(opt.palette.background().color());
        backgroundColor = opt.palette.background().color();
    }
    painter->setPen(Qt::NoPen);
    painter->drawRect(opt.rect);

    // Set Text Color
    if (selected)
        textColor = opt.palette.highlightedText().color();
    else
        textColor = opt.palette.text().color();

    painter->setPen(textColor);

    TaskModel *model = static_cast<TaskFilterModel *>(view->model())->taskModel();
    Positions positions(opt, model);

    // Paint TaskIconArea:
    QIcon icon = index.data(TaskModel::Icon).value<QIcon>();
    painter->drawPixmap(positions.left(), positions.top(),
                        icon.pixmap(positions.taskIconWidth(), positions.taskIconHeight()));

    // Paint TextArea:
    if (!selected) {
        // in small mode we lay out differently
        QString bottom = index.data(TaskModel::Description).toString().split(QLatin1Char('\n')).first();
        painter->setClipRect(positions.textArea());
        painter->drawText(positions.textAreaLeft(), positions.top() + fm.ascent(), bottom);
        if (fm.width(bottom) > positions.textAreaWidth()) {
            // draw a gradient to mask the text
            int gradientStart = positions.textAreaRight() - ELLIPSIS_GRADIENT_WIDTH + 1;
            QLinearGradient lg(gradientStart, 0, gradientStart + ELLIPSIS_GRADIENT_WIDTH, 0);
            lg.setColorAt(0, Qt::transparent);
            lg.setColorAt(1, backgroundColor);
            painter->fillRect(gradientStart, positions.top(), ELLIPSIS_GRADIENT_WIDTH, positions.firstLineHeight(), lg);
        }
    } else {
        // Description
        QString description = index.data(TaskModel::Description).toString();
        // Layout the description
        int leading = fm.leading();
        int height = 0;
        description.replace(QLatin1Char('\n'), QChar::LineSeparator);
        QTextLayout tl(description);
        tl.setAdditionalFormats(index.data(TaskModel::Task_t).value<ProjectExplorer::Task>().formats);
        tl.beginLayout();
        while (true) {
            QTextLine line = tl.createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(positions.textAreaWidth());
            height += leading;
            line.setPosition(QPoint(0, height));
            height += static_cast<int>(line.height());
        }
        tl.endLayout();
        tl.draw(painter, QPoint(positions.textAreaLeft(), positions.top()));

        QColor mix;
        mix.setRgb( static_cast<int>(0.7 * textColor.red()   + 0.3 * backgroundColor.red()),
                static_cast<int>(0.7 * textColor.green() + 0.3 * backgroundColor.green()),
                static_cast<int>(0.7 * textColor.blue()  + 0.3 * backgroundColor.blue()));
        painter->setPen(mix);

        const QString directory = QDir::toNativeSeparators(index.data(TaskModel::File).toString());
        int secondBaseLine = positions.top() + fm.ascent() + height + leading;
        if (index.data(TaskModel::FileNotFound).toBool()) {
            QString fileNotFound = tr("File not found: %1").arg(directory);
            painter->setPen(Qt::red);
            painter->drawText(positions.textAreaLeft(), secondBaseLine, fileNotFound);
        } else {
            painter->drawText(positions.textAreaLeft(), secondBaseLine, directory);
        }
    }
    painter->setPen(textColor);

    // Paint FileArea
    QString file = index.data(TaskModel::File).toString();
    const int pos = file.lastIndexOf(QLatin1Char('/'));
    if (pos != -1)
        file = file.mid(pos +1);
    const int realFileWidth = fm.width(file);
    painter->setClipRect(positions.fileArea());
    painter->drawText(qMin(positions.fileAreaLeft(), positions.fileAreaRight() - realFileWidth),
                      positions.top() + fm.ascent(), file);
    if (realFileWidth > positions.fileAreaWidth()) {
        // draw a gradient to mask the text
        int gradientStart = positions.fileAreaLeft() - 1;
        QLinearGradient lg(gradientStart + ELLIPSIS_GRADIENT_WIDTH, 0, gradientStart, 0);
        lg.setColorAt(0, Qt::transparent);
        lg.setColorAt(1, backgroundColor);
        painter->fillRect(gradientStart, positions.top(), ELLIPSIS_GRADIENT_WIDTH, positions.firstLineHeight(), lg);
    }

    // Paint LineArea
    QString lineText = index.data(TaskModel::Line).toString();
    painter->setClipRect(positions.lineArea());
    const int realLineWidth = fm.width(lineText);
    painter->drawText(positions.lineAreaRight() - realLineWidth, positions.top() + fm.ascent(), lineText);
    painter->setClipRect(opt.rect);

    // Separator lines
    painter->setPen(QColor::fromRgb(150,150,150));
    painter->drawLine(0, opt.rect.bottom(), opt.rect.right(), opt.rect.bottom());
    painter->restore();
}

TaskWindowContext::TaskWindowContext(QWidget *widget)
  : Core::IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Core::Constants::C_PROBLEM_PANE));
}

} // namespace Internal
} // namespace ProjectExplorer

#include "taskwindow.moc"
