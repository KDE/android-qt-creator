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

#ifndef ICORELISTENER_H
#define ICORELISTENER_H

#include "core_global.h"
#include <QtCore/QObject>

namespace Core {
class IEditor;
/*!
  \class Core::ICoreListener

  \brief Provides a hook for plugins to veto on certain events emitted from
the core plugin.

  You implement this interface if you want to prevent certain events from
  occurring, e.g.  if you want to prevent the closing of the whole application
  or to prevent the closing of an editor window under certain conditions.

  If e.g. the application window requests a close, then first
  ICoreListener::coreAboutToClose() is called (in arbitrary order) on all
  registered objects implementing this interface. If one if these calls returns
  false, the process is aborted and the event is ignored.  If all calls return
  true, the corresponding signal is emitted and the event is accepted/performed.

  Guidelines for implementing:
  \list
  \o Return false from the implemented method if you want to prevent the event.
  \o You need to add your implementing object to the plugin managers objects:
     ExtensionSystem::PluginManager::instance()->addObject(yourImplementingObject);
  \o Don't forget to remove the object again at deconstruction (e.g. in the destructor of
     your plugin).
*/
class CORE_EXPORT ICoreListener : public QObject
{
    Q_OBJECT
public:
    ICoreListener(QObject *parent = 0) : QObject(parent) {}
    virtual ~ICoreListener() {}

    virtual bool editorAboutToClose(IEditor * /*editor*/) { return true; }
    virtual bool coreAboutToClose() { return true; }
};

} // namespace Core

#endif // ICORELISTENER_H
