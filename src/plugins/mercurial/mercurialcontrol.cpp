/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#include "mercurialcontrol.h"
#include "mercurialclient.h"

#include <vcsbase/vcsbaseclientsettings.h>
#include <vcsbase/vcsbaseconstants.h>

#include <QtCore/QFileInfo>
#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QDir>

using namespace Mercurial::Internal;

MercurialControl::MercurialControl(MercurialClient *client)
        :   mercurialClient(client)
{
}

QString MercurialControl::displayName() const
{
    return tr("Mercurial");
}

QString MercurialControl::id() const
{
    return QLatin1String(VCSBase::Constants::VCS_ID_MERCURIAL);
}

bool MercurialControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    QFileInfo dir(directory);
    const QString topLevelFound = mercurialClient->findTopLevelForFile(dir);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool MercurialControl::isConfigured() const
{
    const QString binary = mercurialClient->settings()->stringValue(MercurialSettings::binaryPathKey);
    if (binary.isEmpty())
        return false;
    QFileInfo fi(binary);
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool MercurialControl::supportsOperation(Operation operation) const
{
    bool supported = isConfigured();
    switch (operation) {
    case Core::IVersionControl::AddOperation:
    case Core::IVersionControl::DeleteOperation:
    case Core::IVersionControl::MoveOperation:
    case Core::IVersionControl::CreateRepositoryOperation:
    case Core::IVersionControl::AnnotateOperation:
    case Core::IVersionControl::CheckoutOperation:
    case Core::IVersionControl::GetRepositoryRootOperation:
        break;
    case Core::IVersionControl::OpenOperation:
    case Core::IVersionControl::SnapshotOperations:
        supported = false;
        break;
    }
    return supported;
}

bool MercurialControl::vcsOpen(const QString &filename)
{
    Q_UNUSED(filename)
    return true;
}

bool MercurialControl::vcsAdd(const QString &filename)
{
    const QFileInfo fi(filename);
    return mercurialClient->synchronousAdd(fi.absolutePath(), fi.fileName());
}

bool MercurialControl::vcsDelete(const QString &filename)
{
    const QFileInfo fi(filename);
    return mercurialClient->synchronousRemove(fi.absolutePath(), fi.fileName());
}

bool MercurialControl::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo fromInfo(from);
    const QFileInfo toInfo(to);
    return mercurialClient->synchronousMove(fromInfo.absolutePath(),
                                            fromInfo.absoluteFilePath(),
                                            toInfo.absoluteFilePath());
}

bool MercurialControl::vcsCreateRepository(const QString &directory)
{
    return mercurialClient->synchronousCreateRepository(directory);
}

QString MercurialControl::vcsCreateSnapshot(const QString &)
{
    return QString();
}

QStringList MercurialControl::vcsSnapshots(const QString &)
{
    return QStringList();
}

bool MercurialControl::vcsRestoreSnapshot(const QString &, const QString &)
{
    return false;
}

bool MercurialControl::vcsRemoveSnapshot(const QString &, const QString &)
{
    return false;
}

bool MercurialControl::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    mercurialClient->annotate(fi.absolutePath(), fi.fileName(), QString(), line);
    return true;
}

bool MercurialControl::sccManaged(const QString &filename)
{
    const QFileInfo fi(filename);
    QString topLevel;
    const bool managed = managesDirectory(fi.absolutePath(), &topLevel);
    if (!managed || topLevel.isEmpty())
        return false;
    const QDir topLevelDir(topLevel);
    return mercurialClient->manifestSync(topLevel, topLevelDir.relativeFilePath(filename));
}

bool MercurialControl::vcsCheckout(const QString &directory, const QByteArray &url)
{
    return mercurialClient->synchronousClone(QString(), directory, url);
}

QString MercurialControl::vcsGetRepositoryURL(const QString &directory)
{
    return mercurialClient->vcsGetRepositoryURL(directory);
}

void MercurialControl::changed(const QVariant &v)
{
    switch (v.type()) {
    case QVariant::String:
        emit repositoryChanged(v.toString());
        break;
    case QVariant::StringList:
        emit filesChanged(v.toStringList());
        break;
    default:
        break;
    }
}

void MercurialControl::emitConfigurationChanged()
{
    emit configurationChanged();
}
