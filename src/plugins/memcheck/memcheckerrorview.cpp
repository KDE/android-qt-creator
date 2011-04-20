/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#include "memcheckerrorview.h"

#include "suppressiondialog.h"

#include <valgrind/xmlprotocol/error.h>
#include <valgrind/xmlprotocol/errorlistmodel.h>
#include <valgrind/xmlprotocol/frame.h>
#include <valgrind/xmlprotocol/stack.h>
#include <valgrind/xmlprotocol/modelhelpers.h>
#include <valgrind/xmlprotocol/suppression.h>

#include <valgrindtoolbase/valgrindsettings.h>

#include <texteditor/basetexteditor.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <coreplugin/coreconstants.h>

#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QDebug>

#include <QtGui/QLabel>
#include <QtGui/QListView>
#include <QtGui/QPainter>
#include <QtGui/QScrollBar>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QAction>
#include <QtGui/QClipboard>
#include <QtGui/QApplication>
#include <QtGui/QMenu>

using namespace Analyzer;
using namespace Analyzer::Internal;
using namespace Valgrind::XmlProtocol;

MemcheckErrorDelegate::MemcheckErrorDelegate(QListView *parent)
    : QStyledItemDelegate(parent),
      m_detailsWidget(0)
{
    connect(parent->verticalScrollBar(), SIGNAL(valueChanged(int)),
            SLOT(verticalScrolled()));
}

MemcheckErrorDelegate::~MemcheckErrorDelegate()
{
}

QSize MemcheckErrorDelegate::sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    const QListView *view = qobject_cast<const QListView *>(parent());
    const int viewportWidth = view->viewport()->width();
    const bool isSelected = view->selectionModel()->currentIndex() == index;

    int dy = 2 * s_itemMargin;

    if (!isSelected) {
        QFontMetrics fm(opt.font);
        return QSize(viewportWidth, fm.height() + dy);
    }

    if (m_detailsWidget && m_detailsIndex != index) {
        m_detailsWidget->deleteLater();
        m_detailsWidget = 0;
    }

    if (!m_detailsWidget) {
        m_detailsWidget = createDetailsWidget(index, view->viewport());
        QTC_ASSERT(m_detailsWidget->parent() == view->viewport(),
                   m_detailsWidget->setParent(view->viewport()));
        m_detailsIndex = index;
    } else {
        QTC_ASSERT(m_detailsIndex == index, qt_noop());
    }
    const int widthExcludingMargins = viewportWidth - 2 * s_itemMargin;
    m_detailsWidget->setFixedWidth(widthExcludingMargins);

    m_detailsWidgetHeight = m_detailsWidget->heightForWidth(widthExcludingMargins);
    // HACK: it's a bug in QLabel(?) that we have to force the widget to have the size it said
    //       it would have.
    m_detailsWidget->setFixedHeight(m_detailsWidgetHeight);
    return QSize(viewportWidth, dy + m_detailsWidget->heightForWidth(widthExcludingMargins));
}

static QString makeFrameName(const Frame &frame, const QString &relativeTo,
                             bool link = true, const QString &linkAttr = QString())
{
    const QString d = frame.directory();
    const QString f = frame.file();
    const QString fn = frame.functionName();
    const QString fullPath = d + QDir::separator() + f;

    QString path;
    if (!d.isEmpty() && !f.isEmpty())
        path = fullPath;
    else
        path = frame.object();

    if (QFile::exists(path))
        path = QFileInfo(path).canonicalFilePath();

    if (path.startsWith(relativeTo))
        path.remove(0, relativeTo.length());

    if (frame.line() != -1)
        path += QLatin1Char(':') + QString::number(frame.line());

    path = Qt::escape(path);

    if (link && !f.isEmpty() && QFile::exists(fullPath)) {
        // make a hyperlink label
        path = QString("<a href=\"file://%1:%2\" %4>%3</a>")
                    .arg(fullPath, QString::number(frame.line()), path, linkAttr);
    }

    if (!fn.isEmpty())
        return QCoreApplication::translate("Analyzer::Internal", "%1 in %2").arg(Qt::escape(fn), path);
    else if (!path.isEmpty())
        return path;
    else
        return QString("0x%1").arg(frame.instructionPointer(), 0, 16);
}

