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

namespace Utils { class SubmitEditorWidget; }

namespace VcsBase {
namespace Internal {
    class CommonVcsSettings;
}
struct VcsBaseSubmitEditorPrivate;

class VCSBASE_EXPORT VcsBaseSubmitEditorParameters
{
public:
    const char *mimeType;
    const char *id;
    const char *displayName;
    const char *context;
};

class VCSBASE_EXPORT VcsBaseSubmitEditor : public Core::IEditor
{
    Q_OBJECT
    Q_PROPERTY(int fileNameColumn READ fileNameColumn WRITE setFileNameColumn DESIGNABLE false)
    Q_PROPERTY(QAbstractItemView::SelectionMode fileListSelectionMode READ fileListSelectionMode WRITE setFileListSelectionMode DESIGNABLE true)
    Q_PROPERTY(bool lineWrap READ lineWrap WRITE setLineWrap DESIGNABLE true)
    Q_PROPERTY(int lineWrapWidth READ lineWrapWidth WRITE setLineWrapWidth DESIGNABLE true)
    Q_PROPERTY(QString checkScriptWorkingDirectory READ checkScriptWorkingDirectory WRITE setCheckScriptWorkingDirectory DESIGNABLE true)
    Q_PROPERTY(bool emptyFileListEnabled READ isEmptyFileListEnabled WRITE setEmptyFileListEnabled DESIGNABLE true)

protected:
    explicit VcsBaseSubmitEditor(const VcsBaseSubmitEditorParameters *parameters,
                                 Utils::SubmitEditorWidget *editorWidget);

public:
    // Register the actions with the submit editor widget.
    void registerActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                         QAction *submitAction = 0, QAction *diffAction = 0);
    void unregisterActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                           QAction *submitAction = 0, QAction *diffAction = 0);

    ~VcsBaseSubmitEditor();

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
    bool createNew(const QString &contents);
    bool open(QString *errorString, const QString &fileName, const QString &realFileName);
    Core::IFile *file();
    QString displayName() const;
    void setDisplayName(const QString &title);
    bool duplicateSupported() const;
    Core::IEditor *duplicate(QWidget *parent);
    Core::Id id() const;
    bool isTemporary() const { return true; }

    QWidget *toolBar();

    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);

    QStringList checkedFiles() const;

    void setFileModel(QAbstractItemModel *m, const QString &repositoryDirectory = QString());
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

    // Helper to raise an already open submit editor to prevent opening twice.
    static bool raiseSubmitEditor();

signals:
    void diffSelectedFiles(const QStringList &files);

private slots:
    void slotDiffSelectedVcsFiles(const QStringList &rawList);
    bool save(QString *errorString, const QString &fileName, bool autoSave);
    void slotDescriptionChanged();
    void slotCheckSubmitMessage();
    void slotInsertNickName();
    void slotSetFieldNickName(int);
    void slotUpdateEditorSettings(const VcsBase::Internal::CommonVcsSettings &);

protected:
    /* These hooks allow for modifying the contents that goes to
     * the file. The default implementation uses the text
     * of the description editor. */
    virtual QByteArray fileContents() const;
    virtual bool setFileContents(const QString &contents);

private:
    void createUserFields(const QString &fieldConfigFile);
    bool checkSubmitMessage(QString *errorMessage) const;
    bool runSubmitMessageCheckScript(const QString &script, QString *errorMessage) const;
    QString promptForNickName();

    VcsBaseSubmitEditorPrivate *d;
};

} // namespace VcsBase

#endif // VCSBASE_SUBMITEDITOR_H
