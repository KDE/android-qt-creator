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

#include "gitsettings.h"
#include "gitconstants.h"

#include <utils/synchronousprocess.h>

#include <QtCore/QSettings>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>

static const char groupC[] = "Git";
static const char sysEnvKeyC[] = "SysEnv";
static const char pathKeyC[] = "Path";
static const char logCountKeyC[] = "LogCount";
static const char timeoutKeyC[] = "TimeOut";
static const char pullRebaseKeyC[] = "PullRebase";
static const char promptToSubmitKeyC[] = "PromptForSubmit";
static const char omitAnnotationDateKeyC[] = "OmitAnnotationDate";
static const char ignoreSpaceChangesBlameKeyC[] = "SpaceIgnorantBlame";
static const char ignoreSpaceChangesDiffKeyC[] = "SpaceIgnorantDiff";
static const char diffPatienceKeyC[] = "DiffPatience";
static const char winSetHomeEnvironmentKeyC[] = "WinSetHomeEnvironment";
static const char gitkOptionsKeyC[] = "GitKOptions";
static const char showPrettyFormatC[] = "DiffPrettyFormat";

enum {
    defaultPullRebase = 0,
    defaultLogCount = 100,
#ifdef Q_OS_WIN
    defaultTimeOut = 60
#else	
    defaultTimeOut = 30
#endif	
};

namespace Git {
namespace Internal {

GitSettings::GitSettings() :
    adoptPath(false),
    logCount(defaultLogCount),
    timeoutSeconds(defaultTimeOut),
    pullRebase(bool(defaultPullRebase)),
    promptToSubmit(true),
    omitAnnotationDate(false),
    ignoreSpaceChangesInDiff(false),
    ignoreSpaceChangesInBlame(true),
    diffPatience(true),
    winSetHomeEnvironment(false),
    showPrettyFormat(5)
{
}

void GitSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(groupC));
    adoptPath = settings->value(QLatin1String(sysEnvKeyC), false).toBool();
    path = settings->value(QLatin1String(pathKeyC), QString()).toString();
    logCount = settings->value(QLatin1String(logCountKeyC), defaultLogCount).toInt();
    timeoutSeconds = settings->value(QLatin1String(timeoutKeyC), defaultTimeOut).toInt();
    pullRebase = settings->value(QLatin1String(pullRebaseKeyC), bool(defaultPullRebase)).toBool();
    promptToSubmit = settings->value(QLatin1String(promptToSubmitKeyC), true).toBool();
    omitAnnotationDate = settings->value(QLatin1String(omitAnnotationDateKeyC), false).toBool();
    ignoreSpaceChangesInDiff = settings->value(QLatin1String(ignoreSpaceChangesDiffKeyC), true).toBool();
    ignoreSpaceChangesInBlame = settings->value(QLatin1String(ignoreSpaceChangesBlameKeyC), true).toBool();
    diffPatience = settings->value(QLatin1String(diffPatienceKeyC), true).toBool();
    winSetHomeEnvironment = settings->value(QLatin1String(winSetHomeEnvironmentKeyC), false).toBool();
    gitkOptions = settings->value(QLatin1String(gitkOptionsKeyC)).toString();
    showPrettyFormat = settings->value(QLatin1String(showPrettyFormatC), 5).toInt();
    settings->endGroup();
}

void GitSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(QLatin1String(sysEnvKeyC), adoptPath);
    settings->setValue(QLatin1String(pathKeyC), path);
    settings->setValue(QLatin1String(logCountKeyC), logCount);
    settings->setValue(QLatin1String(timeoutKeyC), timeoutSeconds);
    settings->setValue(QLatin1String(pullRebaseKeyC), pullRebase);
    settings->setValue(QLatin1String(promptToSubmitKeyC), promptToSubmit);
    settings->setValue(QLatin1String(omitAnnotationDateKeyC), omitAnnotationDate);
    settings->setValue(QLatin1String(ignoreSpaceChangesDiffKeyC), ignoreSpaceChangesInDiff);
    settings->setValue(QLatin1String(ignoreSpaceChangesBlameKeyC), ignoreSpaceChangesInBlame);
    settings->setValue(QLatin1String(diffPatienceKeyC), diffPatience);
    settings->setValue(QLatin1String(winSetHomeEnvironmentKeyC), winSetHomeEnvironment);
    settings->setValue(QLatin1String(gitkOptionsKeyC), gitkOptions);
    settings->setValue(QLatin1String(showPrettyFormatC), showPrettyFormat);
    settings->endGroup();
}

bool GitSettings::equals(const GitSettings &s) const
{
    return adoptPath == s.adoptPath && path == s.path && logCount == s.logCount
           && timeoutSeconds == s.timeoutSeconds && promptToSubmit == s.promptToSubmit
           && pullRebase == s.pullRebase
           && omitAnnotationDate == s.omitAnnotationDate
           && ignoreSpaceChangesInBlame == s.ignoreSpaceChangesInBlame
           && ignoreSpaceChangesInDiff == s.ignoreSpaceChangesInDiff
           && diffPatience == s.diffPatience && winSetHomeEnvironment == s.winSetHomeEnvironment
           && gitkOptions == s.gitkOptions && showPrettyFormat == s.showPrettyFormat;
}

QString GitSettings::gitBinaryPath(bool *ok, QString *errorMessage) const
{
    // Locate binary in path if one is specified, otherwise default
    // to pathless binary
    if (ok)
        *ok = true;
    if (errorMessage)
        errorMessage->clear();
    const QString binary = QLatin1String(Constants::GIT_BINARY);
    QString currentPath = path;
    // Easy, git is assumed to be elsewhere accessible
    if (!adoptPath)
        currentPath = QString::fromLocal8Bit(qgetenv("PATH"));
    // Search in path?
    const QString pathBinary = Utils::SynchronousProcess::locateBinary(currentPath, binary);
    if (pathBinary.isEmpty()) {
        if (ok)
            *ok = false;
        if (errorMessage)
            *errorMessage = QCoreApplication::translate("Git::Internal::GitSettings",
                                                        "The binary '%1' could not be located in the path '%2'").arg(binary, path);
        return binary;
    }
    return pathBinary;
}

} // namespace Internal
} // namespace Git
