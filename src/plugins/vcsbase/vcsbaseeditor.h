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

#ifndef VCSBASE_BASEEDITOR_H
#define VCSBASE_BASEEDITOR_H

#include "vcsbase_global.h"

#include <texteditor/basetexteditor.h>

#include <QtCore/QSet>

QT_BEGIN_NAMESPACE
class QAction;
class QTextCodec;
class QTextCursor;
QT_END_NAMESPACE

namespace Core {
    class IVersionControl;
}

namespace VCSBase {

struct VCSBaseEditorWidgetPrivate;
class DiffHighlighter;
class BaseAnnotationHighlighter;

// Documentation inside
enum EditorContentType {
    RegularCommandOutput,
    LogOutput,
    AnnotateOutput,
    DiffOutput
};

struct VCSBASE_EXPORT VCSBaseEditorParameters
{
    EditorContentType type;
    const char *id;
    const char *displayName;
    const char *context;
    const char *mimeType;
    const char *extension;
};

class VCSBASE_EXPORT DiffChunk
{
public:
    bool isValid() const;
    QByteArray asPatch() const;

    QString fileName;
    QByteArray chunk;
};

class VCSBASE_EXPORT VCSBaseEditorWidget : public TextEditor::BaseTextEditorWidget
{
    Q_PROPERTY(QString source READ source WRITE setSource)
    Q_PROPERTY(QString diffBaseDirectory READ diffBaseDirectory WRITE setDiffBaseDirectory)
    Q_PROPERTY(QTextCodec *codec READ codec WRITE setCodec)
    Q_PROPERTY(QString annotateRevisionTextFormat READ annotateRevisionTextFormat WRITE setAnnotateRevisionTextFormat)
    Q_PROPERTY(QString copyRevisionTextFormat READ copyRevisionTextFormat WRITE setCopyRevisionTextFormat)
    Q_PROPERTY(bool isFileLogAnnotateEnabled READ isFileLogAnnotateEnabled WRITE setFileLogAnnotateEnabled)
    Q_PROPERTY(bool revertDiffChunkEnabled READ isRevertDiffChunkEnabled WRITE setRevertDiffChunkEnabled)
    Q_OBJECT

protected:
    // Initialization requires calling init() (which in turns calls
    // virtual functions).
    explicit VCSBaseEditorWidget(const VCSBaseEditorParameters *type,
                           QWidget *parent);
public:
    void init();

    virtual ~VCSBaseEditorWidget();

    /* Force read-only: Make it a read-only, temporary file.
     * Should be set to true by version control views. It is not on
     * by default since it should not  trigger when patches are opened as
     * files. */
    void setForceReadOnly(bool b);
    bool isForceReadOnly() const;

    QString source() const;
    void setSource(const  QString &source);

    // Format for "Annotate" revision menu entries. Should contain '%1" placeholder
    QString annotateRevisionTextFormat() const;
    void setAnnotateRevisionTextFormat(const QString &);

    // Format for "Annotate Previous" revision menu entries. Should contain '%1" placeholder.
    // Defaults to "annotateRevisionTextFormat" if unset.
    QString annotatePreviousRevisionTextFormat() const;
    void setAnnotatePreviousRevisionTextFormat(const QString &);

    // Format for "Copy" revision menu entries. Should contain '%1" placeholder
    QString copyRevisionTextFormat() const;
    void setCopyRevisionTextFormat(const QString &);

    // Enable "Annotate" context menu in file log view
    // (set to true if the source is a single file and the VCS implements it)
    bool isFileLogAnnotateEnabled() const;
    void setFileLogAnnotateEnabled(bool e);

    QTextCodec *codec() const;
    void setCodec(QTextCodec *);

    // Base directory for diff views
    QString diffBaseDirectory() const;
    void setDiffBaseDirectory(const QString &d);

    // Diff: Can revert?
    bool isRevertDiffChunkEnabled() const;
    void setRevertDiffChunkEnabled(bool e);

    bool isModified() const;

    EditorContentType contentType() const;

    // Utility to find a parameter set by type in an array.
    static  const VCSBaseEditorParameters *
        findType(const VCSBaseEditorParameters *array, int arraySize, EditorContentType et);

    // Utility to find the codec for a source (file or directory), querying
    // the editor manager and the project managers (defaults to system codec).
    // The codec should be set on editors displaying diff or annotation
    // output.
    static QTextCodec *getCodec(const QString &source);
    static QTextCodec *getCodec(const QString &workingDirectory, const QStringList &files);

    // Utility to return the widget from the IEditor returned by the editor
    // manager which is a BaseTextEditor.
    static VCSBaseEditorWidget *getVcsBaseEditor(const Core::IEditor *editor);

