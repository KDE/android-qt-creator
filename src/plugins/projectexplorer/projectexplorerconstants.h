/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef PROJECTEXPLORERCONSTANTS_H
#define PROJECTEXPLORERCONSTANTS_H

#include <QtCore/QtGlobal>

namespace ProjectExplorer {
namespace Constants {

// Modes and their priorities
const char MODE_SESSION[]         = "Project";
const int  P_MODE_SESSION         = 85;

// Actions
const char NEWSESSION[]           = "ProjectExplorer.NewSession";
const char NEWPROJECT[]           = "ProjectExplorer.NewProject";
const char LOAD[]                 = "ProjectExplorer.Load";
const char UNLOAD[]               = "ProjectExplorer.Unload";
const char CLEARSESSION[]         = "ProjectExplorer.ClearSession";
const char BUILDPROJECTONLY[]     = "ProjectExplorer.BuildProjectOnly";
const char BUILD[]                = "ProjectExplorer.Build";
const char BUILDCM[]              = "ProjectExplorer.BuildCM";
const char BUILDSESSION[]         = "ProjectExplorer.BuildSession";
const char REBUILDPROJECTONLY[]   = "ProjectExplorer.RebuildProjectOnly";
const char REBUILD[]              = "ProjectExplorer.Rebuild";
const char REBUILDCM[]            = "ProjectExplorer.RebuildCM";
const char REBUILDSESSION[]       = "ProjectExplorer.RebuildSession";
const char DEPLOYPROJECTONLY[]    = "ProjectExplorer.DeployProjectOnly";
const char DEPLOY[]               = "ProjectExplorer.Deploy";
const char DEPLOYCM[]             = "ProjectExplorer.DeployCM";
const char DEPLOYSESSION[]        = "ProjectExplorer.DeploySession";
const char PUBLISH[]              = "ProjectExplorer.Publish";
const char CLEANPROJECTONLY[]     = "ProjectExplorer.CleanProjectOnly";
const char CLEAN[]                = "ProjectExplorer.Clean";
const char CLEANCM[]              = "ProjectExplorer.CleanCM";
const char CLEANSESSION[]         = "ProjectExplorer.CleanSession";
const char CANCELBUILD[]          = "ProjectExplorer.CancelBuild";
const char RUN[]                  = "ProjectExplorer.Run";
const char RUNCONTEXTMENU[]       = "ProjectExplorer.RunContextMenu";
const char STOP[]                 = "ProjectExplorer.Stop";
const char ADDNEWFILE[]           = "ProjectExplorer.AddNewFile";
const char ADDEXISTINGFILES[]     = "ProjectExplorer.AddExistingFiles";
const char ADDNEWSUBPROJECT[]     = "ProjectExplorer.AddNewSubproject";
const char REMOVEPROJECT[]        = "ProjectExplorer.RemoveProject";
const char OPENFILE[]             = "ProjectExplorer.OpenFile";
const char SEARCHONFILESYSTEM[]   = "ProjectExplorer.SearchOnFileSystem";
const char SHOWINGRAPHICALSHELL[] = "ProjectExplorer.ShowInGraphicalShell";
const char OPENTERMIANLHERE[]     = "ProjectExplorer.OpenTerminalHere";
const char REMOVEFILE[]           = "ProjectExplorer.RemoveFile";
const char DELETEFILE[]           = "ProjectExplorer.DeleteFile";
const char RENAMEFILE[]           = "ProjectExplorer.RenameFile";
const char SETSTARTUP[]           = "ProjectExplorer.SetStartup";
const char PROJECTTREE_COLLAPSE_ALL[] = "ProjectExplorer.CollapseAll";

const char SHOW_TASK_IN_EDITOR[]  = "ProjectExplorer.ShowTaskInEditor";
const char VCS_ANNOTATE_TASK[]    = "ProjectExplorer.VcsAnnotateTask";
const char SHOW_TASK_OUTPUT[]     = "ProjectExplorer.ShowTaskOutput";

// Run modes
const char RUNMODE[]              = "ProjectExplorer.RunMode";
const char SELECTTARGET[]         = "ProjectExplorer.SelectTarget";

// Action priorities
const int  P_ACTION_RUN            = 100;
const int  P_ACTION_BUILDPROJECT   = 80;

// Context
const char C_PROJECTEXPLORER[]    = "Project Explorer";
const char C_PROJECT_TREE[]       = "ProjectExplorer.ProjectTreeContext";
const char C_APP_OUTPUT[]         = "ProjectExplorer.ApplicationOutput";
const char C_COMPILE_OUTPUT[]     = "ProjectExplorer.CompileOutput";

// Languages
const char LANG_CXX[]             = "CXX";
const char LANG_QMLJS[]           = "QMLJS";

// Menus
const char M_RECENTPROJECTS[]     = "ProjectExplorer.Menu.Recent";
const char M_BUILDPROJECT[]       = "ProjectExplorer.Menu.Build";
const char M_DEBUG[]              = "ProjectExplorer.Menu.Debug";
const char M_DEBUG_STARTDEBUGGING[] = "ProjectExplorer.Menu.Debug.StartDebugging";
const char M_SESSION[]            = "ProjectExplorer.Menu.Session";

// Menu groups
const char G_BUILD_SESSION[]      = "ProjectExplorer.Group.BuildSession";
const char G_BUILD_PROJECT[]      = "ProjectExplorer.Group.Build";
const char G_BUILD_OTHER[]        = "ProjectExplorer.Group.Other";
const char G_BUILD_RUN[]          = "ProjectExplorer.Group.Run";
const char G_BUILD_CANCEL[]       = "ProjectExplorer.Group.BuildCancel";

// Context menus
const char M_SESSIONCONTEXT[]     = "Project.Menu.Session";
const char M_PROJECTCONTEXT[]     = "Project.Menu.Project";
const char M_SUBPROJECTCONTEXT[]  = "Project.Menu.SubProject";
const char M_FOLDERCONTEXT[]      = "Project.Menu.Folder";
const char M_FILECONTEXT[]        = "Project.Menu.File";
const char M_OPENFILEWITHCONTEXT[] = "Project.Menu.File.OpenWith";

// Context menu groups
const char G_SESSION_BUILD[]      = "Session.Group.Build";
const char G_SESSION_FILES[]      = "Session.Group.Files";
const char G_SESSION_OTHER[]      = "Session.Group.Other";
const char G_SESSION_CONFIG[]     = "Session.Group.Config";

const char G_PROJECT_FIRST[]      = "Project.Group.Open";
const char G_PROJECT_BUILD[]      = "Project.Group.Build";
const char G_PROJECT_RUN[]        = "Project.Group.Run";
const char G_PROJECT_FILES[]      = "Project.Group.Files";
const char G_PROJECT_TREE[]       = "Project.Group.Tree";
const char G_PROJECT_LAST[]       = "Project.Group.Last";

const char G_FOLDER_FILES[]       = "ProjectFolder.Group.Files";
const char G_FOLDER_OTHER[]       = "ProjectFolder.Group.Other";
const char G_FOLDER_CONFIG[]      = "ProjectFolder.Group.Config";

const char G_FILE_OPEN[]          = "ProjectFile.Group.Open";
const char G_FILE_OTHER[]         = "ProjectFile.Group.Other";
const char G_FILE_CONFIG[]        = "ProjectFile.Group.Config";

const char RUNMENUCONTEXTMENU[]   = "Project.RunMenu";

// File factory
const char FILE_FACTORY_ID[]      = "ProjectExplorer.FileFactoryId";

// Icons
const char ICON_BUILD[]           = ":/projectexplorer/images/build.png";
const char ICON_BUILD_SMALL[]     = ":/projectexplorer/images/build_small.png";
const char ICON_CLEAN[]           = ":/projectexplorer/images/clean.png";
const char ICON_CLEAN_SMALL[]     = ":/projectexplorer/images/clean_small.png";
const char ICON_REBUILD[]         = ":/projectexplorer/images/rebuild.png";
const char ICON_REBUILD_SMALL[]   = ":/projectexplorer/images/rebuild_small.png";
const char ICON_RUN[]             = ":/projectexplorer/images/run.png";
const char ICON_RUN_SMALL[]       = ":/projectexplorer/images/run_small.png";
const char ICON_DEBUG_SMALL[]     = ":/projectexplorer/images/debugger_start_small.png";
const char ICON_STOP[]            = ":/projectexplorer/images/stop.png";
const char ICON_STOP_SMALL[]      = ":/projectexplorer/images/stop_small.png";
const char ICON_TOOLCHAIN_SETTINGS_CATEGORY[] = ":projectexplorer/images/build.png"; // FIXME: Need an icon!
const char ICON_WINDOW[]          = ":/projectexplorer/images/window.png";

const char TASK_BUILD[]           = "ProjectExplorer.Task.Build";

// Mime types
const char C_SOURCE_MIMETYPE[]    = "text/x-csrc";
const char C_HEADER_MIMETYPE[]    = "text/x-chdr";
const char CPP_SOURCE_MIMETYPE[]  = "text/x-c++src";
const char CPP_HEADER_MIMETYPE[]  = "text/x-c++hdr";
const char FORM_MIMETYPE[]        = "application/x-designer";
const char QML_MIMETYPE[]         = "application/x-qml";
const char RESOURCE_MIMETYPE[]    = "application/vnd.nokia.xml.qt.resource";

// Settings page
const char PROJECTEXPLORER_SETTINGS_CATEGORY[]  = "K.ProjectExplorer";
const char PROJECTEXPLORER_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Projects");
const char PROJECTEXPLORER_SETTINGS_CATEGORY_ICON[]  = ":/core/images/category_project.png";
const char PROJECTEXPLORER_SETTINGS_ID[] = "ProjectExplorer.ProjectExplorer";
const char TOOLCHAIN_SETTINGS_CATEGORY[] = "ProjectExplorer.Settings.ToolChains";
const char TOOLCHAIN_SETTINGS_PAGE_ID[] = "M.ProjectExplorer.ToolChainOptions";

// Task categories
const char TASK_CATEGORY_COMPILE[] = "Task.Category.Compile";
const char TASK_CATEGORY_BUILDSYSTEM[] = "Task.Category.Buildsystem";

// Wizard category
const char PROJECT_WIZARD_CATEGORY[] = "I.Projects"; // (after Qt)
const char PROJECT_WIZARD_TR_SCOPE[] = "ProjectExplorer";
const char PROJECT_WIZARD_TR_CATEGORY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Other Project");

// Build step lists ids:
const char BUILDSTEPS_CLEAN[] = "ProjectExplorer.BuildSteps.Clean";
const char BUILDSTEPS_BUILD[] = "ProjectExplorer.BuildSteps.Build";
const char BUILDSTEPS_DEPLOY[] = "ProjectExplorer.BuildSteps.Deploy";

// Deploy Configuration id:
const char DEFAULT_DEPLOYCONFIGURATION_ID[] = "ProjectExplorer.DefaultDeployConfiguration";

// ToolChain Ids
const char CLANG_TOOLCHAIN_ID[] = "ProjectExplorer.ToolChain.Clang";
const char GCC_TOOLCHAIN_ID[] = "ProjectExplorer.ToolChain.Gcc";
const char LINUXICC_TOOLCHAIN_ID[] = "ProjectExplorer.ToolChain.LinuxIcc";
const char MINGW_TOOLCHAIN_ID[] = "ProjectExplorer.ToolChain.Mingw";
const char MSVC_TOOLCHAIN_ID[] = "ProjectExplorer.ToolChain.Msvc";

// Run Configuration defaults:
const int QML_DEFAULT_DEBUG_SERVER_PORT = 3768;

// Default directory to run custom (build) commands in.
const char DEFAULT_WORKING_DIR[] = "%{buildDir}";

// Settings files keys
const char SHARED_SETTINGS_KEYS_KEY[] = "ProjectExplorer.SharedSettingsKeysKey";
const char ALL_SETTINGS_KEYS_KEY[] = "ProjectExplorer.AllSettingsKeysKey";

} // namespace Constants
} // namespace ProjectExplorer

#endif // PROJECTEXPLORERCONSTANTS_H
