/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#include "mercurialeditor.h"
#include "annotationhighlighter.h"
#include "constants.h"
#include "mercurialplugin.h"
#include "mercurialclient.h"

#include <coreplugin/editormanager/editormanager.h>
#include <vcsbase/diffhighlighter.h>

#include <QtCore/QString>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

using namespace Mercurial::Internal;
using namespace Mercurial;

MercurialEditor::MercurialEditor(const VCSBase::VCSBaseEditorParameters *type, QWidget *parent)
        : VCSBase::VCSBaseEditorWidget(type, parent),
        exactIdentifier12(QLatin1String(Constants::CHANGEIDEXACT12)),
        exactIdentifier40(QLatin1String(Constants::CHANGEIDEXACT40)),
        changesetIdentifier12(QLatin1String(Constants::CHANGESETID12)),
        changesetIdentifier40(QLatin1String(Constants::CHANGESETID40)),
        diffIdentifier(QLatin1String(Constants::DIFFIDENTIFIER))
{
    setAnnotateRevisionTextFormat(tr("Annotate %1"));
    setAnnotatePreviousRevisionTextFormat(tr("Annotate parent revision %1"));
}

QSet<QString> MercurialEditor::annotationChanges() const
{
    QSet<QString> changes;
    const QString data = toPlainText();
    if (data.isEmpty())
        return changes;

    int position = 0;
    while ((position = changesetIdentifier12.indexIn(data, position)) != -1) {
        changes.insert(changesetIdentifier12.cap(1));
        position += changesetIdentifier12.matchedLength();
    }

    return changes;
}

QString MercurialEditor::changeUnderCursor(const QTextCursor &cursorIn) const
{
    QTextCursor cursor = cursorIn;
    cursor.select(QTextCursor::WordUnderCursor);
    if (cursor.hasSelection()) {
        const QString change = cursor.selectedText();
        if (exactIdentifier12.exactMatch(change))
            return change;
        if (exactIdentifier40.exactMatch(change))
            return change;
    }
    return QString();
}

VCSBase::DiffHighlighter *MercurialEditor::createDiffHighlighter() const
{
    return new VCSBase::DiffHighlighter(diffIdentifier);
}

VCSBase::BaseAnnotationHighlighter *MercurialEditor::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new MercurialAnnotationHighlighter(changes);
}

QString MercurialEditor::fileNameFromDiffSpecification(const QTextBlock &inBlock) const
{
    // git-compatible format: check for "+++ b/src/plugins/git/giteditor.cpp" (blame and diff)
    // Go back chunks.
    const QString newFileIndicator = QLatin1String("+++ b/");
    for (QTextBlock  block = inBlock; block.isValid(); block = block.previous()) {
        QString diffFileName = block.text();
        if (diffFileName.startsWith(newFileIndicator)) {
            diffFileName.remove(0, newFileIndicator.size());
            return findDiffFile(diffFileName, MercurialPlugin::instance()->versionControl());
        }

    }
    return QString();
}

QStringList MercurialEditor::annotationPreviousVersions(const QString &revision) const
{
    MercurialClient *client = MercurialPlugin::instance()->client();
    QStringList parents;
    const QFileInfo fi(source());
    const QString workingDirectory = fi.absolutePath();
    // Retrieve parent revisions
    QStringList revisions;
    if (!client->parentRevisionsSync(workingDirectory, fi.fileName(), revision, &revisions))
        return QStringList();
    // Format with short summary
    QStringList descriptions;
    if (!client->shortDescriptionsSync(workingDirectory, revisions, &descriptions))
        return QStringList();
    return descriptions;
}

