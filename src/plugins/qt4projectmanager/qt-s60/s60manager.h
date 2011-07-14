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

#ifndef S60MANAGER_H
#define S60MANAGER_H

#include <QtCore/QObject>

namespace ProjectExplorer {
class ToolChain;
}

namespace Qt4ProjectManager {
namespace Internal {

class S60Manager : public QObject
{
    Q_OBJECT
public:
    S60Manager(QObject *parent = 0);
    ~S60Manager();
    static S60Manager *instance();

    static QString platform(const ProjectExplorer::ToolChain *tc);

private:
    void addAutoReleasedObject(QObject *p);

    static S60Manager *m_instance;
    QObjectList m_pluginObjects;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60MANAGER_H
