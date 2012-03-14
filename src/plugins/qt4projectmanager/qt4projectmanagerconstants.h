/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QT4PROJECTMANAGERCONSTANTS_H
#define QT4PROJECTMANAGERCONSTANTS_H

#include <QtGlobal>

namespace Qt4ProjectManager {
namespace Constants {

// Contexts
const char C_PROFILEEDITOR[] = ".pro File Editor";

// Menus
const char M_CONTEXT[] = "ProFileEditor.ContextMenu";

// Kinds
const char PROJECT_ID[] = "Qt4.Qt4Project";
const char PROFILE_EDITOR_ID[] = "Qt4.proFileEditor";
const char PROFILE_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", ".pro File Editor");
const char PROFILE_MIMETYPE [] = "application/vnd.nokia.qt.qmakeprofile";
const char PROINCLUDEFILE_MIMETYPE [] = "application/vnd.nokia.qt.qmakeproincludefile";
const char PROFEATUREFILE_MIMETYPE [] = "application/vnd.nokia.qt.qmakeprofeaturefile";
const char CPP_SOURCE_MIMETYPE[] = "text/x-c++src";
const char CPP_HEADER_MIMETYPE[] = "text/x-c++hdr";
const char FORM_MIMETYPE[] = "application/x-designer";
const char LINGUIST_MIMETYPE[] = "application/x-linguist";

// Actions
const char RUNQMAKE[] = "Qt4Builder.RunQMake";
const char RUNQMAKECONTEXTMENU[] = "Qt4Builder.RunQMakeContextMenu";
const char BUILDSUBDIR[] = "Qt4Builder.BuildSubDir";
const char REBUILDSUBDIR[] = "Qt4Builder.RebuildSubDir";
const char CLEANSUBDIR[] = "Qt4Builder.CleanSubDir";
const char ADDLIBRARY[] = "Qt4.AddLibrary";
const char JUMP_TO_FILE[] = "Qt4.JumpToFile";
const char SEPARATOR[] = "Qt4.Separator";

// Tasks
const char PROFILE_EVALUATE[] = "Qt4ProjectManager.ProFileEvaluate";

// Project
const char QT4PROJECT_ID[] = "Qt4ProjectManager.Qt4Project";

// Targets
const char DESKTOP_TARGET_ID[] = "Qt4ProjectManager.Target.DesktopTarget";
const char S60_EMULATOR_TARGET_ID[] = "Qt4ProjectManager.Target.S60EmulatorTarget";
const char S60_DEVICE_TARGET_ID[] = "Qt4ProjectManager.Target.S60DeviceTarget";
const char MAEMO5_DEVICE_TARGET_ID[] = "Qt4ProjectManager.Target.MaemoDeviceTarget";
const char HARMATTAN_DEVICE_TARGET_ID[] = "Qt4ProjectManager.Target.HarmattanDeviceTarget";
const char MEEGO_DEVICE_TARGET_ID[] = "Qt4ProjectManager.Target.MeegoDeviceTarget";
const char QT_SIMULATOR_TARGET_ID[] = "Qt4ProjectManager.Target.QtSimulatorTarget";
const char ANDROID_DEVICE_TARGET_ID[] = "Qt4ProjectManager.Target.AndroidDeviceTarget";

// Target Features
const char MOBILE_TARGETFEATURE_ID[] = "Qt4ProjectManager.TargetFeature.Mobile";
const char DESKTOP_TARGETFEATURE_ID[] = "Qt4ProjectManager.TargetFeature.Desktop";
const char SHADOWBUILD_TARGETFEATURE_ID[] = "Qt4ProjectManager.TargetFeature.ShadowBuild";

// Tool chains:
const char GCCE_TOOLCHAIN_ID[] = "Qt4ProjectManager.ToolChain.GCCE";
const char MAEMO_TOOLCHAIN_ID[] = "Qt4ProjectManager.ToolChain.Maemo";
const char RVCT_TOOLCHAIN_ID[] = "Qt4ProjectManager.ToolChain.RVCT";
const char WINSCW_TOOLCHAIN_ID[] = "Qt4ProjectManager.ToolChain.WINSCW";

// ICONS
const char ICON_QT_PROJECT[] = ":/qt4projectmanager/images/qt_project.png";
const char ICON_QTQUICK_APP[] = ":/wizards/images/qtquickapp.png";
const char ICON_HTML5_APP[] = ":/wizards/images/html5app.png";

// Env variables
const char QMAKEVAR_QMLJSDEBUGGER_PATH[] = "QMLJSDEBUGGER_PATH";
const char QMAKEVAR_DECLARATIVE_DEBUG[] = "CONFIG+=declarative_debug";

// Unconfigured Settings page
const char UNCONFIGURED_SETTINGS_PAGE_ID[] = "R.UnconfiguredSettings";
const char UNCONFIGURED_SETTINGS_PAGE_NAME[] = QT_TRANSLATE_NOOP("Qt4ProjectManager", "Unconfigured Project");

// Unconfigured Panel
const char UNCONFIGURED_PANEL_PAGE_ID[] = "UnconfiguredPanel";

} // namespace Constants
} // namespace Qt4ProjectManager

#endif // QT4PROJECTMANAGERCONSTANTS_H

