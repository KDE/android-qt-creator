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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H

#include <QtCore/QMap>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QSettings;
class QToolBar;
class QAction;
class QToolButton;
QT_END_NAMESPACE

namespace Core {
class SideBar;
class SideBarItem;
class Command;

namespace Internal {
class SideBarComboBox;

class SideBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SideBarWidget(SideBar *sideBar, const QString &title);
    virtual ~SideBarWidget();

    QString currentItemId() const;
    QString currentItemTitle() const;
    void setCurrentItem(const QString &id);

    void updateAvailableItems();
    void removeCurrentItem();

    Core::Command *command(const QString &id) const;

signals:
    void splitMe();
    void closeMe();
    void currentWidgetChanged();

private slots:
    void setCurrentIndex(int);

private:
    SideBarComboBox *m_comboBox;
    SideBarItem *m_currentItem;
    QToolBar *m_toolbar;
    QAction *m_splitAction;
    QList<QAction *> m_addedToolBarActions;
    SideBar *m_sideBar;
};

} // namespace Internal
} // namespace Core

#endif // SIDEBARWIDGET_H
