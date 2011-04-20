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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H


#include <QtCore/QObject>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QString;
class QAbstractItemModel;
class QDialog;
QT_END_NAMESPACE

namespace QmlDesigner {

class IPlugin;

class PluginManagerPrivate;

// PluginManager: Loads the plugin libraries on demand "as lazy as
// possible", that is, directories are scanned and
// instances are created only when  instances() is called.

class PluginManager
{
    Q_DISABLE_COPY(PluginManager)
public:
    typedef QList<IPlugin *> IPluginList;

    PluginManager();
    ~PluginManager();

    void setPluginPaths(const QStringList &paths);

    IPluginList instances();

    QDialog *createAboutPluginDialog(QWidget *parent);

private:
    // Convenience to create a model for an "About Plugins"
    // dialog. Forces plugin initialization.
    QAbstractItemModel *createModel(QObject *parent = 0);

    PluginManagerPrivate *m_d;
};

} // namespace QmlDesigner
#endif // PLUGINMANAGER_H
