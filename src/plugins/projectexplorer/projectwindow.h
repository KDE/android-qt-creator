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

#ifndef PROJECTWINDOW_H
#define PROJECTWINDOW_H

#include <QtCore/QMap>
#include <QtGui/QScrollArea>

QT_BEGIN_NAMESPACE
class QLabel;
class QGridLayout;
class QMenu;
class QStackedWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {

class IPropertiesPanel;
class Project;
class Target;
class BuildConfiguration;
class RunConfiguration;

namespace Internal {

class DoubleTabWidget;

class PanelsWidget : public QScrollArea
{
    Q_OBJECT
public:
    PanelsWidget(QWidget *parent);
    ~PanelsWidget();
    // Adds a widget
    void addPropertiesPanel(IPropertiesPanel *panel);

private:
    void addPanelWidget(IPropertiesPanel *panel, int row);

    QList<IPropertiesPanel *> m_panels;
    QGridLayout *m_layout;
    QWidget *m_root;
};

class ProjectWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectWindow(QWidget *parent = 0);
    ~ProjectWindow();

    void aboutToShutdown();
    void extensionsInitialized();
private slots:
    void targetFactoriesChanged();
    void showProperties(int index, int subIndex);
    void restoreStatus();
    void saveStatus();
    void registerProject(ProjectExplorer::Project*);
    void deregisterProject(ProjectExplorer::Project*);
    void startupProjectChanged(ProjectExplorer::Project *);

private:
    bool useTargetPage(ProjectExplorer::Project *project);
    void removeCurrentWidget();

    DoubleTabWidget *m_tabWidget;
    QStackedWidget *m_centralWidget;
    QWidget *m_currentWidget;
    QList<ProjectExplorer::Project *> m_tabIndexToProject;
    QMap<ProjectExplorer::Project *, bool> m_usesTargetPage;
    int m_previousTargetSubIndex;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTWINDOW_H
