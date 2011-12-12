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

#include "gitsettings.h"

#include <utils/synchronousprocess.h>

#include <QtCore/QCoreApplication>

namespace Git {
namespace Internal {

const QLatin1String GitSettings::adoptPathKey("SysEnv");
const QLatin1String GitSettings::pathKey("Path");
const QLatin1String GitSettings::pullRebaseKey("PullRebase");
const QLatin1String GitSettings::omitAnnotationDateKey("OmitAnnotationDate");
const QLatin1String GitSettings::ignoreSpaceChangesInDiffKey("SpaceIgnorantDiff");
const QLatin1String GitSettings::ignoreSpaceChangesInBlameKey("SpaceIgnorantBlame");
const QLatin1String GitSettings::diffPatienceKey("DiffPatience");
const QLatin1String GitSettings::winSetHomeEnvironmentKey("WinSetHomeEnvironment");
const QLatin1String GitSettings::showPrettyFormatKey("DiffPrettyFormat");
const QLatin1String GitSettings::gitkOptionsKey("GitKOptions");
const QLatin1String GitSettings::logDiffKey("LogDiff");

GitSettings::GitSettings()
{
    setSettingsGroup(QLatin1String("Git"));

    declareKey(binaryPathKey, QLatin1String("git"));
#ifdef Q_OS_WIN
    declareKey(timeoutKey, 60);
#else
    declareKey(timeoutKey, 30);
#endif
    declareKey(adoptPathKey, false);
    declareKey(pathKey, QString());
    declareKey(pullRebaseKey, false);
    declareKey(omitAnnotationDateKey, false);
    declareKey(ignoreSpaceChangesInDiffKey, true);
    declareKey(ignoreSpaceChangesInBlameKey, true);
    declareKey(diffPatienceKey, true);
    declareKey(winSetHomeEnvironmentKey, false);
    declareKey(gitkOptionsKey, QString());
    declareKey(showPrettyFormatKey, 2);
    declareKey(logDiffKey, false);
}

QString GitSettings::gitBinaryPath(bool *ok, QString *errorMessage) const
{
    // Locate binary in path if one is specified, otherwise default
    // to pathless binary
    if (ok)
        *ok = true;
    if (errorMessage)
        errorMessage->clear();

    if (m_binaryPath.isEmpty()) {
        const QString binary = stringValue(binaryPathKey);
        QString currentPath = stringValue(pathKey);
        // Easy, git is assumed to be elsewhere accessible
        if (!boolValue(adoptPathKey))
            currentPath = QString::fromLocal8Bit(qgetenv("PATH"));
        // Search in path?
        m_binaryPath = Utils::SynchronousProcess::locateBinary(currentPath, binary);
        if (m_binaryPath.isEmpty()) {
            if (ok)
                *ok = false;
            if (errorMessage)
                *errorMessage = QCoreApplication::translate("Git::Internal::GitSettings",
                                                            "The binary '%1' could not be located in the path '%2'")
                    .arg(binary, currentPath);
        }
    }
    return m_binaryPath;
}

GitSettings &GitSettings::operator = (const GitSettings &s)
{
    VCSBaseClientSettings::operator =(s);
    m_binaryPath.clear();
    return *this;
}

} // namespace Internal
} // namespace Git
