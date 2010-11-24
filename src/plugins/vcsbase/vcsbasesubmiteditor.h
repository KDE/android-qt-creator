/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef VCSBASE_SUBMITEDITOR_H
#define VCSBASE_SUBMITEDITOR_H

#include "vcsbase_global.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QtGui/QAbstractItemView>

QT_BEGIN_NAMESPACE
class QIcon;
class QAbstractItemModel;
class QAction;
QT_END_NAMESPACE

namespace Utils {
    class SubmitEditorWidget;
}

namespace VCSBase {
namespace Internal {
    struct CommonVcsSettings;
}
struct VCSBaseSubmitEditorPrivate;

/* Utility struct to parametrize a VCSBaseSubmitEditor. */
struct VCSBASE_EXPORT VCSBaseSubmitEditorParameters {
    const char *mimeType;
    const char *id;
    const char *displayName;
    const char *context;
};

/* Base class for a submit editor based on the Utils::SubmitEditorWidget
 * that presents the commit message in a text editor and an
 * checkable list of modified files in a list window. The user can delete
 * files from the list by pressing unchecking them or diff the selection
 * by doubleclicking.
 *
 * The action matching the the ids (unless 0) of the parameter struct will be
 * registered with the EditorWidget and submit/diff actions will be added to
 * a toolbar.
 *
 * For the given context, there must be only one instance of the editor
 * active.
 * To start a submit, set the submit template on the editor and the output
 * of the VCS status command listing the modified files as fileList and open
 * it.
 * The submit process is started by listening on the editor close
 * signal and then asking the IFile interface of the editor to save the file
 * within a IFileManager::blockFileChange() section
 * and to launch the submit process. In addition, the action registered
 * for submit should be connected to a slot triggering the close of the
 * current editor in the editor manager. */

class VCSBASE_EXPORT VCSBaseSubmitEditor : public Core::IEditor
{
    Q_OBJECT
    Q_PROPERTY(int fileNameColumn READ fileNameColumn WRITE setFileNameColumn DESIGNABLE false)
    Q_PROPERTY(QAbstractItemView::SelectionMode fileListSelectionMode READ fileListSelectionMode WRITE setFileListSelectionMode DESIGNABLE true)
    Q_PROPERTY(bool lineWrap READ lineWrap WRITE setLineWrap DESIGNABLE true)
    Q_PROPERTY(int lineWrapWidth READ lineWrapWidth WRITE setLineWrapWidth DESIGNABLE true)
    Q_PROPERTY(QString checkScriptWorkingDirectory READ checkScriptWorkingDirectory WRITE setCheckScriptWorkingDirectory DESIGNABLE true)
    Q_PROPERTY(bool emptyFileListEnabled READ isEmptyFileListEnabled WRITE setEmptyFileListEnabled DESIGNABLE true)

protected:
    explicit VCSBaseSubmitEditor(const VCSBaseSubmitEditorParameters *parameters,
                                 Utils::SubmitEditorWidget *editorWidget);

public:
    // Register the actions with the submit editor widget.
    void registerActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                         QAction *submitAction = 0, QAction *diffAction = 0);
    void unregisterActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                           QAction *submitAction = 0, QAction *diffAction = 0);

    virtual ~VCSBaseSubmitEditor();

    // A utility routine to be called when closing a submit editor.
    // Runs checks on the message and prompts according to configuration.
    // Force prompt should be true if it is invoked by closing an editor
    // as opposed to invoking the "Submit" button.
    // 'promptSetting' points to a bool variable containing the plugin's
    // prompt setting. The user can uncheck it from the message box.
    enum PromptSubmitResult { SubmitConfirmed, SubmitCanceled, SubmitDiscarded };
    PromptSubmitResult promptSubmit(const QString &title, const QString &question,
                                    const QString &checkFailureQuestion,
                                    bool *promptSetting,
                                    bool forcePrompt = false,
                                    bool canCommitOnFailure = true) const;

    int fileNameColumn() const;
    void setFileNameColumn(int c);

    QAbstractItemView::SelectionMode fileListSelectionMode() const;
    void setFileListSelectionMode(QAbstractItemView::SelectionMode sm);

    // 'Commit' action enabled despite empty file list
    bool isEmptyFileListEnabled() const;
    void setEmptyFileListEnabled(bool e);

    bool lineWrap() const;
    void setLineWrap(bool);

    int lineWrapWidth() const;
    void setLineWrapWidth(int);

    QString checkScriptWorkingDirectory() const;
    void setCheckScriptWorkingDirectory(const QString &);

    // Core::IEditor
    virtual bool createNew(const QString &contents);
    virtual bool open(const QString &fileName);
    virtual Core::IFile *file();
    virtual QString displayName() const;
    virtual void setDisplayName(const QString &title);
    virtual bool duplicateSupported() const;
    virtual Core::IEditor *duplicate(QWidget * parent);
    virtual QString id() const;

    virtual QWidget *toolBar();
    virtual Core::Context context() const;
    virtual QWidget *widget();

    virtual QByteArray saveState() const;
    virtual bool restoreState(const QByteArray &state);

    QStringList checkedFiles() const;

    void setFileModel(QAbstractItemModel *m);
    QAbstractItemModel *fileModel() const;

    // Utilities returning some predefined icons for actions
    static QIcon diffIcon();
    static QIcon submitIcon();

    // Utility returning all project files in case submit lists need to
    // be restricted to them
    static QStringList currentProjectFiles(bool nativeSeparators, QString *name = 0);

    // Reduce a list of untracked files reported by a VCS down to the files
    // that are actually part of the current project(s).
    static void filterUntrackedFilesOfProject(const QString &repositoryDirectory, QStringList *untrackedFiles);

    virtual bool isTemporary() const { return true; }

    // Helper to raise an already open submit editor to prevent opening twice.
    static bool raiseSubmitEditor();

signals:
    void diffSelectedFiles(const QStringList &files);

private slots:
    void slotDiffSelectedVCSFiles(const QStringList &rawList);
    bool save(const QString &fileName);
    void slotDescriptionChanged();
    void slotCheckSubmitMessage();
    void slotInsertNickName();
    void slotSetFieldNickName(int);
    void slotUpdateEditorSettings(const VCSBase::Internal::CommonVcsSettings &);

protected:
    /* These hooks allow for modifying the contents that goes to
     * the file. The default implementation uses the text
     * of the description editor. */
    virtual QString fileContents() const;
    virtual bool setFileContents(const QString &contents);

private:
    void createUserFields(const QString &fieldConfigFile);
    bool checkSubmitMessage(QString *errorMessage) const;
    bool runSubmitMessageCheckScript(const QString &script, QString *errorMessage) const;
    QString promptForNickName();

    VCSBaseSubmitEditorPrivate *m_d;
};

} // namespace VCSBase

#endif // VCSBASE_SUBMITEDITOR_H
