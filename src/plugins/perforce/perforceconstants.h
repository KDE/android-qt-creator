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

#ifndef PERFORCE_CONSTANTS_H
#define PERFORCE_CONSTANTS_H

#include <QtCore/QtGlobal>

namespace Perforce {
namespace Constants {

const char PERFORCE_SUBMIT_EDITOR_ID[] = "Perforce.SubmitEditor";
const char PERFORCE_SUBMIT_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce.SubmitEditor");
const char PERFORCESUBMITEDITOR_CONTEXT[] = "Perforce Submit Editor";

const char PERFORCE_COMMANDLOG_EDITOR_ID[] = "Perfoirce.CommandLogEditor";
const char PERFORCE_COMMANDLOG_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce CommandLog Editor");
const char PERFORCE_COMMANDLOG_EDITOR_CONTEXT[] = "Perforce Command Log Editor";

const char PERFORCE_LOG_EDITOR_ID[] = "Perforce.LogEditor";
const char PERFORCE_LOG_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce Log Editor");
const char PERFORCE_LOG_EDITOR_CONTEXT[] = "Perforce Log Editor";

const char PERFORCE_DIFF_EDITOR_ID[] = "Perforce.DiffEditor";
const char PERFORCE_DIFF_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce Diff Editor");
const char PERFORCE_DIFF_EDITOR_CONTEXT[] = "Perforce Diff Editor";

const char PERFORCE_ANNOTATION_EDITOR_ID[] = "Perforce.AnnotationEditor";
const char PERFORCE_ANNOTATION_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce Annotation Editor");
const char PERFORCE_ANNOTATION_EDITOR_CONTEXT[] = "Perforce Annotation Editor";

const char SUBMIT_CURRENT[] = "Perforce.SubmitCurrentLog";
const char DIFF_SELECTED[] = "Perforce.DiffSelectedFilesInLog";
const char SUBMIT_MIMETYPE[] = "application/vnd.nokia.text.p4.submit";

enum { debug = 0 };

} // Constants
} // Perforce

#endif // PERFORCE_CONSTANTS_H
