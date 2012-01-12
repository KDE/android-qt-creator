/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "subversioncontrol.h"
#include "subversionplugin.h"
#include "subversionsettings.h"

#include <vcsbase/vcsbaseconstants.h>

#include <QtCore/QFileInfo>

using namespace Subversion;
using namespace Subversion::Internal;

SubversionControl::SubversionControl(SubversionPlugin *plugin) :
    m_plugin(plugin)
{
}

QString SubversionControl::displayName() const
{
    return QLatin1String("subversion");
}

Core::Id SubversionControl::id() const
{
    return VcsBase::Constants::VCS_ID_SUBVERSION;
}

bool SubversionControl::isConfigured() const
{
    const QString binary = m_plugin->settings().svnCommand;
    if (binary.isEmpty())
        return false;
    QFileInfo fi(binary);
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool SubversionControl::supportsOperation(Operation operation) const
{
    bool rc = isConfigured();
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case MoveOperation:
    case AnnotateOperation:
    case CheckoutOperation:
    case GetRepositoryRootOperation:
        break;
    case OpenOperation:
    case CreateRepositoryOperation:
    case SnapshotOperations:
        rc = false;
        break;
    }
    return rc;
}

bool SubversionControl::vcsOpen(const QString & /* fileName */)
{
    // Open for edit: N/A
    return true;
}

bool SubversionControl::vcsAdd(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsAdd(fi.absolutePath(), fi.fileName());
}

bool SubversionControl::vcsDelete(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsDelete(fi.absolutePath(), fi.fileName());
}

bool SubversionControl::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo fromInfo(from);
    const QFileInfo toInfo(to);
    return m_plugin->vcsMove(fromInfo.absolutePath(), fromInfo.absoluteFilePath(), toInfo.absoluteFilePath());
}

bool SubversionControl::vcsCheckout(const QString &directory, const QByteArray &url)
{
    return m_plugin->vcsCheckout(directory, url);
}

QString SubversionControl::vcsGetRepositoryURL(const QString &directory)
{
    return m_plugin->vcsGetRepositoryURL(directory);
}

bool SubversionControl::vcsCreateRepository(const QString &)
{
    return false;
}

QString SubversionControl::vcsCreateSnapshot(const QString &)
{
    return QString();
}

QStringList SubversionControl::vcsSnapshots(const QString &)
{
    return QStringList();
}

bool SubversionControl::vcsRestoreSnapshot(const QString &, const QString &)
{
    return false;
}

bool SubversionControl::vcsRemoveSnapshot(const QString &, const QString &)
{
    return false;
}

bool SubversionControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    return m_plugin->managesDirectory(directory, topLevel);
}

bool SubversionControl::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    m_plugin->vcsAnnotate(fi.absolutePath(), fi.fileName(), QString(), line);
    return true;
}

void SubversionControl::emitRepositoryChanged(const QString &s)
{
    emit repositoryChanged(s);
}

void SubversionControl::emitFilesChanged(const QStringList &l)
{
    emit filesChanged(l);
}

void SubversionControl::emitConfigurationChanged()
{
    emit configurationChanged();
}
