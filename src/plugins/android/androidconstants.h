/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDCONSTANTS_H
#define ANDROIDCONSTANTS_H

#include <QtCore/QLatin1String>

namespace Android {
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
#define ANDROID_EXE_SUFFIX ".exe"
#define ANDROID_BAT_SUFFIX ".bat"
#else
#define ANDROID_EXE_SUFFIX ""
#define ANDROID_BAT_SUFFIX ""
#endif

static const QLatin1String ANDROID_RC_ID(ANDROID_PREFIX);
static const QLatin1String ANDROID_RC_ID_PREFIX(ANDROID_PREFIX ".");

static const QLatin1String AndroidArgumentsKey(ANDROID_PREFIX ".Arguments");
static const QLatin1String AndroidSimulatorPathKey(ANDROID_PREFIX ".Simulator");
static const QLatin1String AndroidDeviceIdKey(ANDROID_PREFIX ".DeviceId");
static const QLatin1String AndroidProFileKey(ANDROID_PREFIX ".ProFile");
static const QLatin1String AndroidExportedLocalDirsKey(ANDROID_PREFIX ".ExportedLocalDirs");
static const QLatin1String AndroidBaseEnvironmentBaseKey(ANDROID_PREFIX ".BaseEnvironmentBase");
static const QLatin1String AndroidUserEnvironmentChangesKey(ANDROID_PREFIX ".UserEnvironmentChanges");
static const QLatin1String AndroidUseRemoteGdbKey(ANDROID_PREFIX ".UseRemoteGdb");

} // namespace Internal

namespace Constants {
const char * const ANDROID_SETTINGS_ID = "ZZ.Android Configurations";
const char * const ANDROID_SETTINGS_CATEGORY = "X.Android";
const char * const ANDROID_SETTINGS_TR_CATEGORY = QT_TRANSLATE_NOOP("Android", "Android");
const char * const ANDROID_SETTINGS_CATEGORY_ICON = ":/android/images/AndroidDevice.png";
const char * const ANDROID_TOOLCHAIN_ID = "Qt4ProjectManager.ToolChain.Android";
const char * const ANDROIDQT     = "Qt4ProjectManager.QtVersion.Android";

}
} // namespace Android

#endif  // ANDROIDCONSTANTS_H