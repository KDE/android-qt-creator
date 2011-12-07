/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
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

#ifndef BAZAARCONSTANTS_H
#define BAZAARCONSTANTS_H

namespace Bazaar {
namespace Constants {

const char BAZAAR[] = "bazaar";
const char BAZAARREPO[] = ".bzr";
const char BAZAARDEFAULT[] = "bzr";

// Changeset identifiers
const char CHANGESET_ID[] = "([0-9]+)"; // match and capture
const char CHANGESET_ID_EXACT[] = "[0-9]+"; // match

// Base editor parameters
const char COMMANDLOG_ID[] = "Bazaar Command Log Editor";
const char COMMANDLOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Bazaar Command Log Editor");
const char COMMANDLOG[] = "Bazaar Command Log Editor";
const char COMMANDAPP[] = "application/vnd.nokia.text.scs_bazaar_commandlog";
const char COMMANDEXT[] = "vcsBazaarCommand";

const char FILELOG_ID[] = "Bazaar File Log Editor";
const char FILELOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Bazaar File Log Editor");
const char FILELOG[] = "Bazaar File Log Editor";
const char LOGAPP[] = "application/vnd.nokia.text.scs_bazaar_log";
const char LOGEXT[] = "vcsBazaarLog";

const char ANNOTATELOG_ID[] = "Bazaar Annotation Editor";
const char ANNOTATELOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Bazaar Annotation Editor");
const char ANNOTATELOG[] = "Bazaar Annotation Editor";
const char ANNOTATEAPP[] = "application/vnd.nokia.text.scs_bazaar_annotatelog";
const char ANNOTATEEXT[] = "vcsBazaarAnnotate";

const char DIFFLOG_ID[] = "Bazaar Diff Editor";
const char DIFFLOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Bazaar Diff Editor");
const char DIFFLOG[] = "Bazaar Diff Editor";
const char DIFFAPP[] = "text/x-patch";
const char DIFFEXT[] = "diff";

// Submit editor parameters
const char COMMIT_ID[] = "Bazaar Commit Log Editor";
const char COMMIT_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Bazaar Commit Log Editor");
const char COMMITMIMETYPE[] = "application/vnd.nokia.text.scs_bazaar_commitlog";

// Menu items
// File menu actions
const char ADD[] = "Bazaar.AddSingleFile";
const char DELETE[] = "Bazaar.DeleteSingleFile";
const char ANNOTATE[] = "Bazaar.Annotate";
const char DIFF[] = "Bazaar.DiffSingleFile";
const char LOG[] = "Bazaar.LogSingleFile";
const char REVERT[] = "Bazaar.RevertSingleFile";
const char STATUS[] = "Bazaar.Status";

// Directory menu Actions
const char DIFFMULTI[] = "Bazaar.Action.DiffMulti";
const char REVERTMULTI[] = "Bazaar.Action.RevertALL";
const char STATUSMULTI[] = "Bazaar.Action.StatusMulti";
const char LOGMULTI[] = "Bazaar.Action.Logmulti";

// Repository menu actions
const char PULL[] = "Bazaar.Action.Pull";
const char PUSH[] = "Bazaar.Action.Push";
const char UPDATE[] = "Bazaar.Action.Update";
const char COMMIT[] = "Bazaar.Action.Commit";
const char CREATE_REPOSITORY[] = "Bazaar.Action.CreateRepository";

// Submit editor actions
const char DIFFEDITOR[] = "Bazaar.Action.Editor.Diff";

} // namespace Constants
} // namespace Bazaar

#endif // BAZAARCONSTANTS_H
