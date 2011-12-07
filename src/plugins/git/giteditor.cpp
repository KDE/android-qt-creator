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

#include "giteditor.h"

#include "annotationhighlighter.h"
#include "gitconstants.h"
#include "gitplugin.h"
#include "gitclient.h"
#include "gitsettings.h"
#include <QtCore/QTextCodec>

#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>
#include <vcsbase/diffhighlighter.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>
#include <QtCore/QSet>
#include <QtCore/QTextStream>

#include <QtGui/QTextCursor>
#include <QtGui/QTextEdit>
#include <QtGui/QTextBlock>

#define CHANGE_PATTERN_8C "[a-f0-9]{7,8}"
#define CHANGE_PATTERN_40C "[a-f0-9]{40,40}"

namespace Git {
namespace Internal {

// ------------ GitEditor
GitEditor::GitEditor(const VCSBase::VCSBaseEditorParameters *type,
                     QWidget *parent)  :
    VCSBase::VCSBaseEditorWidget(type, parent),
    m_changeNumberPattern8(QLatin1String(CHANGE_PATTERN_8C)),
    m_changeNumberPattern40(QLatin1String(CHANGE_PATTERN_40C))
{
    QTC_ASSERT(m_changeNumberPattern8.isValid(), return);
    QTC_ASSERT(m_changeNumberPattern40.isValid(), return);
    setAnnotateRevisionTextFormat(tr("Blame %1"));
    setAnnotatePreviousRevisionTextFormat(tr("Blame parent revision %1"));
}

QSet<QString> GitEditor::annotationChanges() const
{
    QSet<QString> changes;
    const QString txt = toPlainText();
    if (txt.isEmpty())
        return changes;
    // Hunt for first change number in annotation: "<change>:"
    QRegExp r(QLatin1String("^(" CHANGE_PATTERN_8C ") "));
    QTC_ASSERT(r.isValid(), return changes);
    if (r.indexIn(txt) != -1) {
        changes.insert(r.cap(1));
        r.setPattern(QLatin1String("\n(" CHANGE_PATTERN_8C ") "));
        QTC_ASSERT(r.isValid(), return changes);
        int pos = 0;
        while ((pos = r.indexIn(txt, pos)) != -1) {
            pos += r.matchedLength();
            changes.insert(r.cap(1));
        }
    }
    return changes;
}

QString GitEditor::changeUnderCursor(const QTextCursor &c) const
{
    QTextCursor cursor = c;
    // Any number is regarded as change number.
    cursor.select(QTextCursor::WordUnderCursor);
    if (!cursor.hasSelection())
        return QString();
    const QString change = cursor.selectedText();
    if (m_changeNumberPattern8.exactMatch(change))
        return change;
    if (m_changeNumberPattern40.exactMatch(change))
        return change;
    return QString();
}

VCSBase::DiffHighlighter *GitEditor::createDiffHighlighter() const
{
    const QRegExp filePattern(QLatin1String("^(diff --git a/|index |[+-][+-][+-] [ab]).*$"));
    return new VCSBase::DiffHighlighter(filePattern);
}

VCSBase::BaseAnnotationHighlighter *GitEditor::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new GitAnnotationHighlighter(changes);
}

QString GitEditor::fileNameFromDiffSpecification(const QTextBlock &inBlock) const
{
        // Check for "+++ b/src/plugins/git/giteditor.cpp" (blame and diff)
    // Go back chunks.
    const QString newFileIndicator = QLatin1String("+++ b/");
    for (QTextBlock  block = inBlock; block.isValid(); block = block.previous()) {
        QString diffFileName = block.text();
        if (diffFileName.startsWith(newFileIndicator)) {
            diffFileName.remove(0, newFileIndicator.size());
            return findDiffFile(diffFileName, GitPlugin::instance()->versionControl());
        }
    }
    return QString();
}

/* Remove the date specification from annotation, which is tabular:
\code
8ca887aa (author               YYYY-MM-DD HH:MM:SS <offset>  <line>)<content>
\endcode */

static QByteArray removeAnnotationDate(const QByteArray &b)
{
    if (b.isEmpty())
        return QByteArray();

    const int parenPos = b.indexOf(')');
    if (parenPos == -1)
        return QByteArray(b);
    int datePos = parenPos;

    int i = parenPos;
    while (i >= 0 && b.at(i) != ' ')
        --i;
    while (i >= 0 && b.at(i) == ' ')
        --i;
    int spaceCount = 0;
    // i is now on timezone. Go back 3 spaces: That is where the date starts.
    while (i >= 0) {
        if (b.at(i) == ' ')
            ++spaceCount;
        if (spaceCount == 3) {
            datePos = i;
            break;
        }
        --i;
    }
    if (datePos == 0)
        return QByteArray(b);

    // Copy over the parts that have not changed into a new byte array
    Q_ASSERT(b.size() >= parenPos);
    QByteArray result;
    int prevPos = 0;
    int pos = b.indexOf('\n', 0) + 1;
    forever {
        Q_ASSERT(prevPos < pos);
        int afterParen = prevPos + parenPos;
        result.append(b.constData() + prevPos, datePos);
        result.append(b.constData() + afterParen, pos - afterParen);
        prevPos = pos;
        Q_ASSERT(prevPos != 0);
        if (pos == b.size())
            break;

        pos = b.indexOf('\n', pos) + 1;
        if (pos == 0) // indexOf returned -1
            pos = b.size();
    }
    return result;
}

void GitEditor::setPlainTextDataFiltered(const QByteArray &a)
{
    QByteArray array = a;
    // If desired, filter out the date from annotation
    const bool omitAnnotationDate = contentType() == VCSBase::AnnotateOutput
                                    && GitPlugin::instance()->settings().boolValue(GitSettings::omitAnnotationDateKey);
    if (omitAnnotationDate)
        array = removeAnnotationDate(a);
    setPlainTextData(array);
}

void GitEditor::commandFinishedGotoLine(bool ok, int /* exitCode */, const QVariant &v)
{
    if (ok && v.type() == QVariant::Int) {
        const int line = v.toInt();
        if (line >= 0)
            gotoLine(line);
    }
}

QStringList GitEditor::annotationPreviousVersions(const QString &revision) const
{
    QStringList revisions;
    QString errorMessage;
    GitClient *client = GitPlugin::instance()->gitClient();
    const QFileInfo fi(source());
    const QString workingDirectory = fi.absolutePath();
    // Get the SHA1's of the file.
    if (!client->synchronousParentRevisions(workingDirectory, QStringList(fi.fileName()),
                                            revision, &revisions, &errorMessage)) {
        VCSBase::VCSBaseOutputWindow::instance()->appendSilently(errorMessage);
        return QStringList();
    }
    // Format verbose, SHA1 being first token
    QStringList descriptions;
    if (!client->synchronousShortDescriptions(workingDirectory, revisions, &descriptions, &errorMessage)) {
        VCSBase::VCSBaseOutputWindow::instance()->appendSilently(errorMessage);
        return QStringList();
    }
    return descriptions;
}

} // namespace Internal
} // namespace Git