QString relativeToPath()
{
    // project for which we insert the snippet
    const ProjectExplorer::Project *project =
            ProjectExplorer::ProjectExplorerPlugin::instance()->startupProject();

    QString relativeTo( project ? project->projectDirectory() : QDir::homePath() );
    if (!relativeTo.endsWith(QDir::separator()))
        relativeTo.append(QDir::separator());

    return relativeTo;
}

QString errorLocation(const QModelIndex &index, const Error &error,
                      bool link = false, const QString &linkAttr = QString())
{
    const ErrorListModel *model = 0;
    const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel*>(index.model());
    while(!model && proxy) {
        model = qobject_cast<const ErrorListModel*>(proxy->sourceModel());
        proxy = qobject_cast<const QAbstractProxyModel*>(proxy->sourceModel());
    };
    QTC_ASSERT(model, return QString());

    return QCoreApplication::translate("Analyzer::Internal", "in %1").
            arg(makeFrameName(model->findRelevantFrame(error), relativeToPath(),
                              link, linkAttr));
}

QWidget *MemcheckErrorDelegate::createDetailsWidget(const QModelIndex &errorIndex, QWidget *parent) const
{
    QWidget *widget = new QWidget(parent);
    QVBoxLayout *layout = new QVBoxLayout;
    // code + white-space:pre so the padding (see below) works properly
    // don't include frameName here as it should wrap if required and pre-line is not supported
    // by Qt yet it seems
    const QString displayTextTemplate = QString("<code style='white-space:pre'>%1:</code> %2");

    QString relativeTo = relativeToPath();

    const Error error = errorIndex.data(ErrorListModel::ErrorRole).value<Error>();

    QLabel *errorLabel = new QLabel();
    errorLabel->setWordWrap(true);
    errorLabel->setContentsMargins(0, 0, 0, 0);
    errorLabel->setMargin(0);
    errorLabel->setIndent(0);
    QPalette p = errorLabel->palette();
    QColor lc = p.color(QPalette::Text);
    QString linkStyle = QString("style=\"color:rgba(%1, %2, %3, %4);\"")
                            .arg(lc.red()).arg(lc.green()).arg(lc.blue()).arg(int(0.7 * 255));
    p.setBrush(QPalette::Text, p.highlightedText());
    errorLabel->setPalette(p);
    errorLabel->setText(QString("%1&nbsp;&nbsp;<span %4>%2</span>")
                            .arg(error.what(), errorLocation(errorIndex, error, true, linkStyle),
                                 linkStyle));
    connect(errorLabel, SIGNAL(linkActivated(QString)), SLOT(openLinkInEditor(QString)));
    layout->addWidget(errorLabel);

    const QVector<Stack> stacks = error.stacks();
    for (int i = 0; i < stacks.count(); ++i) {
        const Stack &stack = stacks.at(i);
        // auxwhat for additional stacks
        if (i > 0) {
            QLabel *stackLabel = new QLabel(stack.auxWhat());
            stackLabel->setWordWrap(true);
            stackLabel->setContentsMargins(0, 0, 0, 0);
            stackLabel->setMargin(0);
            stackLabel->setIndent(0);
            QPalette p = stackLabel->palette();
            p.setBrush(QPalette::Text, p.highlightedText());
            stackLabel->setPalette(p);
            layout->addWidget(stackLabel);
        }
        int frameNr = 1;
        foreach (const Frame &frame, stack.frames()) {
            QString frameName = makeFrameName(frame, relativeTo);
            QTC_ASSERT(!frameName.isEmpty(), qt_noop());

            QLabel *frameLabel = new QLabel(widget);
            frameLabel->setAutoFillBackground(true);
            if (frameNr % 2 == 0) {
                // alternating rows
                QPalette p = frameLabel->palette();
                p.setBrush(QPalette::Base, p.alternateBase());
                frameLabel->setPalette(p);
            }
            frameLabel->setFont(QFont("monospace"));
            connect(frameLabel, SIGNAL(linkActivated(QString)), SLOT(openLinkInEditor(QString)));
            // pad frameNr to 2 chars since only 50 frames max are supported by valgrind
            const QString displayText = displayTextTemplate
                                            .arg(frameNr++, 2).arg(frameName);
            frameLabel->setText(displayText);

            frameLabel->setToolTip(Valgrind::XmlProtocol::toolTipForFrame(frame));
            frameLabel->setWordWrap(true);
            frameLabel->setContentsMargins(0, 0, 0, 0);
            frameLabel->setMargin(0);
            frameLabel->setIndent(10);
            layout->addWidget(frameLabel);
        }
    }

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    widget->setLayout(layout);
    return widget;
}

void MemcheckErrorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &basicOption,
                                  const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt(basicOption);
    initStyleOption(&opt, index);

    const QListView *const view = qobject_cast<const QListView *>(parent());
    const bool isSelected = view->selectionModel()->currentIndex() == index;

    QFontMetrics fm(opt.font);
    QPoint pos = opt.rect.topLeft();

    painter->save();

    const QColor bgColor = isSelected ? opt.palette.highlight().color() : opt.palette.background().color();
    painter->setBrush(bgColor);

    // clear background
    painter->setPen(Qt::NoPen);
    painter->drawRect(opt.rect);

    pos.rx() += s_itemMargin;
    pos.ry() += s_itemMargin;

    const Error error = index.data(ErrorListModel::ErrorRole).value<Error>();

    if (isSelected) {
        // only show detailed widget and let it handle everything
        QTC_ASSERT(m_detailsIndex == index, qt_noop());
        QTC_ASSERT(m_detailsWidget, return); // should have been set in sizeHint()
        m_detailsWidget->move(pos);
        // when scrolling quickly, the widget can get stuck in a visible part of the scroll area
        // even though it should not be visible. therefore we hide it every time the scroll value
        // changes and un-hide it when the item with details widget is paint()ed, i.e. visible.
        m_detailsWidget->show();

        const int viewportWidth = view->viewport()->width();
        const int widthExcludingMargins = viewportWidth - 2 * s_itemMargin;
        QTC_ASSERT(m_detailsWidget->width() == widthExcludingMargins, qt_noop());
        QTC_ASSERT(m_detailsWidgetHeight == m_detailsWidget->height(), qt_noop());
    } else {
        // the reference coordinate for text drawing is the text baseline; move it inside the view rect.
        pos.ry() += fm.ascent();

        const QColor textColor = opt.palette.text().color();
        painter->setPen(textColor);
        // draw only text + location
        const QString what = error.what();
        painter->drawText(pos, what);

        const QString name = errorLocation(index, error);
        const int whatWidth = QFontMetrics(opt.font).width(what);
        const int space = 10;
        const int widthLeft = opt.rect.width() - (pos.x() + whatWidth + space + s_itemMargin);
        if (widthLeft > 0) {
            QFont monospace = opt.font;
            monospace.setFamily("monospace");
            QFontMetrics metrics(monospace);
            QColor nameColor = textColor;
            nameColor.setAlphaF(0.7);

            painter->setFont(monospace);
            painter->setPen(nameColor);

            QPoint namePos = pos;
            namePos.rx() += whatWidth + space;
            painter->drawText(namePos, metrics.elidedText(name, Qt::ElideLeft, widthLeft));
        }
    }

    // Separator lines (like build issues pane)
    painter->setPen(QColor::fromRgb(150,150,150));
    painter->drawLine(0, opt.rect.bottom(), opt.rect.right(), opt.rect.bottom());

    painter->restore();
}

void MemcheckErrorDelegate::currentChanged(const QModelIndex &now, const QModelIndex &previous)
{
    if (m_detailsWidget) {
        m_detailsWidget->deleteLater();
        m_detailsWidget = 0;
    }

    m_detailsIndex = QModelIndex();
    if (now.isValid())
        emit sizeHintChanged(now);
    if (previous.isValid())
        emit sizeHintChanged(previous);
}

void MemcheckErrorDelegate::layoutChanged()
{
    if (m_detailsWidget) {
        m_detailsWidget->deleteLater();
        m_detailsWidget = 0;
        m_detailsIndex = QModelIndex();
    }
}

void MemcheckErrorDelegate::viewResized()
{
    const QListView *view = qobject_cast<const QListView *>(parent());
    if (m_detailsWidget)
        emit sizeHintChanged(view->selectionModel()->currentIndex());
}

void MemcheckErrorDelegate::verticalScrolled()
{
    if (m_detailsWidget)
        m_detailsWidget->hide();
}

