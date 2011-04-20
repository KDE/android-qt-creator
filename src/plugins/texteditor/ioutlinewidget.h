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

#ifndef IOUTLINEWIDGET_H
#define IOUTLINEWIDGET_H

#include <texteditor/texteditor_global.h>
#include <QtGui/QWidget>

namespace Core {
class IEditor;
}

namespace TextEditor {

class TEXTEDITOR_EXPORT IOutlineWidget : public QWidget
{
    Q_OBJECT
public:
    IOutlineWidget(QWidget *parent = 0) : QWidget(parent) {}

    virtual QList<QAction*> filterMenuActions() const = 0;
    virtual void setCursorSynchronization(bool syncWithCursor) = 0;

    virtual void restoreSettings(int position) { Q_UNUSED(position); }
    virtual void saveSettings(int position) { Q_UNUSED(position); }
};

class TEXTEDITOR_EXPORT IOutlineWidgetFactory : public QObject {
    Q_OBJECT
public:
    virtual bool supportsEditor(Core::IEditor *editor) const = 0;
    virtual IOutlineWidget *createWidget(Core::IEditor *editor) = 0;
};

} // namespace TextEditor

#endif // IOUTLINEWIDGET_H
