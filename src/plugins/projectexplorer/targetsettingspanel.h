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

#ifndef TARGETSETTINGSPANEL_H
#define TARGETSETTINGSPANEL_H

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QStackedWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Target;
class Project;

namespace Internal {

class TargetSettingsWidget;
class PanelsWidget;

class TargetSettingsPanelWidget : public QWidget
{
    Q_OBJECT
public:
    TargetSettingsPanelWidget(Project *project);
    ~TargetSettingsPanelWidget();

    void setupUi();

    int currentSubIndex() const;
    void setCurrentSubIndex(int subIndex);

private slots:
    void currentTargetChanged(int targetIndex, int subIndex);
    void removeTarget();
    void addTarget(QAction *);
    void targetAdded(ProjectExplorer::Target *target);
    void removedTarget(ProjectExplorer::Target *target);
    void activeTargetChanged(ProjectExplorer::Target *target);
    void updateTargetAddAndRemoveButtons();

private:
    Target *m_currentTarget;
    Project *m_project;
    TargetSettingsWidget *m_selector;
    QStackedWidget *m_centralWidget;
    QWidget *m_noTargetLabel;
    PanelsWidget *m_panelWidgets[2];
    QList<Target *> m_targets;
    QMenu *m_addMenu;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TARGETSETTINGSPANEL_H
