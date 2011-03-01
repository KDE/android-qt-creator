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

#ifndef CPPTYPEHIERARCHY_H
#define CPPTYPEHIERARCHY_H

#include "cppelementevaluator.h"

#include <coreplugin/inavigationwidgetfactory.h>

#include <QtCore/QString>
#include <QtGui/QWidget>
#include <QtGui/QStackedWidget>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
class QStandardItem;
class QModelIndex;
QT_END_NAMESPACE

namespace Core {
class IEditor;
}

namespace Utils {
class NavigationTreeView;
class AnnotatedItemDelegate;
}

namespace CppEditor {
namespace Internal {

class CPPEditorWidget;

class CppTypeHierarchyWidget : public QWidget
{
    Q_OBJECT
public:
    CppTypeHierarchyWidget(Core::IEditor *editor);
    virtual ~CppTypeHierarchyWidget();

    bool handleEditorChange(Core::IEditor *editor);

public slots:
    void perform();

private slots:
    void onItemClicked(const QModelIndex &index);

private:
    enum ItemRole {
        AnnotationRole = Qt::UserRole + 1,
        LinkRole
    };

    void buildModel(const CppClass &cppClass, QStandardItem *item);

    CPPEditorWidget *m_cppEditor;
    Utils::NavigationTreeView *m_treeView;
    QStandardItemModel *m_model;
    Utils::AnnotatedItemDelegate *m_delegate;
};

// @todo: Pretty much the same design as the OutlineWidgetStack. Maybe we can generalize the
// outline factory so that it works for different widgets that support the same editor.
class CppTypeHierarchyStackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
    CppTypeHierarchyStackedWidget(QWidget *parent = 0);
    virtual ~CppTypeHierarchyStackedWidget();

public slots:
    void editorChanged(Core::IEditor* editor);

private:
    CppTypeHierarchyWidget *m_typeHiearchyWidgetInstance;
};

class CppTypeHierarchyFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
public:
    CppTypeHierarchyFactory();
    virtual ~CppTypeHierarchyFactory();

    virtual QString displayName() const;
    virtual int priority() const;
    virtual QString id() const;
    virtual QKeySequence activationSequence() const;
    virtual Core::NavigationView createWidget();
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPTYPEHIERARCHY_H
