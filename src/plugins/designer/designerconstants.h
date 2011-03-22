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

#ifndef DESIGNERPLUGIN_CONSTANTS_H
#define DESIGNERPLUGIN_CONSTANTS_H

#include <QtCore/QtGlobal>

namespace Designer {
namespace Constants {

const char * const INFO_READ_ONLY = "DesignerXmlEditor.ReadOnly";
const char * const K_DESIGNER_XML_EDITOR_ID = "FormEditor.DesignerXmlEditor";
const char * const C_DESIGNER_XML_EDITOR = "Designer Xml Editor";
const char * const C_DESIGNER_XML_DISPLAY_NAME  = QT_TRANSLATE_NOOP("Designer", "Form Editor");

const char * const SETTINGS_CATEGORY = "P.Designer";
const char * const SETTINGS_CATEGORY_ICON = ":/core/images/category_design.png";
const char * const SETTINGS_TR_CATEGORY = QT_TRANSLATE_NOOP("Designer", "Designer");
const char * const SETTINGS_CPP_SETTINGS_ID = "Class Generation";
const char * const SETTINGS_CPP_SETTINGS_NAME = QT_TRANSLATE_NOOP("Designer", "Class Generation");

// context
const char * const C_FORMEDITOR       = "FormEditor.FormEditor";
const char * const FORMEDITOR_ID      = "FormEditor";
const char * const C_FORMEDITOR_DISPLAY_NAME  = QT_TRANSLATE_NOOP("Designer", "Form Editor");
const char * const T_FORMEDITOR         = "FormEditor.Toolbar";
const char * const M_FORMEDITOR         = "FormEditor.Menu";
const char * const M_FORMEDITOR_VIEWS   = "FormEditor.Menu.Views";
const char * const M_FORMEDITOR_PREVIEW = "FormEditor.Menu.Preview";

// Wizard type
const char * const FORM_FILE_TYPE       = "Qt4FormFiles";
const char * const FORM_MIMETYPE = "application/x-designer";

enum DesignerSubWindows
{
    WidgetBoxSubWindow,
    ObjectInspectorSubWindow,
    PropertyEditorSubWindow,
    SignalSlotEditorSubWindow,
    ActionEditorSubWindow,
    DesignerSubWindowCount
};

enum EditModes
{
    EditModeWidgetEditor,
    EditModeSignalsSlotEditor,
    EditModeBuddyEditor,
    EditModeTabOrderEditor,
    NumEditModes
};

namespace Internal {
    enum { debug = 0 };
}
} // Constants
} // Designer

#endif //DESIGNERPLUGIN_CONSTANTS_H
