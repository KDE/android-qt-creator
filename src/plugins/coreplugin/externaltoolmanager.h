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

#ifndef EXTERNALTOOLMANAGER_H
#define EXTERNALTOOLMANAGER_H

#include "core_global.h"

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QList>
#include <QtCore/QString>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace Core {
class ICore;
class ActionContainer;

namespace Internal {
class ExternalToolRunner;
class ExternalTool;
}

class CORE_EXPORT ExternalToolManager : public QObject
{
    Q_OBJECT

public:
    static ExternalToolManager *instance() { return m_instance; }

    ExternalToolManager(Core::ICore *core);
    ~ExternalToolManager();

    QMap<QString, QList<Internal::ExternalTool *> > toolsByCategory() const;
    QMap<QString, Internal::ExternalTool *> toolsById() const;

    void setToolsByCategory(const QMap<QString, QList<Internal::ExternalTool *> > &tools);

signals:
    void replaceSelectionRequested(const QString &text);

private slots:
    void menuActivated();
    void openPreferences();

private:
    void initialize();
    void parseDirectory(const QString &directory,
                        QMap<QString, QMultiMap<int, Internal::ExternalTool*> > *categoryMenus,
                        QMap<QString, Internal::ExternalTool *> *tools,
                        bool isPreset = false);
    void readSettings(const QMap<QString, Internal::ExternalTool *> &tools,
                      QMap<QString, QList<Internal::ExternalTool*> > *categoryPriorityMap);
    void writeSettings();

    static ExternalToolManager *m_instance;
    Core::ICore *m_core;
    QMap<QString, Internal::ExternalTool *> m_tools;
    QMap<QString, QList<Internal::ExternalTool *> > m_categoryMap;
    QMap<QString, QAction *> m_actions;
    QMap<QString, ActionContainer *> m_containers;
    QAction *m_configureSeparator;
    QAction *m_configureAction;

    // for sending the replaceSelectionRequested signal
    friend class Core::Internal::ExternalToolRunner;
};

} // Core


#endif // EXTERNALTOOLMANAGER_H