    // Utility to find the line number of the current editor. Optionally,
    // pass in the file name to match it. To be used when jumping to current
    // line number in a 'annnotate current file' slot, which checks if the
    // current file originates from the current editor or the project selection.
    static int lineNumberOfCurrentEditor(const QString &currentFile = QString());

    //Helper to go to line of editor if it is a text editor
    static bool gotoLineOfEditor(Core::IEditor *e, int lineNumber);

    // Convenience functions to determine the source to pass on to a diff
    // editor if one has a call consisting of working directory and file arguments.
    // ('git diff XX' -> 'XX' , 'git diff XX file' -> 'XX/file').
    static QString getSource(const QString &workingDirectory, const QString &fileName);
    static QString getSource(const QString &workingDirectory, const QStringList &fileNames);
    // Convenience functions to determine an title/id to identify the editor
    // from the arguments (','-joined arguments or directory) + revision.
    static QString getTitleId(const QString &workingDirectory,
                              const QStringList &fileNames,
                              const QString &revision = QString());

    bool setConfigurationWidget(QWidget *w);
    QWidget *configurationWidget() const;

    /* Tagging editors: Sometimes, an editor should be re-used, for example, when showing
     * a diff of the same file with different diff-options. In order to be able to find
     * the editor, they get a 'tag' containing type and parameters (dynamic property string). */
    static void tagEditor(Core::IEditor *e, const QString &tag);
    static Core::IEditor* locateEditorByTag(const QString &tag);
    static QString editorTag(EditorContentType t, const QString &workingDirectory, const QStringList &files,
                             const QString &revision = QString());

signals:
    // These signals also exist in the opaque editable (IEditor) that is
    // handled by the editor manager for convenience. They are emitted
    // for LogOutput/AnnotateOutput content types.
    void describeRequested(const QString &source, const QString &change);
    void annotateRevisionRequested(const QString &source, const QString &change, int lineNumber);
    void diffChunkApplied(const VCSBase::DiffChunk &dc);
    void diffChunkReverted(const VCSBase::DiffChunk &dc);

public slots:
    // Convenience slot to set data read from stdout, will use the
    // documents' codec to decode
    void setPlainTextData(const QByteArray &data);

protected:
    virtual TextEditor::BaseTextEditor *createEditor();

    void contextMenuEvent(QContextMenuEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *);

public slots:
    void setFontSettings(const TextEditor::FontSettings &);

private slots:
    void describe();
    void slotActivateAnnotation();
    void slotPopulateDiffBrowser();
    void slotDiffBrowse(int);
    void slotDiffCursorPositionChanged();
    void slotAnnotateRevision();
    void slotCopyRevision();
    void slotApplyDiffChunk();
    void slotPaste();

protected:
    /* A helper that can be used to locate a file in a diff in case it
     * is relative. Tries to derive the directory from base directory,
     * source and version control. */
    QString findDiffFile(const QString &f, Core::IVersionControl *control = 0) const;

    virtual bool canApplyDiffChunk(const DiffChunk &dc) const;
    // Revert a patch chunk. Default implementation uses patch.exe
    virtual bool applyDiffChunk(const DiffChunk &dc, bool revert = false) const;

private:
    // Implement to return a set of change identifiers in
    // annotation mode
    virtual QSet<QString> annotationChanges() const = 0;
    // Implement to identify a change number at the cursor position
    virtual QString changeUnderCursor(const QTextCursor &) const = 0;
    // Factory functions for highlighters
    virtual DiffHighlighter *createDiffHighlighter() const = 0;
    virtual BaseAnnotationHighlighter *createAnnotationHighlighter(const QSet<QString> &changes) const = 0;
    // Implement to return a local file name from the diff file specification
    // (text cursor at position above change hunk)
    virtual QString fileNameFromDiffSpecification(const QTextBlock &diffFileSpec) const = 0;
    // Implement to return the previous version[s] of an annotation change
    // for "Annotate previous version"
    virtual QStringList annotationPreviousVersions(const QString &revision) const;
    // cut out chunk and determine file name.
    DiffChunk diffChunk(QTextCursor cursor) const;

    void jumpToChangeFromDiff(QTextCursor cursor);
    QAction *createDescribeAction(const QString &change);
    QAction *createAnnotateAction(const QString &change, bool previous = false);
    QAction *createCopyRevisionAction(const QString &change);

    VCSBaseEditorWidgetPrivate *d;
};

} // namespace VCSBase

Q_DECLARE_METATYPE(VCSBase::DiffChunk)

#endif // VCSBASE_BASEEDITOR_H
