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

#ifndef IPROJECTPROPERTIES_H
#define IPROJECTPROPERTIES_H

#include "projectexplorer_export.h"

#include <QtCore/QObject>
#include <QtGui/QIcon>
#include <QtGui/QWidget>

namespace ProjectExplorer {
class Project;
class Target;

namespace Constants {
    const int PANEL_LEFT_MARGIN = 70;
}

class PROJECTEXPLORER_EXPORT PropertiesPanel
{
    Q_DISABLE_COPY(PropertiesPanel)

public:
    PropertiesPanel() {}
    ~PropertiesPanel() { delete m_widget; }

    QString displayName() const { return m_displayName; }
    QIcon icon() const { return m_icon; }
    QWidget *widget() const { return m_widget; }

    void setDisplayName(const QString &name) { m_displayName = name; }
    void setIcon(const QIcon &icon) { m_icon = icon; }
    void setWidget(QWidget *widget) { m_widget = widget; }

private:
    QString m_displayName;
    QWidget *m_widget;
    QIcon m_icon;
};

class PROJECTEXPLORER_EXPORT IPanelFactory : public QObject
{
    Q_OBJECT
public:
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
};

class PROJECTEXPLORER_EXPORT IProjectPanelFactory : public IPanelFactory
{
    Q_OBJECT
public:
    virtual bool supports(Project *project) = 0;
    virtual PropertiesPanel *createPanel(Project *project) = 0;
};

class PROJECTEXPLORER_EXPORT ITargetPanelFactory : public IPanelFactory
{
    Q_OBJECT
public:
    virtual bool supports(Target *target) = 0;
    virtual PropertiesPanel *createPanel(Target *target) = 0;
};

} // namespace ProjectExplorer

#endif // IPROJECTPROPERTIES_H