void MemcheckErrorDelegate::copy()
{
    QTC_ASSERT(m_detailsIndex.isValid(), return);

    QString content;
    QTextStream stream(&content);
    const Error error = m_detailsIndex.data(ErrorListModel::ErrorRole).value<Error>();

    stream << error.what() << "\n";
    stream << "  " << errorLocation(m_detailsIndex, error) << "\n";

    const QString relativeTo = relativeToPath();

    foreach(const Stack &stack, error.stacks()) {
        if (!stack.auxWhat().isEmpty()) {
            stream << stack.auxWhat();
        }
        int i = 1;
        foreach(const Frame &frame, stack.frames()) {
            stream << "  " << i++ << ": " << makeFrameName(frame, relativeTo) << "\n";
        }
    }

    stream.flush();
    QApplication::clipboard()->setText(content);
}

void MemcheckErrorDelegate::openLinkInEditor(const QString &link)
{
    const int pathStart = strlen("file://");
    const int pathEnd = link.lastIndexOf(':');
    const QString path = link.mid(pathStart, pathEnd - pathStart);
    const int line = link.mid(pathEnd + 1).toInt(0);
    TextEditor::BaseTextEditorWidget::openEditorAt(path, qMax(line, 0));
}

MemcheckErrorView::MemcheckErrorView(QWidget *parent)
    : QListView(parent),
      m_settings(0)
{
    setItemDelegate(new MemcheckErrorDelegate(this));
    connect(this, SIGNAL(resized()), itemDelegate(), SLOT(viewResized()));

    m_copyAction = new QAction(this);
    m_copyAction->setText(tr("Copy Selection"));
    m_copyAction->setIcon(QIcon(Core::Constants::ICON_COPY));
    m_copyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    m_copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_copyAction, SIGNAL(triggered()), itemDelegate(), SLOT(copy()));
    addAction(m_copyAction);

    m_suppressAction = new QAction(this);
    m_suppressAction->setText(tr("Suppress Error"));
    m_suppressAction->setIcon(QIcon(QLatin1String(":/qmldesigner/images/eye_crossed.png")));
    m_suppressAction->setShortcut(QKeySequence(Qt::Key_Delete));
    m_suppressAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_suppressAction, SIGNAL(triggered()), this, SLOT(suppressError()));
    addAction(m_suppressAction);
}

MemcheckErrorView::~MemcheckErrorView()
{
    itemDelegate()->deleteLater();
}

void MemcheckErrorView::setModel(QAbstractItemModel *model)
{
    QListView::setModel(model);
    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)),
            itemDelegate(), SLOT(currentChanged(QModelIndex, QModelIndex)));

    connect(model, SIGNAL(layoutChanged()),
            itemDelegate(), SLOT(layoutChanged()));
}

void MemcheckErrorView::resizeEvent(QResizeEvent *e)
{
    emit resized();
    QListView::resizeEvent(e);
}

void MemcheckErrorView::setDefaultSuppressionFile(const QString &suppFile)
{
    m_defaultSuppFile = suppFile;
}

QString MemcheckErrorView::defaultSuppressionFile() const
{
    return m_defaultSuppFile;
}

// slot, can (for now) be invoked either when the settings were modified *or* when the active
// settings object has changed.
void MemcheckErrorView::settingsChanged(AnalyzerSettings *settings)
{
    QTC_ASSERT(settings, return);

    m_settings = settings;
}

void MemcheckErrorView::contextMenuEvent(QContextMenuEvent *e)
{
    const QModelIndexList indizes = selectionModel()->selectedRows();
    if (indizes.isEmpty())
        return;


    QList<Error> errors;
    foreach(const QModelIndex &index, indizes) {
        Error error = model()->data(index, ErrorListModel::ErrorRole).value<Error>();
        if (!error.suppression().isNull())
            errors << error;
    }

    QMenu menu;
    menu.addAction(m_copyAction);
    menu.addSeparator();
    menu.addAction(m_suppressAction);
    m_suppressAction->setEnabled(!errors.isEmpty());
    menu.exec(e->globalPos());
}

void MemcheckErrorView::suppressError()
{
    SuppressionDialog *dialog = new SuppressionDialog(this);
    if (dialog->shouldShow()) {
        dialog->setModal(true);
        dialog->show();
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    } else {
        delete dialog;
    }
}
