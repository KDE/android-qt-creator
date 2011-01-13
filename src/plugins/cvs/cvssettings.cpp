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

#include "cvssettings.h"

#include <QtCore/QSettings>
#include <QtCore/QTextStream>

static const char *groupC = "CVS";
static const char *commandKeyC = "Command";
static const char *rootC = "Root";
static const char *promptToSubmitKeyC = "PromptForSubmit";
static const char *diffOptionsKeyC = "DiffOptions";
static const char *describeByCommitIdKeyC = "DescribeByCommitId";
static const char *defaultDiffOptions = "-du";
static const char *timeOutKeyC = "TimeOut";

enum { defaultTimeOutS = 30 };

static QString defaultCommand()
{
    QString rc;
    rc = QLatin1String("cvs");
#if defined(Q_OS_WIN32)
    rc.append(QLatin1String(".exe"));
#endif
    return rc;
}

namespace CVS {
namespace Internal {

CVSSettings::CVSSettings() :
    cvsCommand(defaultCommand()),
    cvsDiffOptions(QLatin1String(defaultDiffOptions)),
    timeOutS(defaultTimeOutS),
    promptToSubmit(true),
    describeByCommitId(true)
{
}

void CVSSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(groupC));
    cvsCommand = settings->value(QLatin1String(commandKeyC), defaultCommand()).toString();
    promptToSubmit = settings->value(QLatin1String(promptToSubmitKeyC), true).toBool();
    cvsRoot = settings->value(QLatin1String(rootC), QString()).toString();
    cvsDiffOptions = settings->value(QLatin1String(diffOptionsKeyC), QLatin1String(defaultDiffOptions)).toString();
    describeByCommitId = settings->value(QLatin1String(describeByCommitIdKeyC), true).toBool();
    timeOutS = settings->value(QLatin1String(timeOutKeyC), defaultTimeOutS).toInt();
    settings->endGroup();
}

void CVSSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(QLatin1String(commandKeyC), cvsCommand);
    settings->setValue(QLatin1String(promptToSubmitKeyC), promptToSubmit);
    settings->setValue(QLatin1String(rootC), cvsRoot);
    settings->setValue(QLatin1String(diffOptionsKeyC), cvsDiffOptions);
    settings->setValue(QLatin1String(timeOutKeyC), timeOutS);
    settings->setValue(QLatin1String(describeByCommitIdKeyC), describeByCommitId);
    settings->endGroup();
}

bool CVSSettings::equals(const CVSSettings &s) const
{
    return promptToSubmit     == promptToSubmit
        && describeByCommitId == s.describeByCommitId
        && cvsCommand         == s.cvsCommand
        && cvsRoot            == s.cvsRoot
        && timeOutS           == s.timeOutS
        && cvsDiffOptions     == s.cvsDiffOptions;
}

QStringList CVSSettings::addOptions(const QStringList &args) const
{
    if (cvsRoot.isEmpty())
        return args;

    QStringList rc;
    rc.push_back(QLatin1String("-d"));
    rc.push_back(cvsRoot);
    rc.append(args);
    return rc;
}

} // namespace Internal
} // namespace CVS
