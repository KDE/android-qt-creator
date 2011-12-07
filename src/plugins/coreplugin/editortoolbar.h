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

#ifndef FAKETOOLBAR_H
#define FAKETOOLBAR_H

#include "core_global.h"

#include <utils/styledbar.h>

namespace Core {
class IEditor;

struct EditorToolBarPrivate;

/**
  * Fakes an IEditor-like toolbar for design mode widgets such as Qt Designer and Bauhaus.
  * Creates a combobox for open files and lock and close buttons on the right.
  */
class CORE_EXPORT EditorToolBar : public Utils::StyledBar
{
    Q_OBJECT

public:
    explicit EditorToolBar(QWidget *parent = 0);
    virtual ~EditorToolBar();

    enum ToolbarCreationFlags { FlagsNone = 0, FlagsStandalone = 1 };

    /**
      * Adds an editor whose state is listened to, so that the toolbar can be kept up to date
      * with regards to locked status and tooltips.
      */
    void addEditor(IEditor *editor);

    /**
      * Sets the editor and adds its custom toolbar to the widget.
      */
    void setCurrentEditor(IEditor *editor);

    void setToolbarCreationFlags(ToolbarCreationFlags flags);

    /**
      * Adds a toolbar to the widget and sets invisible by default.
      */
    void addCenterToolBar(QWidget *toolBar);

    void setNavigationVisible(bool isVisible);
    void setCanGoBack(bool canGoBack);
    void setCanGoForward(bool canGoForward);
    void removeToolbarForEditor(IEditor *editor);
    void setCloseSplitEnabled(bool enable);

public slots:
    void updateEditorStatus(IEditor *editor);

signals:
    void closeClicked();
    void goBackClicked();
    void goForwardClicked();
    void horizontalSplitClicked();
    void verticalSplitClicked();
    void closeSplitClicked();
    void listSelectionActivated(int row);

private slots:
    void updateEditorListSelection(Core::IEditor *newSelection);
    void changeActiveEditor(int row);
    void listContextMenu(QPoint);
    void makeEditorWritable();

    void checkEditorStatus();
    void closeEditor();
    void updateActionShortcuts();

private:
    void updateToolBar(QWidget *toolBar);
    IEditor *currentEditor() const;

    EditorToolBarPrivate *d;
};

} // namespace Core

#endif // FAKETOOLBAR_H
