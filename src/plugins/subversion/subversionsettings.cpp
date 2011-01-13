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

#include "subversionsettings.h"

#include <QtCore/QSettings>

static const char groupC[] = "Subversion";
static const char commandKeyC[] = "Command";
static const char userKeyC[] = "User";
static const char passwordKeyC[] = "Password";
static const char authenticationKeyC[] = "Authentication";

static const char promptToSubmitKeyC[] = "PromptForSubmit";
static const char timeOutKeyC[] = "TimeOut";
static const char spaceIgnorantAnnotationKeyC[] = "SpaceIgnorantAnnotation";
static const char logCountKeyC[] = "LogCount";

enum { defaultTimeOutS = 30, defaultLogCount = 1000 };

static QString defaultCommand()
{
    QString rc;
    rc = QLatin1String("svn");
#if defined(Q_OS_WIN32)
    rc.append(QLatin1String(".exe"));
#endif
    return rc;
}

using namespace Subversion::Internal;

SubversionSettings::SubversionSettings() :
    svnCommand(defaultCommand()),
    useAuthentication(false),
    logCount(defaultLogCount),
    timeOutS(defaultTimeOutS),
    promptToSubmit(true),
    spaceIgnorantAnnotation(true)
{
}

void SubversionSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(groupC));
    svnCommand = settings->value(QLatin1String(commandKeyC), defaultCommand()).toString();
    useAuthentication = settings->value(QLatin1String(authenticationKeyC), QVariant(false)).toBool();
    user = settings->value(QLatin1String(userKeyC), QString()).toString();
    password =  settings->value(QLatin1String(passwordKeyC), QString()).toString();
    timeOutS = settings->value(QLatin1String(timeOutKeyC), defaultTimeOutS).toInt();
    promptToSubmit = settings->value(QLatin1String(promptToSubmitKeyC), true).toBool();
    spaceIgnorantAnnotation = settings->value(QLatin1String(spaceIgnorantAnnotationKeyC), true).toBool();
    logCount = settings->value(QLatin1String(logCountKeyC), int(defaultLogCount)).toInt();
    settings->endGroup();
}

void SubversionSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(QLatin1String(commandKeyC), svnCommand);
    settings->setValue(QLatin1String(authenticationKeyC), QVariant(useAuthentication));
    settings->setValue(QLatin1String(userKeyC), user);
    settings->setValue(QLatin1String(passwordKeyC), password);
    settings->setValue(QLatin1String(promptToSubmitKeyC), promptToSubmit);
    settings->setValue(QLatin1String(timeOutKeyC), timeOutS);
    settings->setValue(QLatin1String(spaceIgnorantAnnotationKeyC), spaceIgnorantAnnotation);
    settings->setValue(QLatin1String(logCountKeyC), logCount);
    settings->endGroup();
}

bool SubversionSettings::equals(const SubversionSettings &s) const
{
    return svnCommand        == s.svnCommand
        && useAuthentication == s.useAuthentication
        && user              == s.user
        && password          == s.password
        && logCount          == s.logCount
        && timeOutS          == s.timeOutS
        && promptToSubmit    == s.promptToSubmit
        && spaceIgnorantAnnotation == s.spaceIgnorantAnnotation;
}
