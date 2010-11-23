/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef ANDROIDCONSTANTS_H
#define ANDROIDCONSTANTS_H

#include <QtCore/QLatin1String>

namespace Qt4ProjectManager {
namespace Internal {

enum AndroidQemuStatus {
    AndroidQemuStarting,
    AndroidQemuFailedToStart,
    AndroidQemuFinished,
    AndroidQemuCrashed,
    AndroidQemuUserReason
};

#define ANDROID_PREFIX "Qt4ProjectManager.AndroidRunConfiguration"

#ifdef Q_OS_WIN32
#define ANDROID_EXEC_SUFFIX ".exe"
#else
#define ANDROID_EXEC_SUFFIX ""
#endif

static const QLatin1String ANDROID_RC_ID(ANDROID_PREFIX);
static const QLatin1String ANDROID_RC_ID_PREFIX(ANDROID_PREFIX ".");

static const QLatin1String AndroidArgumentsKey(ANDROID_PREFIX ".Arguments");
static const QLatin1String AndroidSimulatorPathKey(ANDROID_PREFIX ".Simulator");
static const QLatin1String AndroidDeviceIdKey(ANDROID_PREFIX ".DeviceId");
static const QLatin1String AndroidLastDeployedHostsKey(ANDROID_PREFIX ".LastDeployedHosts");
static const QLatin1String AndroidLastDeployedFilesKey(ANDROID_PREFIX ".LastDeployedFiles");
static const QLatin1String AndroidLastDeployedRemotePathsKey(ANDROID_PREFIX ".LastDeployedRemotePaths");
static const QLatin1String AndroidLastDeployedTimesKey(ANDROID_PREFIX ".LastDeployedTimes");
static const QLatin1String AndroidDeployToSysrootKey(ANDROID_PREFIX ".DeployToSysroot");
static const QLatin1String AndroidProFileKey(ANDROID_PREFIX ".ProFile");
static const QLatin1String AndroidExportedLocalDirsKey(ANDROID_PREFIX ".ExportedLocalDirs");
static const QLatin1String AndroidRemoteMountPointsKey(ANDROID_PREFIX ".RemoteMountPoints");
static const QLatin1String AndroidBaseEnvironmentBaseKey(ANDROID_PREFIX ".BaseEnvironmentBase");
static const QLatin1String AndroidUserEnvironmentChangesKey(ANDROID_PREFIX ".UserEnvironmentChanges");
static const QLatin1String AndroidUseRemoteGdbKey(ANDROID_PREFIX ".UseRemoteGdb");

} // namespace Internal
} // namespace Qt4ProjectManager

#endif  // ANDROIDCONSTANTS_H
