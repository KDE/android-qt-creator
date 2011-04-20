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

#include "qmljsoutline.h"
#include "qmloutlinemodel.h"
#include "qmljseditoreditable.h"
#include "qmljsoutlinetreeview.h"

#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QAction>
#include <QtGui/QVBoxLayout>
#include <QtGui/QTextBlock>

using namespace QmlJS;

enum {
    debug = false
};

namespace QmlJSEditor {
namespace Internal {


QmlJSOutlineFilterModel::QmlJSOutlineFilterModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

bool QmlJSOutlineFilterModel::filterAcceptsRow(int sourceRow,
                                               const QModelIndex &sourceParent) const
{
    if (m_filterBindings) {
        QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        QVariant itemType = sourceIndex.data(QmlOutlineModel::ItemTypeRole);
        if (itemType == QmlOutlineModel::NonElementBindingType) {
            return false;
        }
    }
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

QVariant QmlJSOutlineFilterModel::data(const QModelIndex &index, int role) const
{
    if (role == QmlOutlineModel::AnnotationRole) {
        // Don't show element id etc behind element if the property is also visible
        if (!filterBindings()
                && index.data(QmlOutlineModel::ItemTypeRole) == QmlOutlineModel::ElementType) {
            return QVariant();
        }
    }
    return QSortFilterProxyModel::data(index, role);
}

bool QmlJSOutlineFilterModel::filterBindings() const
{
    return m_filterBindings;
}

void QmlJSOutlineFilterModel::setFilterBindings(bool filterBindings)
{
    m_filterBindings = filterBindings;
    invalidateFilter();
}

QmlJSOutlineWidget::QmlJSOutlineWidget(QWidget *parent) :
    TextEditor::IOutlineWidget(parent),
    m_treeView(new QmlJSOutlineTreeView(this)),
    m_filterModel(new QmlJSOutlineFilterModel(this)),
    m_editor(0),
    m_enableCursorSync(true),
    m_blockCursorSync(false)
{
    m_filterModel->setFilterBindings(false);

    m_treeView->setModel(m_filterModel);

    QVBoxLayout *layout = new QVBoxLayout;

    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_treeView);

    m_showBindingsAction = new QAction(this);
    m_showBindingsAction->setText(tr("Show All Bindings"));
    m_showBindingsAction->setCheckable(true);
    m_showBindingsAction->setChecked(true);
    connect(m_showBindingsAction, SIGNAL(toggled(bool)), this, SLOT(setShowBindings(bool)));

    setLayout(layout);
}

void QmlJSOutlineWidget::setEditor(QmlJSTextEditorWidget *editor)
{
    m_editor = editor;

    m_filterModel->setSourceModel(m_editor->outlineModel());
    modelUpdated();

    connect(m_treeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(updateSelectionInText(QItemSelection)));

    connect(m_treeView, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(updateTextCursor(QModelIndex)));

    connect(m_editor, SIGNAL(outlineModelIndexChanged(QModelIndex)),
            this, SLOT(updateSelectionInTree(QModelIndex)));
    connect(m_editor->outlineModel(), SIGNAL(updated()),
            this, SLOT(modelUpdated()));
}

QList<QAction*> QmlJSOutlineWidget::filterMenuActions() const
{
    QList<QAction*> list;
    list.append(m_showBindingsAction);
    return list;
}

void QmlJSOutlineWidget::setCursorSynchronization(bool syncWithCursor)
{
    m_enableCursorSync = syncWithCursor;
    if (m_enableCursorSync)
        updateSelectionInTree(m_editor->outlineModelIndex());
}

void QmlJSOutlineWidget::restoreSettings(int position)
{
    QSettings *settings = Core::ICore::instance()->settings();
    bool showBindings = settings->value("QmlJSOutline."+QString::number(position)+".ShowBindings", true).toBool();
    m_showBindingsAction->setChecked(showBindings);
}

void QmlJSOutlineWidget::saveSettings(int position)
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->setValue("QmlJSOutline."+QString::number(position)+".ShowBindings",
                       m_showBindingsAction->isChecked());
}

void QmlJSOutlineWidget::modelUpdated()
{
    m_treeView->expandAll();
}

void QmlJSOutlineWidget::updateSelectionInTree(const QModelIndex &index)
{
    if (!syncCursor())
        return;

    m_blockCursorSync = true;

    QModelIndex baseIndex = index;
    QModelIndex filterIndex = m_filterModel->mapFromSource(baseIndex);
    while (baseIndex.isValid() && !filterIndex.isValid()) { // Search for ancestor index actually shown
        baseIndex = baseIndex.parent();
        filterIndex = m_filterModel->mapFromSource(baseIndex);
    }

    m_treeView->selectionModel()->select(filterIndex, QItemSelectionModel::ClearAndSelect);
    m_treeView->scrollTo(filterIndex);
    m_blockCursorSync = false;
}

void QmlJSOutlineWidget::updateSelectionInText(const QItemSelection &selection)
{
    if (!syncCursor())
        return;

    if (!selection.indexes().isEmpty()) {
        QModelIndex index = selection.indexes().first();

        updateTextCursor(index);
    }
}

void QmlJSOutlineWidget::updateTextCursor(const QModelIndex &index)
{
    QModelIndex sourceIndex = m_filterModel->mapToSource(index);
    AST::SourceLocation location = m_editor->outlineModel()->sourceLocation(sourceIndex);

    if (!location.isValid())
        return;

    const QTextBlock lastBlock = m_editor->document()->lastBlock();
    const uint textLength = lastBlock.position() + lastBlock.length();
    if (location.offset >= textLength)
        return;

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    editorManager->cutForwardNavigationHistory();
    editorManager->addCurrentPositionToNavigationHistory();

    QTextCursor textCursor = m_editor->textCursor();
    m_blockCursorSync = true;
    textCursor.setPosition(location.offset);
    m_editor->setTextCursor(textCursor);
    m_editor->centerCursor();
    m_blockCursorSync = false;
}

void QmlJSOutlineWidget::setShowBindings(bool showBindings)
{
    m_filterModel->setFilterBindings(!showBindings);
    modelUpdated();
    updateSelectionInTree(m_editor->outlineModelIndex());
}

bool QmlJSOutlineWidget::syncCursor()
{
    return m_enableCursorSync && !m_blockCursorSync;
}

bool QmlJSOutlineWidgetFactory::supportsEditor(Core::IEditor *editor) const
{
    if (qobject_cast<QmlJSEditorEditable*>(editor))
        return true;
    return false;
}

TextEditor::IOutlineWidget *QmlJSOutlineWidgetFactory::createWidget(Core::IEditor *editor)
{
    QmlJSOutlineWidget *widget = new QmlJSOutlineWidget;

    QmlJSEditorEditable *qmlJSEditable = qobject_cast<QmlJSEditorEditable*>(editor);
    QmlJSTextEditorWidget *qmlJSEditor = qobject_cast<QmlJSTextEditorWidget*>(qmlJSEditable->widget());
    Q_ASSERT(qmlJSEditor);

    widget->setEditor(qmlJSEditor);

    return widget;
}

} // namespace Internal
} // namespace QmlJSEditor
