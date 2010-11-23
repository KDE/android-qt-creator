/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "androidglobal.h"

#include <QtCore/QString>

namespace Qt4ProjectManager {
namespace Internal {

QString AndroidGlobal::homeDirOnDevice(const QString &uname)
{
    return uname == QLatin1String("root")
        ? QString::fromLatin1("/root")
        : QLatin1String("/home/") + uname;
}

QString AndroidGlobal::remoteSudo()
{
    return QLatin1String("/usr/lib/mad-developer/devrootsh");
}

QString AndroidGlobal::remoteCommandPrefix(const QString &commandFilePath)
{
    return QString::fromLocal8Bit("%1 chmod a+x %2; %3; ")
        .arg(remoteSudo(), commandFilePath, remoteSourceProfilesCommand());
}

QString AndroidGlobal::remoteSourceProfilesCommand()
{
    const QList<QByteArray> profiles = QList<QByteArray>() << "/etc/profile"
        << "/home/user/.profile" << "~/.profile";
    QByteArray remoteCall(":");
    foreach (const QByteArray &profile, profiles)
        remoteCall += "; test -f " + profile + " && source " + profile;
    return QString::fromAscii(remoteCall);
}

QString AndroidGlobal::remoteEnvironment(const QList<Utils::EnvironmentItem> &list)
{
    QString env;
    QString placeHolder = QLatin1String("%1=%2 ");
    foreach (const Utils::EnvironmentItem &item, list)
        env.append(placeHolder.arg(item.name).arg(item.value));
    return env.mid(0, env.size() - 1);
}

} // namespace Internal
} // namespace Qt4ProjectManager
