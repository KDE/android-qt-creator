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

#include "cvseditor.h"
#include "cvsutils.h"

#include "annotationhighlighter.h"
#include "cvsconstants.h"

#include <utils/qtcassert.h>
#include <vcsbase/diffhighlighter.h>

#include <QtCore/QDebug>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>

namespace CVS {
namespace Internal {

// Match a CVS revision ("1.1.1.1")
#define CVS_REVISION_PATTERN "[\\d\\.]+"
#define CVS_REVISION_AT_START_PATTERN "^("CVS_REVISION_PATTERN") "

CVSEditor::CVSEditor(const VCSBase::VCSBaseEditorParameters *type,
                                   QWidget *parent) :
    VCSBase::VCSBaseEditor(type, parent),
    m_revisionAnnotationPattern(QLatin1String(CVS_REVISION_AT_START_PATTERN".*$")),
    m_revisionLogPattern(QLatin1String("^revision  *("CVS_REVISION_PATTERN")$"))
{
    QTC_ASSERT(m_revisionAnnotationPattern.isValid(), return);
    QTC_ASSERT(m_revisionLogPattern.isValid(), return);
    setAnnotateRevisionTextFormat(tr("Annotate revision \"%1\""));
}

QSet<QString> CVSEditor::annotationChanges() const
{
    QSet<QString> changes;
    const QString txt = toPlainText();
    if (txt.isEmpty())
        return changes;
    // Hunt for first change number in annotation: "1.1 (author)"
    QRegExp r(QLatin1String(CVS_REVISION_AT_START_PATTERN));
    QTC_ASSERT(r.isValid(), return changes);
    if (r.indexIn(txt) != -1) {
        changes.insert(r.cap(1));
        r.setPattern(QLatin1String("\n("CVS_REVISION_PATTERN") "));
        QTC_ASSERT(r.isValid(), return changes);
        int pos = 0;
        while ((pos = r.indexIn(txt, pos)) != -1) {
            pos += r.matchedLength();
            changes.insert(r.cap(1));
        }
    }
    if (CVS::Constants::debug)
        qDebug() << "CVSEditor::annotationChanges() returns #" << changes.size();
    return changes;
}

QString CVSEditor::changeUnderCursor(const QTextCursor &c) const
{
    // Try to match "1.1" strictly:
    // 1) Annotation: Check for a revision number at the beginning of the line.
    //    Note that "cursor.select(QTextCursor::WordUnderCursor)" will
    //    only select the part up until the dot.
    //    Check if we are at the beginning of a line within a reasonable offset.
    // 2) Log: check for lines like "revision 1.1", cursor past "revision"
    switch (contentType()) {
    case VCSBase::RegularCommandOutput:
    case VCSBase::DiffOutput:
        break;
    case VCSBase::AnnotateOutput: {
            const QTextBlock block = c.block();
            if (c.atBlockStart() || (c.position() - block.position() < 3)) {
                const QString line = block.text();
                if (m_revisionAnnotationPattern.exactMatch(line))
                    return m_revisionAnnotationPattern.cap(1);
            }
        }
        break;
    case VCSBase::LogOutput: {
            const QTextBlock block = c.block();
            if (c.position() - block.position() > 8 && m_revisionLogPattern.exactMatch(block.text()))
                return m_revisionLogPattern.cap(1);
        }
        break;
    }
    return QString();
}

/* \code
cvs diff -d -u -r1.1 -r1.2:
--- mainwindow.cpp<\t>13 Jul 2009 13:50:15 -0000 <\t>1.1
+++ mainwindow.cpp<\t>14 Jul 2009 07:09:24 -0000<\t>1.2
@@ -6,6 +6,5 @@
\endcode
*/

VCSBase::DiffHighlighter *CVSEditor::createDiffHighlighter() const
{
    const QRegExp filePattern(QLatin1String("^[-+][-+][-+] .*1\\.[\\d\\.]+$"));
    QTC_ASSERT(filePattern.isValid(), /**/);
    return new VCSBase::DiffHighlighter(filePattern);
}

VCSBase::BaseAnnotationHighlighter *CVSEditor::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new CVSAnnotationHighlighter(changes);
}

QString CVSEditor::fileNameFromDiffSpecification(const QTextBlock &inBlock) const
{
    // "+++ mainwindow.cpp<\t>13 Jul 2009 13:50:15 -0000      1.1"
    // Go back chunks
    const QString diffIndicator = QLatin1String("+++ ");
    for (QTextBlock  block = inBlock; block.isValid() ; block = block.previous()) {
        QString diffFileName = block.text();
        if (diffFileName.startsWith(diffIndicator)) {
            diffFileName.remove(0, diffIndicator.size());
            const int tabIndex = diffFileName.indexOf(QLatin1Char('\t'));
            if (tabIndex != -1)
                diffFileName.truncate(tabIndex);
            return findDiffFile(diffFileName);
        }
    }
    return QString();
}

QStringList CVSEditor::annotationPreviousVersions(const QString &revision) const
{
    if (isFirstRevision(revision))
        return QStringList();
    return QStringList(previousRevision(revision));
}

}
}
