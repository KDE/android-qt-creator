/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef RENDERNODEINSTANCESERVER_H
#define RENDERNODEINSTANCESERVER_H

#include "qt4nodeinstanceserver.h"

namespace QmlDesigner {

class Qt4RenderNodeInstanceServer : public Qt4NodeInstanceServer
{
    Q_OBJECT
public:
    explicit Qt4RenderNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);

    void createScene(const CreateSceneCommand &command);
    void clearScene(const ClearSceneCommand &command);
    void completeComponent(const CompleteComponentCommand &command);
    void changeState(const ChangeStateCommand &command);

protected:
    void collectItemChangesAndSendChangeCommands();

private:
    QSet<ServerNodeInstance> m_dirtyInstanceSet;
};

} // namespace QmlDesigner

#endif // RENDERNODEINSTANCESERVER_H
