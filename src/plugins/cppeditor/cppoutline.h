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

#ifndef CPPOUTLINE_H
#define CPPOUTLINE_H

#include "cppeditor.h"

#include <utils/navigationtreeview.h>
#include <texteditor/ioutlinewidget.h>

#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QTreeView>

namespace CppEditor {
namespace Internal {

class CppOutlineTreeView : public Utils::NavigationTreeView
{
    Q_OBJECT
public:
    CppOutlineTreeView(QWidget *parent);

    void contextMenuEvent(QContextMenuEvent *event);
};

class CppOutlineFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    CppOutlineFilterModel(CPlusPlus::OverviewModel *sourceModel, QObject *parent);
    // QSortFilterProxyModel
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const;
private:
    CPlusPlus::OverviewModel *m_sourceModel;
};

class CppOutlineWidget : public TextEditor::IOutlineWidget
{
    Q_OBJECT
public:
    CppOutlineWidget(CPPEditor *editor);

    // IOutlineWidget
    virtual QList<QAction*> filterMenuActions() const;
    virtual void setCursorSynchronization(bool syncWithCursor);

private slots:
    void modelUpdated();
    void updateSelectionInTree(const QModelIndex &index);
    void updateSelectionInText(const QItemSelection &selection);
    void updateTextCursor(const QModelIndex &index);

private:
    bool syncCursor();

private:
    CPPEditor *m_editor;
    CppOutlineTreeView *m_treeView;
    CPlusPlus::OverviewModel *m_model;
    CppOutlineFilterModel *m_proxyModel;

    bool m_enableCursorSync;
    bool m_blockCursorSync;
};

class CppOutlineWidgetFactory : public TextEditor::IOutlineWidgetFactory
{
    Q_OBJECT
public:
    bool supportsEditor(Core::IEditor *editor) const;
    TextEditor::IOutlineWidget *createWidget(Core::IEditor *editor);
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPOUTLINE_H
