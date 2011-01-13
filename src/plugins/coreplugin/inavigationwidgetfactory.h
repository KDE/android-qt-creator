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

#ifndef INAVIGATIONWIDGET_H
#define INAVIGATIONWIDGET_H

#include <coreplugin/core_global.h>
#include <QtCore/QObject>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QToolButton;
class QKeySequence;
class QWidget;
QT_END_NAMESPACE

namespace Core {

struct NavigationView
{
    QWidget *widget;
    QList<QToolButton *> dockToolBarWidgets;
};

class CORE_EXPORT INavigationWidgetFactory : public QObject
{
    Q_OBJECT
public:
    INavigationWidgetFactory();
    virtual ~INavigationWidgetFactory();

    virtual QString displayName() const = 0;
    virtual int priority() const = 0;
    virtual QString id() const = 0;
    virtual QKeySequence activationSequence() const;
    // This design is not optimal, think about it again once we need to extend it
    // It could be implemented as returning an object which has both the widget
    // and the docktoolbar widgets
    // Similar to how IView
    virtual NavigationView createWidget() = 0;

    // Read and store settings for the widget, created by this factory
    // and being at position position. (The position is important since
    // a certain type of widget could exist multiple times.)
    virtual void saveSettings(int position, QWidget *widget);
    virtual void restoreSettings(int position, QWidget *widget);
};

} // namespace Core

#endif // INAVIGATIONWIDGET_H
