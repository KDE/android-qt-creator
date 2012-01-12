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

#include "gitclient.h"
#include "gitutils.h"

#include "commitdata.h"
#include "gitconstants.h"
#include "gitplugin.h"
#include "gitsubmiteditor.h"
#include "gitversioncontrol.h"

#include <vcsbase/submitfilemodel.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/id.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/iversioncontrol.h>

#include <texteditor/itexteditor.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <vcsbase/command.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorparameterwidget.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <vcsbase/vcsbaseplugin.h>

#include <QtCore/QRegExp>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTime>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QSignalMapper>

#include <QtGui/QComboBox>
#include <QtGui/QMainWindow> // for msg box parent
#include <QtGui/QMessageBox>
#include <QtGui/QToolButton>
#include <QtCore/QTextCodec>

static const char GIT_DIRECTORY[] = ".git";

namespace Git {
namespace Internal {

class BaseGitDiffArgumentsWidget : public VcsBase::VcsBaseEditorParameterWidget
{
    Q_OBJECT

public:
    BaseGitDiffArgumentsWidget(GitClient *client, const QString &directory,
                               const QStringList &args) :
        m_workingDirectory(directory),
        m_client(client)
    {
        QTC_ASSERT(!directory.isEmpty(), return);
        QTC_ASSERT(m_client, return);

        m_patienceButton = addToggleButton(QLatin1String("--patience"), tr("Patience"),
                                           tr("Use the patience algorithm for calculating the differences."));
        mapSetting(m_patienceButton, client->settings()->boolPointer(GitSettings::diffPatienceKey));
        m_ignoreWSButton = addToggleButton("--ignore-space-change", tr("Ignore Whitespace"),
                                           tr("Ignore whitespace only changes."));
        mapSetting(m_ignoreWSButton, m_client->settings()->boolPointer(GitSettings::ignoreSpaceChangesInDiffKey));

        setBaseArguments(args);
    }

protected:
    QString m_workingDirectory;
    GitClient *m_client;
    QToolButton *m_patienceButton;
    QToolButton *m_ignoreWSButton;
};

class GitCommitDiffArgumentsWidget : public BaseGitDiffArgumentsWidget
{
    Q_OBJECT

public:
    GitCommitDiffArgumentsWidget(Git::Internal::GitClient *client, const QString &directory,
                                 const QStringList &args, const QStringList &unstaged,
                                 const QStringList &staged) :
        BaseGitDiffArgumentsWidget(client, directory, args),
        m_unstagedFileNames(unstaged),
        m_stagedFileNames(staged)
    { }

    void executeCommand()
    {
        m_client->diff(m_workingDirectory, arguments(), m_unstagedFileNames, m_stagedFileNames);
    }

private:
    const QStringList m_unstagedFileNames;
    const QStringList m_stagedFileNames;
};

class GitFileDiffArgumentsWidget : public BaseGitDiffArgumentsWidget
{
    Q_OBJECT
public:
    GitFileDiffArgumentsWidget(Git::Internal::GitClient *client, const QString &directory,
                               const QStringList &args, const QString &file) :
        BaseGitDiffArgumentsWidget(client, directory, args),
        m_fileName(file)
    { }

    void executeCommand()
    {
        m_client->diff(m_workingDirectory, arguments(), m_fileName);
    }

private:
    const QString m_fileName;
};

class GitBranchDiffArgumentsWidget : public BaseGitDiffArgumentsWidget
{
    Q_OBJECT
public:
    GitBranchDiffArgumentsWidget(Git::Internal::GitClient *client, const QString &directory,
                                 const QStringList &args, const QString &branch) :
        BaseGitDiffArgumentsWidget(client, directory, args),
        m_branchName(branch)
    { }

    void executeCommand()
    {
        m_client->diffBranch(m_workingDirectory, arguments(), m_branchName);
    }

private:
    const QString m_branchName;
};

class GitShowArgumentsWidget : public BaseGitDiffArgumentsWidget
{
    Q_OBJECT

public:
    GitShowArgumentsWidget(Git::Internal::GitClient *client,
                           const QString &directory,
                           const QStringList &args,
                           const QString &id) :
        BaseGitDiffArgumentsWidget(client, directory, args),
        m_client(client),
        m_workingDirectory(directory),
        m_id(id)
    {
        QList<ComboBoxItem> prettyChoices;
        prettyChoices << ComboBoxItem(tr("oneline"), QLatin1String("oneline"))
                      << ComboBoxItem(tr("short"), QLatin1String("short"))
                      << ComboBoxItem(tr("medium"), QLatin1String("medium"))
                      << ComboBoxItem(tr("full"), QLatin1String("full"))
                      << ComboBoxItem(tr("fuller"), QLatin1String("fuller"))
                      << ComboBoxItem(tr("email"), QLatin1String("email"))
                      << ComboBoxItem(tr("raw"), QLatin1String("raw"));
        mapSetting(addComboBox(QLatin1String("--pretty"), prettyChoices),
                   m_client->settings()->intPointer(GitSettings::showPrettyFormatKey));
    }

    void executeCommand()
    {
        m_client->show(m_workingDirectory, m_id, arguments());
    }

private:
    GitClient *m_client;
    QString m_workingDirectory;
    QString m_id;
};

class GitBlameArgumentsWidget : public VcsBase::VcsBaseEditorParameterWidget
{
    Q_OBJECT

public:
    GitBlameArgumentsWidget(Git::Internal::GitClient *client,
                            const QString &directory,
                            const QStringList &args,
                            const QString &revision, const QString &fileName) :
        m_editor(0),
        m_client(client),
        m_workingDirectory(directory),
        m_revision(revision),
        m_fileName(fileName)
    {
        mapSetting(addToggleButton(QString(), tr("Omit Date"),
                                   tr("Hide the date of a change from the output.")),
                   m_client->settings()->boolPointer(GitSettings::omitAnnotationDateKey));
        mapSetting(addToggleButton(QString("-w"), tr("Ignore Whitespace"),
                                   tr("Ignore whitespace only changes.")),
                   m_client->settings()->boolPointer(GitSettings::ignoreSpaceChangesInBlameKey));

        setBaseArguments(args);
    }

    void setEditor(VcsBase::VcsBaseEditorWidget *editor)
    {
        Q_ASSERT(editor);
        m_editor = editor;
    }

    void executeCommand()
    {
        int line = -1;
        if (m_editor)
            line = m_editor->lineNumberOfCurrentEditor();
        m_client->blame(m_workingDirectory, arguments(), m_fileName, m_revision, line);
    }

private:
    VcsBase::VcsBaseEditorWidget *m_editor;
    GitClient *m_client;
    QString m_workingDirectory;
    QString m_revision;
    QString m_fileName;
};

class GitLogArgumentsWidget : public BaseGitDiffArgumentsWidget
{
    Q_OBJECT

public:
    GitLogArgumentsWidget(Git::Internal::GitClient *client,
                          const QString &directory,
                          bool enableAnnotationContextMenu,
                          const QStringList &args,
                          const QStringList &fileNames) :
        BaseGitDiffArgumentsWidget(client, directory, args),
        m_client(client),
        m_workingDirectory(directory),
        m_enableAnnotationContextMenu(enableAnnotationContextMenu),
        m_fileNames(fileNames)
    {
        QToolButton *button = addToggleButton(QLatin1String("--patch"), tr("Show Diff"),
                                              tr("Show difference."));
        mapSetting(button, m_client->settings()->boolPointer(GitSettings::logDiffKey));
        connect(button, SIGNAL(toggled(bool)), m_patienceButton, SLOT(setEnabled(bool)));
        connect(button, SIGNAL(toggled(bool)), m_ignoreWSButton, SLOT(setEnabled(bool)));
        m_patienceButton->setEnabled(button->isChecked());
        m_ignoreWSButton->setEnabled(button->isChecked());
    }

    void executeCommand()
    {
        m_client->log(m_workingDirectory, m_fileNames, m_enableAnnotationContextMenu, arguments());
    }

private:
    GitClient *m_client;
    QString m_workingDirectory;
    bool m_enableAnnotationContextMenu;
    QStringList m_fileNames;
};

inline Core::IEditor* locateEditor(const Core::ICore *core, const char *property, const QString &entry)
{
    foreach (Core::IEditor *ed, core->editorManager()->openedEditors())
        if (ed->file()->property(property).toString() == entry)
            return ed;
    return 0;
}

// Return converted command output, remove '\r' read on Windows
static inline QString commandOutputFromLocal8Bit(const QByteArray &a)
{
    QString output = QString::fromLocal8Bit(a);
    output.remove(QLatin1Char('\r'));
    return output;
}

// Return converted command output split into lines
static inline QStringList commandOutputLinesFromLocal8Bit(const QByteArray &a)
{
    QString output = commandOutputFromLocal8Bit(a);
    const QChar newLine = QLatin1Char('\n');
    if (output.endsWith(newLine))
        output.truncate(output.size() - 1);
    if (output.isEmpty())
        return QStringList();
    return output.split(newLine);
}

static inline VcsBase::VcsBaseOutputWindow *outputWindow()
{
    return VcsBase::VcsBaseOutputWindow::instance();
}

static inline QString msgRepositoryNotFound(const QString &dir)
{
    return GitClient::tr("Cannot determine the repository for \"%1\".").arg(dir);
}

static inline QString msgParseFilesFailed()
{
    return  GitClient::tr("Cannot parse the file output.");
}

// ---------------- GitClient

const char *GitClient::stashNamePrefix = "stash@{";

GitClient::GitClient(GitSettings *settings) :
    m_cachedGitVersion(0),
    m_msgWait(tr("Waiting for data...")),
    m_core(Core::ICore::instance()),
    m_repositoryChangedSignalMapper(0),
    m_settings(settings)
{
    Q_ASSERT(settings);
    connect(m_core, SIGNAL(saveSettingsRequested()), this, SLOT(saveSettings()));
}

GitClient::~GitClient()
{
}

const char *GitClient::noColorOption = "--no-color";
const char *GitClient::decorateOption = "--decorate";

QString GitClient::findRepositoryForDirectory(const QString &dir)
{
    if (synchronousGitVersion(true) >= 0x010700) {
        QByteArray outputText;
        QStringList arguments;
        arguments << QLatin1String("rev-parse") << QLatin1String("--show-toplevel");
        fullySynchronousGit(dir, arguments, &outputText, 0, false);
        return outputText.trimmed();
    } else {
        // Check for ".git/config"
        const QString checkFile = QLatin1String(GIT_DIRECTORY) + QLatin1String("/config");
        return VcsBase::VcsBasePlugin::findRepositoryForDirectory(dir, checkFile);
    }
}

VcsBase::VcsBaseEditorWidget *GitClient::findExistingVCSEditor(const char *registerDynamicProperty,
                                                               const QString &dynamicPropertyValue) const
{
    VcsBase::VcsBaseEditorWidget *rc = 0;
    Core::IEditor *outputEditor = locateEditor(m_core, registerDynamicProperty, dynamicPropertyValue);
    if (!outputEditor)
        return 0;

    // Exists already
    Core::EditorManager::instance()->activateEditor(outputEditor, Core::EditorManager::ModeSwitch);
    outputEditor->createNew(m_msgWait);
    rc = VcsBase::VcsBaseEditorWidget::getVcsBaseEditor(outputEditor);

    return rc;
}

/* Create an editor associated to VCS output of a source file/directory
 * (using the file's codec). Makes use of a dynamic property to find an
 * existing instance and to reuse it (in case, say, 'git diff foo' is
 * already open). */
VcsBase::VcsBaseEditorWidget *GitClient::createVcsEditor(const Core::Id &id,
                                                         QString title,
                                                         // Source file or directory
                                                         const QString &source,
                                                         CodecType codecType,
                                                         // Dynamic property and value to identify that editor
                                                         const char *registerDynamicProperty,
                                                         const QString &dynamicPropertyValue,
                                                         QWidget *configWidget) const
{
    VcsBase::VcsBaseEditorWidget *rc = 0;
    Q_ASSERT(!findExistingVCSEditor(registerDynamicProperty, dynamicPropertyValue));

    // Create new, set wait message, set up with source and codec
    Core::IEditor *outputEditor = m_core->editorManager()->openEditorWithContents(id, &title, m_msgWait);
    outputEditor->file()->setProperty(registerDynamicProperty, dynamicPropertyValue);
    rc = VcsBase::VcsBaseEditorWidget::getVcsBaseEditor(outputEditor);
    connect(rc, SIGNAL(annotateRevisionRequested(QString,QString,int)),
            this, SLOT(slotBlameRevisionRequested(QString,QString,int)));
    QTC_ASSERT(rc, return 0);
    rc->setSource(source);
    if (codecType == CodecSource) {
        rc->setCodec(getSourceCodec(source));
    } else if (codecType == CodecLogOutput) {
        QString encodingName = readConfigValue(source, QLatin1String("i18n.logOutputEncoding"));
        if (encodingName.isEmpty())
            encodingName = QLatin1String("utf-8");
        rc->setCodec(QTextCodec::codecForName(encodingName.toLocal8Bit()));
    }

    rc->setForceReadOnly(true);
    m_core->editorManager()->activateEditor(outputEditor, Core::EditorManager::ModeSwitch);

    if (configWidget)
        rc->setConfigurationWidget(configWidget);

    return rc;
}

void GitClient::diff(const QString &workingDirectory,
                     const QStringList &diffArgs,
                     const QStringList &unstagedFileNames,
                     const QStringList &stagedFileNames)
{
    const QString binary = settings()->stringValue(GitSettings::binaryPathKey);
    const Core::Id editorId = Git::Constants::GIT_DIFF_EDITOR_ID;
    const QString title = tr("Git Diff");

    VcsBase::VcsBaseEditorWidget *editor = findExistingVCSEditor("originalFileName", workingDirectory);
    if (!editor) {
        GitCommitDiffArgumentsWidget *argWidget =
                new GitCommitDiffArgumentsWidget(this, workingDirectory, diffArgs,
                                                 unstagedFileNames, stagedFileNames);

        editor = createVcsEditor(editorId, title,
                                 workingDirectory, CodecSource, "originalFileName", workingDirectory, argWidget);
        connect(editor, SIGNAL(diffChunkReverted(VcsBase::DiffChunk)), argWidget, SLOT(executeCommand()));
        editor->setRevertDiffChunkEnabled(true);
    }

    GitCommitDiffArgumentsWidget *argWidget = qobject_cast<GitCommitDiffArgumentsWidget *>(editor->configurationWidget());
    QStringList userDiffArgs = argWidget->arguments();
    editor->setDiffBaseDirectory(workingDirectory);

    // Create a batch of 2 commands to be run after each other in case
    // we have a mixture of staged/unstaged files as is the case
    // when using the submit dialog.
    VcsBase::Command *command = createCommand(workingDirectory, editor);
    // Directory diff?

    QStringList cmdArgs;
    cmdArgs << QLatin1String("diff") << QLatin1String(noColorOption);

    int timeout = settings()->intValue(GitSettings::timeoutKey);

    if (unstagedFileNames.empty() && stagedFileNames.empty()) {
       QStringList arguments(cmdArgs);
       arguments << userDiffArgs;
       outputWindow()->appendCommand(workingDirectory, binary, arguments);
       command->addJob(arguments, timeout);
    } else {
        // Files diff.
        if (!unstagedFileNames.empty()) {
           QStringList arguments(cmdArgs);
           arguments << userDiffArgs;
           arguments << QLatin1String("--") << unstagedFileNames;
           outputWindow()->appendCommand(workingDirectory, binary, arguments);
           command->addJob(arguments, timeout);
        }
        if (!stagedFileNames.empty()) {
           QStringList arguments(cmdArgs);
           arguments << userDiffArgs;
           arguments << QLatin1String("--cached") << diffArgs << QLatin1String("--") << stagedFileNames;
           outputWindow()->appendCommand(workingDirectory, binary, arguments);
           command->addJob(arguments, timeout);
        }
    }
    command->execute();
}

void GitClient::diff(const QString &workingDirectory,
                     const QStringList &diffArgs,
                     const QString &fileName)
{
    const Core::Id editorId = Git::Constants::GIT_DIFF_EDITOR_ID;
    const QString title = tr("Git Diff \"%1\"").arg(fileName);
    const QString sourceFile = VcsBase::VcsBaseEditorWidget::getSource(workingDirectory, fileName);

    VcsBase::VcsBaseEditorWidget *editor = findExistingVCSEditor("originalFileName", sourceFile);
    if (!editor) {
        GitFileDiffArgumentsWidget *argWidget =
                new GitFileDiffArgumentsWidget(this, workingDirectory, diffArgs, fileName);

        editor = createVcsEditor(editorId, title, sourceFile, CodecSource, "originalFileName", sourceFile, argWidget);
        connect(editor, SIGNAL(diffChunkReverted(VcsBase::DiffChunk)), argWidget, SLOT(executeCommand()));
        editor->setRevertDiffChunkEnabled(true);
    }

    GitFileDiffArgumentsWidget *argWidget = qobject_cast<GitFileDiffArgumentsWidget *>(editor->configurationWidget());
    QStringList userDiffArgs = argWidget->arguments();

    QStringList cmdArgs;
    cmdArgs << QLatin1String("diff") << QLatin1String(noColorOption)
              << userDiffArgs;

    if (!fileName.isEmpty())
        cmdArgs << QLatin1String("--") << fileName;
    executeGit(workingDirectory, cmdArgs, editor);
}

void GitClient::diffBranch(const QString &workingDirectory,
                           const QStringList &diffArgs,
                           const QString &branchName)
{
    const Core::Id editorId = Git::Constants::GIT_DIFF_EDITOR_ID;
    const QString title = tr("Git Diff Branch \"%1\"").arg(branchName);
    const QString sourceFile = VcsBase::VcsBaseEditorWidget::getSource(workingDirectory, QStringList());

    VcsBase::VcsBaseEditorWidget *editor = findExistingVCSEditor("BranchName", branchName);
    if (!editor)
        editor = createVcsEditor(editorId, title, sourceFile, CodecSource, "BranchName", branchName,
                                 new GitBranchDiffArgumentsWidget(this, workingDirectory,
                                                                  diffArgs, branchName));

    GitBranchDiffArgumentsWidget *argWidget = qobject_cast<GitBranchDiffArgumentsWidget *>(editor->configurationWidget());
    QStringList userDiffArgs = argWidget->arguments();

    QStringList cmdArgs;
    cmdArgs << QLatin1String("diff") << QLatin1String(noColorOption)
              << userDiffArgs  << branchName;

    executeGit(workingDirectory, cmdArgs, editor);
}

void GitClient::status(const QString &workingDirectory)
{
    // @TODO: Use "--no-color" once it is supported
    QStringList statusArgs(QLatin1String("status"));
    statusArgs << QLatin1String("-u");
    VcsBase::VcsBaseOutputWindow *outwin = outputWindow();
    outwin->setRepository(workingDirectory);
    VcsBase::Command *command = executeGit(workingDirectory, statusArgs, 0, true);
    connect(command, SIGNAL(finished(bool,int,QVariant)), outwin, SLOT(clearRepository()),
            Qt::QueuedConnection);
}

static const char graphLogFormatC[] = "%h %d %an %s %ci";

// Create a graphical log.
void GitClient::graphLog(const QString &workingDirectory, const QString & branch)
{
    QStringList arguments;
    arguments << QLatin1String("log") << QLatin1String(noColorOption);

    int logCount = settings()->intValue(GitSettings::logCountKey);
    if (logCount > 0)
         arguments << QLatin1String("-n") << QString::number(logCount);
    arguments << (QLatin1String("--pretty=format:") +  QLatin1String(graphLogFormatC))
              << QLatin1String("--topo-order") <<  QLatin1String("--graph");

    QString title;
    if (branch.isEmpty()) {
        title = tr("Git Log");
    } else {
        title = tr("Git Log \"%1\"").arg(branch);
        arguments << branch;
    }
    const Core::Id editorId = Git::Constants::GIT_LOG_EDITOR_ID;
    const QString sourceFile = VcsBase::VcsBaseEditorWidget::getSource(workingDirectory, QStringList());
    VcsBase::VcsBaseEditorWidget *editor = findExistingVCSEditor("logFileName", sourceFile);
    if (!editor)
        editor = createVcsEditor(editorId, title, sourceFile, CodecLogOutput, "logFileName", sourceFile, 0);
    executeGit(workingDirectory, arguments, editor);
}

void GitClient::log(const QString &workingDirectory, const QStringList &fileNames,
                    bool enableAnnotationContextMenu, const QStringList &args)
{
    const QString msgArg = fileNames.empty() ? workingDirectory :
                           fileNames.join(QString(", "));
    const QString title = tr("Git Log \"%1\"").arg(msgArg);
    const Core::Id editorId = Git::Constants::GIT_LOG_EDITOR_ID;
    const QString sourceFile = VcsBase::VcsBaseEditorWidget::getSource(workingDirectory, fileNames);
    VcsBase::VcsBaseEditorWidget *editor = findExistingVCSEditor("logFileName", sourceFile);
    if (!editor)
        editor = createVcsEditor(editorId, title, sourceFile, CodecLogOutput, "logFileName", sourceFile,
                                 new GitLogArgumentsWidget(this, workingDirectory,
                                                           enableAnnotationContextMenu,
                                                           args, fileNames));
    editor->setFileLogAnnotateEnabled(enableAnnotationContextMenu);

    QStringList arguments;
    arguments << QLatin1String("log") << QLatin1String(noColorOption)
              << QLatin1String(decorateOption);

    int logCount = settings()->intValue(GitSettings::logCountKey);
    if (logCount > 0)
         arguments << QLatin1String("-n") << QString::number(logCount);

    GitLogArgumentsWidget *argWidget = qobject_cast<GitLogArgumentsWidget *>(editor->configurationWidget());
    QStringList userArgs = argWidget->arguments();

    arguments.append(userArgs);

    if (!fileNames.isEmpty())
        arguments.append(fileNames);

    executeGit(workingDirectory, arguments, editor);
}

// Do not show "0000" or "^32ae4"
static inline bool canShow(const QString &sha)
{
    if (sha.startsWith(QLatin1Char('^')))
        return false;
    if (sha.count(QLatin1Char('0')) == sha.size())
        return false;
    return true;
}

static inline QString msgCannotShow(const QString &sha)
{
    return GitClient::tr("Cannot describe \"%1\".").arg(sha);
}

void GitClient::show(const QString &source, const QString &id, const QStringList &args)
{
    if (!canShow(id)) {
        outputWindow()->append(msgCannotShow(id));
        return;
    }

    const QString title = tr("Git Show \"%1\"").arg(id);
    const Core::Id editorId = Git::Constants::GIT_DIFF_EDITOR_ID;
    VcsBase::VcsBaseEditorWidget *editor = findExistingVCSEditor("show", id);
    if (!editor)
        editor = createVcsEditor(editorId, title, source, CodecSource, "show", id,
                                 new GitShowArgumentsWidget(this, source, args, id));

    GitShowArgumentsWidget *argWidget = qobject_cast<GitShowArgumentsWidget *>(editor->configurationWidget());
    QStringList userArgs = argWidget->arguments();

    QStringList arguments;
    arguments << QLatin1String("show") << QLatin1String(noColorOption);
    arguments << QLatin1String(decorateOption);
    arguments.append(userArgs);
    arguments << id;

    const QFileInfo sourceFi(source);
    const QString workDir = sourceFi.isDir() ? sourceFi.absoluteFilePath() : sourceFi.absolutePath();
    executeGit(workDir, arguments, editor);
}

void GitClient::saveSettings()
{
    settings()->writeSettings(m_core->settings());
}

void GitClient::slotBlameRevisionRequested(const QString &source, QString change, int lineNumber)
{
    // This might be invoked with a verbose revision description
    // "SHA1 author subject" from the annotation context menu. Strip the rest.
    const int blankPos = change.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        change.truncate(blankPos);
    const QFileInfo fi(source);
    blame(fi.absolutePath(), QStringList(), fi.fileName(), change, lineNumber);
}

QTextCodec *GitClient::getSourceCodec(const QString &file) const
{
    if (QFileInfo(file).isFile())
        return VcsBase::VcsBaseEditorWidget::getCodec(file);
    QString encodingName = readConfigValue(file, QLatin1String("gui.encoding"));
    if (encodingName.isEmpty())
        encodingName = QLatin1String("utf-8");
    return QTextCodec::codecForName(encodingName.toLocal8Bit());
}

void GitClient::blame(const QString &workingDirectory,
                      const QStringList &args,
                      const QString &fileName,
                      const QString &revision,
                      int lineNumber)
{
    const Core::Id editorId = Git::Constants::GIT_BLAME_EDITOR_ID;
    const QString id = VcsBase::VcsBaseEditorWidget::getTitleId(workingDirectory, QStringList(fileName), revision);
    const QString title = tr("Git Blame \"%1\"").arg(id);
    const QString sourceFile = VcsBase::VcsBaseEditorWidget::getSource(workingDirectory, fileName);

    VcsBase::VcsBaseEditorWidget *editor = findExistingVCSEditor("blameFileName", id);
    if (!editor) {
        GitBlameArgumentsWidget *argWidget =
                new GitBlameArgumentsWidget(this, workingDirectory, args,
                                            revision, fileName);
        editor = createVcsEditor(editorId, title, sourceFile, CodecSource, "blameFileName", id, argWidget);
        argWidget->setEditor(editor);
    }

    GitBlameArgumentsWidget *argWidget = qobject_cast<GitBlameArgumentsWidget *>(editor->configurationWidget());
    QStringList userBlameArgs = argWidget->arguments();

    QStringList arguments(QLatin1String("blame"));
    arguments << QLatin1String("--root");
    arguments.append(userBlameArgs);
    arguments << QLatin1String("--") << fileName;
    if (!revision.isEmpty())
        arguments << revision;
    executeGit(workingDirectory, arguments, editor, false, VcsBase::Command::NoReport, lineNumber);
}

void GitClient::checkoutBranch(const QString &workingDirectory, const QString &branch)
{
    QStringList arguments(QLatin1String("checkout"));
    arguments <<  branch;
    VcsBase::Command *cmd = executeGit(workingDirectory, arguments, 0, true);
    connectRepositoryChanged(workingDirectory, cmd);
}

bool GitClient::synchronousCheckoutBranch(const QString &workingDirectory,
                                          const QString &branch,
                                          QString *errorMessage /* = 0 */)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("checkout") << branch;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    const QString output = commandOutputFromLocal8Bit(outputText);
    outputWindow()->append(output);
    if (!rc) {
        const QString stdErr = commandOutputFromLocal8Bit(errorText);
        //: Meaning of the arguments: %1: Branch, %2: Repository, %3: Error message
        const QString msg = tr("Cannot checkout \"%1\" of \"%2\": %3").arg(branch, workingDirectory, stdErr);
        if (errorMessage) {
            *errorMessage = msg;
        } else {
            outputWindow()->appendError(msg);
        }
        return false;
    }
    return true;
}

void GitClient::checkout(const QString &workingDirectory, const QString &fileName)
{
    // Passing an empty argument as the file name is very dangereous, since this makes
    // git checkout apply to all files. Almost looks like a bug in git.
    if (fileName.isEmpty())
        return;

    QStringList arguments;
    arguments << QLatin1String("checkout") << QLatin1String("HEAD") << QLatin1String("--")
            << fileName;

    executeGit(workingDirectory, arguments, 0, true);
}

void GitClient::hardReset(const QString &workingDirectory, const QString &commit)
{
    QStringList arguments;
    arguments << QLatin1String("reset") << QLatin1String("--hard");
    if (!commit.isEmpty())
        arguments << commit;

    VcsBase::Command *cmd = executeGit(workingDirectory, arguments, 0, true);
    connectRepositoryChanged(workingDirectory, cmd);
}

void GitClient::addFile(const QString &workingDirectory, const QString &fileName)
{
    QStringList arguments;
    arguments << QLatin1String("add") << fileName;

    executeGit(workingDirectory, arguments, 0, true);
}

// Warning: 'intendToAdd' works only from 1.6.1 onwards
bool GitClient::synchronousAdd(const QString &workingDirectory,
                               bool intendToAdd,
                               const QStringList &files)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("add");
    if (intendToAdd)
        arguments << QLatin1String("--intent-to-add");
    arguments.append(files);
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString errorMessage = tr("Cannot add %n file(s) to \"%1\": %2", 0, files.size()).
                                     arg(QDir::toNativeSeparators(workingDirectory),
                                     commandOutputFromLocal8Bit(errorText));
        outputWindow()->appendError(errorMessage);
    }
    return rc;
}

bool GitClient::synchronousDelete(const QString &workingDirectory,
                                  bool force,
                                  const QStringList &files)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("rm");
    if (force)
        arguments << QLatin1String("--force");
    arguments.append(files);
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString errorMessage = tr("Cannot remove %n file(s) from \"%1\": %2", 0, files.size()).
                                     arg(QDir::toNativeSeparators(workingDirectory), commandOutputFromLocal8Bit(errorText));
        outputWindow()->appendError(errorMessage);
    }
    return rc;
}

bool GitClient::synchronousMove(const QString &workingDirectory,
                                const QString &from,
                                const QString &to)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("mv");
    arguments << (from);
    arguments << (to);
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString errorMessage = tr("Cannot move from \"%1\" to \"%2\": %3").
                                     arg(from, to, commandOutputFromLocal8Bit(errorText));
        outputWindow()->appendError(errorMessage);
    }
    return rc;
}

bool GitClient::synchronousReset(const QString &workingDirectory,
                                 const QStringList &files,
                                 QString *errorMessage)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("reset");
    if (files.isEmpty())
        arguments << QLatin1String("--hard");
    else
        arguments << QLatin1String("HEAD") << QLatin1String("--") << files;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    const QString output = commandOutputFromLocal8Bit(outputText);
    outputWindow()->append(output);
    // Note that git exits with 1 even if the operation is successful
    // Assume real failure if the output does not contain "foo.cpp modified"
    // or "Unstaged changes after reset" (git 1.7.0).
    if (!rc && (!output.contains(QLatin1String("modified"))
         && !output.contains(QLatin1String("Unstaged changes after reset")))) {
        const QString stdErr = commandOutputFromLocal8Bit(errorText);
        const QString msg = files.isEmpty() ?
                            tr("Cannot reset \"%1\": %2").arg(QDir::toNativeSeparators(workingDirectory), stdErr) :
                            tr("Cannot reset %n file(s) in \"%1\": %2", 0, files.size()).
                            arg(QDir::toNativeSeparators(workingDirectory), stdErr);
        if (errorMessage) {
            *errorMessage = msg;
        } else {
            outputWindow()->appendError(msg);
        }
        return false;
    }
    return true;
}

// Initialize repository
bool GitClient::synchronousInit(const QString &workingDirectory)
{
    QByteArray outputText;
    QByteArray errorText;
    const QStringList arguments(QLatin1String("init"));
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    // '[Re]Initialized...'
    outputWindow()->append(commandOutputFromLocal8Bit(outputText));
    if (!rc)
        outputWindow()->appendError(commandOutputFromLocal8Bit(errorText));
    else {
        // TODO: Turn this into a VcsBaseClient and use resetCachedVcsInfo(...)
        Core::VcsManager *vcsManager = m_core->vcsManager();
        vcsManager->resetVersionControlForDirectory(workingDirectory);
    }
    return rc;
}

/* Checkout, supports:
 * git checkout -- <files>
 * git checkout revision -- <files>
 * git checkout revision -- . */
bool GitClient::synchronousCheckoutFiles(const QString &workingDirectory,
                                         QStringList files /* = QStringList() */,
                                         QString revision /* = QString() */,
                                         QString *errorMessage /* = 0 */,
                                         bool revertStaging /* = true */)
{
    if (revertStaging && revision.isEmpty())
        revision = QLatin1String("HEAD");
    if (files.isEmpty())
        files = QStringList(QString(QLatin1Char('.')));
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("checkout");
    if (revertStaging)
        arguments << revision;
    arguments << QLatin1String("--") << files;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString fileArg = files.join(QLatin1String(", "));
        //: Meaning of the arguments: %1: revision, %2: files, %3: repository,
        //: %4: Error message
        const QString msg = tr("Cannot checkout \"%1\" of %2 in \"%3\": %4").
                            arg(revision, fileArg, workingDirectory, commandOutputFromLocal8Bit(errorText));
        if (errorMessage) {
            *errorMessage = msg;
        } else {
            outputWindow()->appendError(msg);
        }
        return false;
    }
    return true;
}

static inline QString msgParentRevisionFailed(const QString &workingDirectory,
                                              const QString &revision,
                                              const QString &why)
{
    //: Failed to find parent revisions of a SHA1 for "annotate previous"
    return GitClient::tr("Cannot find parent revisions of \"%1\" in \"%2\": %3").arg(revision, workingDirectory, why);
}

static inline QString msgInvalidRevision()
{
    return GitClient::tr("Invalid revision");
}

// Split a line of "<commit> <parent1> ..." to obtain parents from "rev-list" or "log".
static inline bool splitCommitParents(const QString &line,
                                      QString *commit = 0,
                                      QStringList *parents = 0)
{
    if (commit)
        commit->clear();
    if (parents)
        parents->clear();
    QStringList tokens = line.trimmed().split(QLatin1Char(' '));
    if (tokens.size() < 2)
        return false;
    if (commit)
        *commit = tokens.front();
    tokens.pop_front();
    if (parents)
        *parents = tokens;
    return true;
}

// Find out the immediate parent revisions of a revision of the repository.
// Might be several in case of merges.
bool GitClient::synchronousParentRevisions(const QString &workingDirectory,
                                           const QStringList &files /* = QStringList() */,
                                           const QString &revision,
                                           QStringList *parents,
                                           QString *errorMessage)
{
    QByteArray outputTextData;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("rev-list") << QLatin1String(GitClient::noColorOption)
              << QLatin1String("--parents") << QLatin1String("--max-count=1") << revision;
    if (!files.isEmpty()) {
        arguments.append(QLatin1String("--"));
        arguments.append(files);
    }
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputTextData, &errorText);
    if (!rc) {
        *errorMessage = msgParentRevisionFailed(workingDirectory, revision, commandOutputFromLocal8Bit(errorText));
        return false;
    }
    // Should result in one line of blank-delimited revisions, specifying current first
    // unless it is top.
    QString outputText = commandOutputFromLocal8Bit(outputTextData);
    outputText.remove(QLatin1Char('\n'));
    if (!splitCommitParents(outputText, 0, parents)) {
        *errorMessage = msgParentRevisionFailed(workingDirectory, revision, msgInvalidRevision());
        return false;
    }
    return true;
}

// Short SHA1, author, subject
static const char defaultShortLogFormatC[] = "%h (%an \"%s\")";

bool GitClient::synchronousShortDescription(const QString &workingDirectory, const QString &revision,
                                            QString *description, QString *errorMessage)
{
    // Short SHA 1, author, subject
    return synchronousShortDescription(workingDirectory, revision,
                                       QLatin1String(defaultShortLogFormatC),
                                       description, errorMessage);
}

// Convenience working on a list of revisions
bool GitClient::synchronousShortDescriptions(const QString &workingDirectory, const QStringList &revisions,
                                             QStringList *descriptions, QString *errorMessage)
{
    descriptions->clear();
    foreach (const QString &revision, revisions) {
        QString description;
        if (!synchronousShortDescription(workingDirectory, revision, &description, errorMessage)) {
            descriptions->clear();
            return false;
        }
        descriptions->push_back(description);
    }
    return true;
}

static inline QString msgCannotDetermineBranch(const QString &workingDirectory, const QString &why)
{
    return GitClient::tr("Cannot retrieve branch of \"%1\": %2").arg(QDir::toNativeSeparators(workingDirectory), why);
}

// Retrieve head revision/branch
bool GitClient::synchronousTopRevision(const QString &workingDirectory, QString *revision,
                                       QString *branch, QString *errorMessageIn)
{
    QByteArray outputTextData;
    QByteArray errorText;
    QStringList arguments;
    QString errorMessage;
    do {
        // get revision
        if (revision) {
            revision->clear();
            arguments << QLatin1String("log") << QLatin1String(noColorOption)
                    <<  QLatin1String("--max-count=1") << QLatin1String("--pretty=format:%H");
            if (!fullySynchronousGit(workingDirectory, arguments, &outputTextData, &errorText)) {
                errorMessage = tr("Cannot retrieve top revision of \"%1\": %2").arg(QDir::toNativeSeparators(workingDirectory), commandOutputFromLocal8Bit(errorText));
                break;
            }
            *revision = commandOutputFromLocal8Bit(outputTextData);
            revision->remove(QLatin1Char('\n'));
        } // revision desired
        // get branch
        if (branch) {
            branch->clear();
            arguments.clear();
            arguments << QLatin1String("branch") << QLatin1String(noColorOption);
            if (!fullySynchronousGit(workingDirectory, arguments, &outputTextData, &errorText)) {
                errorMessage = msgCannotDetermineBranch(workingDirectory, commandOutputFromLocal8Bit(errorText));
                break;
            }
            /* parse output for current branch: \code
* master
  branch2
\endcode */
            const QString branchPrefix = QLatin1String("* ");
            foreach(const QString &line, commandOutputLinesFromLocal8Bit(outputTextData)) {
                if (line.startsWith(branchPrefix)) {
                    *branch = line;
                    branch->remove(0, branchPrefix.size());
                    break;
                }
            }
            if (branch->isEmpty()) {
                errorMessage = msgCannotDetermineBranch(workingDirectory,
                                                        QString::fromLatin1("Internal error: Failed to parse output: %1").arg(commandOutputFromLocal8Bit(outputTextData)));
                break;
            }
        } // branch
    } while (false);
    const bool failed = (revision && revision->isEmpty()) || (branch && branch->isEmpty());
    if (failed && !errorMessage.isEmpty()) {
        if (errorMessageIn) {
            *errorMessageIn = errorMessage;
        } else {
            outputWindow()->appendError(errorMessage);
        }
    }
    return !failed;
}

// Format an entry in a one-liner for selection list using git log.
bool GitClient::synchronousShortDescription(const QString &workingDirectory, const QString &revision,
                                            const QString &format, QString *description,
                                            QString *errorMessage)
{
    QByteArray outputTextData;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("log") << QLatin1String(GitClient::noColorOption)
              << (QLatin1String("--pretty=format:") + format)
              << QLatin1String("--max-count=1") << revision;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputTextData, &errorText);
    if (!rc) {
        *errorMessage = tr("Cannot describe revision \"%1\" in \"%2\": %3").arg(revision, workingDirectory, commandOutputFromLocal8Bit(errorText));
        return false;
    }
    *description = commandOutputFromLocal8Bit(outputTextData);
    if (description->endsWith(QLatin1Char('\n')))
        description->truncate(description->size() - 1);
    return true;
}

// Create a default message to be used for describing stashes
static inline QString creatorStashMessage(const QString &keyword = QString())
{
    QString rc = QCoreApplication::applicationName();
    rc += QLatin1Char(' ');
    if (!keyword.isEmpty()) {
        rc += keyword;
        rc += QLatin1Char(' ');
    }
    rc += QDateTime::currentDateTime().toString(Qt::ISODate);
    return rc;
}

/* Do a stash and return the message as identifier. Note that stash names (stash{n})
 * shift as they are pushed, so, enforce the use of messages to identify them. Flags:
 * StashPromptDescription: Prompt the user for a description message.
 * StashImmediateRestore: Immediately re-apply this stash (used for snapshots), user keeps on working
 * StashIgnoreUnchanged: Be quiet about unchanged repositories (used for IVersionControl's snapshots). */

QString GitClient::synchronousStash(const QString &workingDirectory, const QString &messageKeyword,
                                    unsigned flags, bool *unchanged)
{
    if (unchanged)
        *unchanged = false;
    QString message;
    bool success = false;
    // Check for changes and stash
    QString errorMessage;
    switch (gitStatus(workingDirectory, false, 0, &errorMessage)) {
    case  StatusChanged: {
            message = creatorStashMessage(messageKeyword);
            do {
                if ((flags & StashPromptDescription)) {
                    if (!inputText(Core::ICore::instance()->mainWindow(),
                                   tr("Stash Description"), tr("Description:"), &message))
                        break;
                }
                if (!executeSynchronousStash(workingDirectory, message))
                    break;
                if ((flags & StashImmediateRestore)
                    && !synchronousStashRestore(workingDirectory, QLatin1String("stash@{0}")))
                    break;
                success = true;
            } while (false);
        }
        break;
    case StatusUnchanged:
        if (unchanged)
            *unchanged = true;
        if (!(flags & StashIgnoreUnchanged))
            outputWindow()->append(msgNoChangedFiles());
        break;
    case StatusFailed:
        outputWindow()->append(errorMessage);
        break;
    }
    if (!success)
        message.clear();
    return message;
}

bool GitClient::executeSynchronousStash(const QString &workingDirectory,
                                        const QString &message,
                                        QString *errorMessage)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("stash");
    if (!message.isEmpty())
        arguments << QLatin1String("save") << message;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString msg = tr("Cannot stash in \"%1\": %2").
                            arg(QDir::toNativeSeparators(workingDirectory),
                                commandOutputFromLocal8Bit(errorText));
        if (errorMessage) {
            *errorMessage = msg;
        } else {
            outputWindow()->append(msg);
        }
        return false;
    }
    return true;
}

// Resolve a stash name from message
bool GitClient::stashNameFromMessage(const QString &workingDirectory,
                                     const QString &message, QString *name,
                                     QString *errorMessage)
{
    // All happy
    if (message.startsWith(QLatin1String(stashNamePrefix))) {
        *name = message;
        return true;
    }
    // Retrieve list and find via message
    QList<Stash> stashes;
    if (!synchronousStashList(workingDirectory, &stashes, errorMessage))
        return false;
    foreach (const Stash &s, stashes) {
        if (s.message == message) {
            *name = s.name;
            return true;
        }
    }
    //: Look-up of a stash via its descriptive message failed.
    const QString msg = tr("Cannot resolve stash message \"%1\" in \"%2\".").arg(message, workingDirectory);
    if (errorMessage) {
        *errorMessage = msg;
    } else {
        outputWindow()->append(msg);
    }
    return  false;
}

bool GitClient::synchronousBranchCmd(const QString &workingDirectory, QStringList branchArgs,
                                     QString *output, QString *errorMessage)
{
    branchArgs.push_front(QLatin1String("branch"));
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, branchArgs, &outputText, &errorText);
    if (!rc) {
        *errorMessage = tr("Cannot run \"git branch\" in \"%1\": %2").arg(QDir::toNativeSeparators(workingDirectory), commandOutputFromLocal8Bit(errorText));
        return false;
    }
    *output = commandOutputFromLocal8Bit(outputText);
    return true;
}

bool GitClient::synchronousRemoteCmd(const QString &workingDirectory, QStringList remoteArgs,
                                     QString *output, QString *errorMessage)
{
    remoteArgs.push_front(QLatin1String("remote"));
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, remoteArgs, &outputText, &errorText);
    if (!rc) {
        *errorMessage = tr("Cannot run \"git remote\" in \"%1\": %2").arg(QDir::toNativeSeparators(workingDirectory), commandOutputFromLocal8Bit(errorText));
        return false;
    }
    *output = commandOutputFromLocal8Bit(outputText);
    return true;
}

bool GitClient::synchronousShow(const QString &workingDirectory, const QString &id,
                                 QString *output, QString *errorMessage)
{
    if (!canShow(id)) {
        *errorMessage = msgCannotShow(id);
        return false;
    }
    QStringList args(QLatin1String("show"));
    args << QLatin1String(decorateOption) << QLatin1String(noColorOption) << id;
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, args, &outputText, &errorText);
    if (!rc) {
        *errorMessage = tr("Cannot run \"git show\" in \"%1\": %2").arg(QDir::toNativeSeparators(workingDirectory), commandOutputFromLocal8Bit(errorText));
        return false;
    }
    *output = commandOutputFromLocal8Bit(outputText);
    return true;
}

// Retrieve list of files to be cleaned
bool GitClient::synchronousCleanList(const QString &workingDirectory,
                                     QStringList *files, QString *errorMessage)
{
    files->clear();
    QStringList args;
    args << QLatin1String("clean") << QLatin1String("--dry-run") << QLatin1String("-dxf");
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, args, &outputText, &errorText);
    if (!rc) {
        *errorMessage = tr("Cannot run \"git clean\" in \"%1\": %2").arg(QDir::toNativeSeparators(workingDirectory), commandOutputFromLocal8Bit(errorText));
        return false;
    }
    // Filter files that git would remove
    const QString prefix = QLatin1String("Would remove ");
    foreach(const QString &line, commandOutputLinesFromLocal8Bit(outputText))
        if (line.startsWith(prefix))
            files->push_back(line.mid(prefix.size()));
    return true;
}

bool GitClient::synchronousApplyPatch(const QString &workingDirectory,
                                      const QString &file, QString *errorMessage)
{
    QStringList args;
    args << QLatin1String("apply") << QLatin1String("--whitespace=fix") << file;
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, args, &outputText, &errorText);
    if (rc) {
        if (!errorText.isEmpty())
            *errorMessage = tr("There were warnings while applying \"%1\" to \"%2\":\n%3").arg(file, workingDirectory, commandOutputFromLocal8Bit(errorText));
    } else {
        *errorMessage = tr("Cannot apply patch \"%1\" to \"%2\": %3").arg(file, workingDirectory, commandOutputFromLocal8Bit(errorText));
        return false;
    }
    return true;
}

// Factory function to create an asynchronous command
VcsBase::Command *GitClient::createCommand(const QString &workingDirectory,
                                           VcsBase::VcsBaseEditorWidget* editor,
                                           bool useOutputToWindow,
                                           int editorLineNumber)
{
    VcsBase::Command *command = new VcsBase::Command(gitBinaryPath(), workingDirectory, processEnvironment());
    command->setCookie(QVariant(editorLineNumber));
    if (editor)
        connect(command, SIGNAL(finished(bool,int,QVariant)), editor, SLOT(commandFinishedGotoLine(bool,int,QVariant)));
    if (useOutputToWindow) {
        if (editor) // assume that the commands output is the important thing
            connect(command, SIGNAL(outputData(QByteArray)), outputWindow(), SLOT(appendDataSilently(QByteArray)));
        else
            connect(command, SIGNAL(outputData(QByteArray)), outputWindow(), SLOT(appendData(QByteArray)));
    } else {
        if (editor)
            connect(command, SIGNAL(outputData(QByteArray)), editor, SLOT(setPlainTextDataFiltered(QByteArray)));
    }

    if (outputWindow())
        connect(command, SIGNAL(errorText(QString)), outputWindow(), SLOT(appendError(QString)));
    return command;
}

// Execute a single command
VcsBase::Command *GitClient::executeGit(const QString &workingDirectory,
                                        const QStringList &arguments,
                                        VcsBase::VcsBaseEditorWidget* editor,
                                        bool useOutputToWindow,
                                        VcsBase::Command::TerminationReportMode tm,
                                        int editorLineNumber,
                                        bool unixTerminalDisabled)
{
    outputWindow()->appendCommand(workingDirectory, settings()->stringValue(GitSettings::binaryPathKey), arguments);
    VcsBase::Command *command = createCommand(workingDirectory, editor, useOutputToWindow, editorLineNumber);
    command->addJob(arguments, settings()->intValue(GitSettings::timeoutKey));
    command->setTerminationReportMode(tm);
    command->setUnixTerminalDisabled(unixTerminalDisabled);
    command->execute();
    return command;
}

QProcessEnvironment GitClient::processEnvironment() const
{

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (settings()->boolValue(GitSettings::adoptPathKey))
        environment.insert(QLatin1String("PATH"), settings()->stringValue(GitSettings::pathKey));
#ifdef Q_OS_WIN
    if (settings()->boolValue(GitSettings::winSetHomeEnvironmentKey))
        environment.insert(QLatin1String("HOME"), QDir::toNativeSeparators(QDir::homePath()));
#endif // Q_OS_WIN
    // Set up SSH and C locale (required by git using perl).
    VcsBase::VcsBasePlugin::setProcessEnvironment(&environment, false);
    return environment;
}

// Synchronous git execution using Utils::SynchronousProcess, with
// log windows updating.
Utils::SynchronousProcessResponse GitClient::synchronousGit(const QString &workingDirectory,
                                                            const QStringList &gitArguments,
                                                            unsigned flags,
                                                            QTextCodec *stdOutCodec)
{
    return VcsBase::VcsBasePlugin::runVcs(workingDirectory, gitBinaryPath(), gitArguments,
                                          settings()->intValue(GitSettings::timeoutKey) * 1000,
                                          processEnvironment(),
                                          flags, stdOutCodec);
}

bool GitClient::fullySynchronousGit(const QString &workingDirectory,
                                    const QStringList &gitArguments,
                                    QByteArray* outputText,
                                    QByteArray* errorText,
                                    bool logCommandToWindow) const
{
    return VcsBase::VcsBasePlugin::runFullySynchronous(workingDirectory, gitBinaryPath(), gitArguments,
                                                       processEnvironment(), outputText, errorText,
                                                       settings()->intValue(GitSettings::timeoutKey) * 1000,
                                                       logCommandToWindow);
}

static inline int askWithDetailedText(QWidget *parent,
                                      const QString &title, const QString &msg,
                                      const QString &inf,
                                      QMessageBox::StandardButton defaultButton,
                                      QMessageBox::StandardButtons buttons = QMessageBox::Yes|QMessageBox::No)
{
    QMessageBox msgBox(QMessageBox::Question, title, msg, buttons, parent);
    msgBox.setDetailedText(inf);
    msgBox.setDefaultButton(defaultButton);
    return msgBox.exec();
}

// Convenience that pops up an msg box.
GitClient::StashResult GitClient::ensureStash(const QString &workingDirectory)
{
    QString errorMessage;
    const StashResult sr = ensureStash(workingDirectory, &errorMessage);
    if (sr == StashFailed)
        outputWindow()->appendError(errorMessage);
    return sr;
}

// Ensure that changed files are stashed before a pull or similar
GitClient::StashResult GitClient::ensureStash(const QString &workingDirectory, QString *errorMessage)
{
    QString statusOutput;
    switch (gitStatus(workingDirectory, false, &statusOutput, errorMessage)) {
        case StatusChanged:
        break;
        case StatusUnchanged:
        return StashUnchanged;
        case StatusFailed:
        return StashFailed;
    }

    const int answer = askWithDetailedText(m_core->mainWindow(), tr("Changes"),
                             tr("Would you like to stash your changes?"),
                             statusOutput, QMessageBox::Yes, QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
    switch (answer) {
        case QMessageBox::Cancel:
            return StashCanceled;
        case QMessageBox::Yes:
            if (!executeSynchronousStash(workingDirectory, creatorStashMessage(QLatin1String("push")), errorMessage))
                return StashFailed;
            break;
        case QMessageBox::No: // At your own risk, so.
            return NotStashed;
        }

    return Stashed;
 }

// Trim a git status file spec: "modified:    foo .cpp" -> "modified: foo .cpp"
static inline QString trimFileSpecification(QString fileSpec)
{
    const int colonIndex = fileSpec.indexOf(QLatin1Char(':'));
    if (colonIndex != -1) {
        // Collapse the sequence of spaces
        const int filePos = colonIndex + 2;
        int nonBlankPos = filePos;
        for ( ; fileSpec.at(nonBlankPos).isSpace(); nonBlankPos++) ;
        if (nonBlankPos > filePos)
            fileSpec.remove(filePos, nonBlankPos - filePos);
    }
    return fileSpec;
}

GitClient::StatusResult GitClient::gitStatus(const QString &workingDirectory, bool untracked,
                                             QString *output, QString *errorMessage, bool *onBranch)
{
    // Run 'status'. Note that git returns exitcode 1 if there are no added files.
    QByteArray outputText;
    QByteArray errorText;

    QStringList statusArgs(QLatin1String("status"));
    if (untracked)
        statusArgs << QLatin1String("-u");
    statusArgs << QLatin1String("-s") << QLatin1String("-b");

    const bool statusRc = fullySynchronousGit(workingDirectory, statusArgs, &outputText, &errorText);
    if (output)
        *output = commandOutputFromLocal8Bit(outputText);

    static const char * NO_BRANCH = "## HEAD (no branch)\n";

    const bool branchKnown = !outputText.startsWith(NO_BRANCH);
    if (onBranch)
        *onBranch = branchKnown;
    // Is it something really fatal?
    if (!statusRc && !branchKnown) {
        if (errorMessage) {
            const QString error = commandOutputFromLocal8Bit(errorText);
            *errorMessage = tr("Cannot obtain status: %1").arg(error);
        }
        return StatusFailed;
    }
    // Unchanged (output text depending on whether -u was passed)
    if (outputText.count('\n') == 1)
        return StatusUnchanged;
    return StatusChanged;
}

// Quietly retrieve branch list of remote repository URL
//
// The branch HEAD is pointing to is always returned first.
QStringList GitClient::synchronousRepositoryBranches(const QString &repositoryURL)
{
    QStringList arguments(QLatin1String("ls-remote"));
    arguments << repositoryURL << QLatin1String("HEAD") << QLatin1String("refs/heads/*");
    const unsigned flags =
            VcsBase::VcsBasePlugin::SshPasswordPrompt|
            VcsBase::VcsBasePlugin::SuppressStdErrInLogWindow|
            VcsBase::VcsBasePlugin::SuppressFailMessageInLogWindow;
    const Utils::SynchronousProcessResponse resp = synchronousGit(QString(), arguments, flags);
    QStringList branches;
    branches << "<detached HEAD>";
    QString headSha;
    if (resp.result == Utils::SynchronousProcessResponse::Finished) {
        // split "82bfad2f51d34e98b18982211c82220b8db049b<tab>refs/heads/master"
        foreach(const QString &line, resp.stdOut.split(QLatin1Char('\n'))) {
            if (line.endsWith("\tHEAD")) {
                Q_ASSERT(headSha.isNull());
                headSha = line.left(line.indexOf(QChar('\t')));
                continue;
            }

            const QString pattern = QLatin1String("\trefs/heads/");
            const int pos = line.lastIndexOf(pattern);
            if (pos != -1) {
                const QString branchName = line.mid(pos + pattern.count());
                if (line.startsWith(headSha))
                    branches[0] = branchName;
                else
                    branches.push_back(branchName);
            }
        }
    }
    return branches;
}

void GitClient::launchGitK(const QString &workingDirectory)
{
    const QFileInfo binaryInfo(gitBinaryPath());
    QDir foundBinDir(binaryInfo.dir());
    const bool foundBinDirIsCmdDir = foundBinDir.dirName() == "cmd";
    QProcessEnvironment env = processEnvironment();
    if (tryLauchingGitK(env, workingDirectory, foundBinDir.path(), foundBinDirIsCmdDir))
        return;
    if (!foundBinDirIsCmdDir)
        return;
    foundBinDir.cdUp();
    tryLauchingGitK(env, workingDirectory, foundBinDir.path() + "/bin", false);
}

bool GitClient::tryLauchingGitK(const QProcessEnvironment &env,
                                const QString &workingDirectory,
                                const QString &gitBinDirectory,
                                bool silent)
{
#ifdef Q_OS_WIN
    // Launch 'wish' shell from git binary directory with the gitk located there
    const QString binary = gitBinDirectory + QLatin1String("/wish");
    QStringList arguments(gitBinDirectory + QLatin1String("/gitk"));
#else
    // Simple: Run gitk from binary path
    const QString binary = gitBinDirectory + QLatin1String("/gitk");
    QStringList arguments;
#endif
    VcsBase::VcsBaseOutputWindow *outwin = VcsBase::VcsBaseOutputWindow::instance();
    const QString gitkOpts = settings()->stringValue(GitSettings::gitkOptionsKey);
    if (!gitkOpts.isEmpty())
        arguments.append(Utils::QtcProcess::splitArgs(gitkOpts));
    outwin->appendCommand(workingDirectory, binary, arguments);
    // This should always use QProcess::startDetached (as not to kill
    // the child), but that does not have an environment parameter.
    bool success = false;
    if (settings()->boolValue(GitSettings::adoptPathKey)) {
        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(workingDirectory);
        process->setProcessEnvironment(env);
        process->start(binary, arguments);
        success = process->waitForStarted();
        if (success)
            connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));
        else
            delete process;
    } else {
        success = QProcess::startDetached(binary, arguments, workingDirectory);
    }
    if (!success) {
        const QString error = tr("Cannot launch \"%1\".").arg(binary);
        if (silent)
            outwin->appendSilently(error);
        else
            outwin->appendError(error);
    }
    return success;
}

QString GitClient::gitBinaryPath(bool *ok, QString *errorMessage) const
{
    return settings()->gitBinaryPath(ok, errorMessage);
}

bool GitClient::getCommitData(const QString &workingDirectory,
                              bool amend,
                              QString *commitTemplate,
                              CommitData *commitData,
                              QString *errorMessage)
{
    commitData->clear();

    // Find repo
    const QString repoDirectory = GitClient::findRepositoryForDirectory(workingDirectory);
    if (repoDirectory.isEmpty()) {
        *errorMessage = msgRepositoryNotFound(workingDirectory);
        return false;
    }

    commitData->panelInfo.repository = repoDirectory;

    QDir gitDir(repoDirectory);
    if (!gitDir.cd(QLatin1String(GIT_DIRECTORY))) {
        *errorMessage = tr("The repository \"%1\" is not initialized.").arg(repoDirectory);
        return false;
    }

    // Read description
    const QString descriptionFile = gitDir.absoluteFilePath(QLatin1String("description"));
    if (QFileInfo(descriptionFile).isFile()) {
        Utils::FileReader reader;
        if (!reader.fetch(descriptionFile, QIODevice::Text, errorMessage))
            return false;
        commitData->panelInfo.description = commandOutputFromLocal8Bit(reader.data()).trimmed();
    }

    // Run status. Note that it has exitcode 1 if there are no added files.
    bool onBranch;
    QString output;
    const StatusResult status = gitStatus(repoDirectory, true, &output, errorMessage, &onBranch);
    switch (status) {
    case  StatusChanged:
        if (!onBranch) {
            *errorMessage = tr("You did not checkout a branch.");
            return false;
        }
        break;
    case StatusUnchanged:
        if (amend)
            break;
        *errorMessage = msgNoChangedFiles();
        return false;
    case StatusFailed:
        return false;
    }

    //    Output looks like:
    //    ## branch_name
    //    MM filename
    //     A new_unstaged_file
    //    R  old -> new
    //    ?? missing_file
    if (status != StatusUnchanged) {
        if (!commitData->parseFilesFromStatus(output)) {
            *errorMessage = msgParseFilesFailed();
            return false;
        }

        // Filter out untracked files that are not part of the project
        QStringList untrackedFiles = commitData->filterFiles(CommitData::UntrackedFile);

        VcsBase::VcsBaseSubmitEditor::filterUntrackedFilesOfProject(repoDirectory, &untrackedFiles);
        QList<CommitData::StateFilePair> filteredFiles;
        QList<CommitData::StateFilePair>::const_iterator it = commitData->files.constBegin();
        for ( ; it != commitData->files.constEnd(); ++it) {
            if (it->first == CommitData::UntrackedFile && !untrackedFiles.contains(it->second))
                continue;
            filteredFiles.append(*it);
        }
        commitData->files = filteredFiles;

        if (commitData->files.isEmpty()) {
            *errorMessage = msgNoChangedFiles();
            return false;
        }
    }

    commitData->panelData.author = readConfigValue(workingDirectory, QLatin1String("user.name"));
    commitData->panelData.email = readConfigValue(workingDirectory, QLatin1String("user.email"));
    commitData->commitEncoding = readConfigValue(workingDirectory, QLatin1String("i18n.commitEncoding"));

    // Get the commit template or the last commit message
    if (amend) {
        // Amend: get last commit data as "SHA1@message".
        QStringList args(QLatin1String("log"));
        const QString format = synchronousGitVersion(true) > 0x010701 ? "%h@%B" : "%h@%s%n%n%b";
        args << QLatin1String("--max-count=1") << QLatin1String("--pretty=format:") + format;
        QTextCodec *codec = QTextCodec::codecForName(commitData->commitEncoding.toLocal8Bit());
        const Utils::SynchronousProcessResponse sp = synchronousGit(repoDirectory, args, 0, codec);
        if (sp.result != Utils::SynchronousProcessResponse::Finished) {
            *errorMessage = tr("Cannot retrieve last commit data of repository \"%1\".").arg(repoDirectory);
            return false;
        }
        const int separatorPos = sp.stdOut.indexOf(QLatin1Char('@'));
        QTC_ASSERT(separatorPos != -1, return false)
        commitData->amendSHA1= sp.stdOut.left(separatorPos);
        *commitTemplate = sp.stdOut.mid(separatorPos + 1);
    } else {
        // Commit: Get the commit template
        QString templateFilename = gitDir.absoluteFilePath(QLatin1String("MERGE_MSG"));
        if (!QFileInfo(templateFilename).isFile())
            templateFilename = readConfigValue(workingDirectory, QLatin1String("commit.template"));
        if (!templateFilename.isEmpty()) {
            // Make relative to repository
            const QFileInfo templateFileInfo(templateFilename);
            if (templateFileInfo.isRelative())
                templateFilename = repoDirectory + QLatin1Char('/') + templateFilename;
            Utils::FileReader reader;
            if (!reader.fetch(templateFilename, QIODevice::Text, errorMessage))
                return false;
            *commitTemplate = QString::fromLocal8Bit(reader.data());
        }
    }
    return true;
}

// Log message for commits/amended commits to go to output window
static inline QString msgCommitted(const QString &amendSHA1, int fileCount)
{
    if (amendSHA1.isEmpty())
        return GitClient::tr("Committed %n file(s).\n", 0, fileCount);
    if (fileCount)
        return GitClient::tr("Amended \"%1\" (%n file(s)).\n", 0, fileCount).arg(amendSHA1);
    return GitClient::tr("Amended \"%1\".").arg(amendSHA1);
}

bool GitClient::addAndCommit(const QString &repositoryDirectory,
                             const GitSubmitEditorPanelData &data,
                             const QString &amendSHA1,
                             const QString &messageFile,
                             VcsBase::SubmitFileModel *model)
{
    const QString renameSeparator = QLatin1String(" -> ");
    const bool amend = !amendSHA1.isEmpty();

    QStringList filesToAdd;
    QStringList filesToRemove;
    QStringList filesToReset;

    int commitCount = 0;

    for (int i = 0; i < model->rowCount(); ++i) {
        const CommitData::FileState state = static_cast<CommitData::FileState>(model->data(i).toInt());
        QString file = model->file(i);
        const bool checked = model->checked(i);

        if (checked)
            ++commitCount;

        if (state == CommitData::UntrackedFile && checked)
            filesToAdd.append(file);

        if (state == CommitData::ModifiedStagedFile && !checked) {
            filesToReset.append(file);
        } else if (state == CommitData::AddedStagedFile && !checked) {
            filesToReset.append(file);
        } else if (state == CommitData::DeletedStagedFile && !checked) {
            filesToReset.append(file);
        } else if (state == CommitData::RenamedStagedFile && !checked) {
            const int pos = file.indexOf(QLatin1String(" -> "));
            const QString newFile = file.mid(pos + 4);
            filesToReset.append(newFile);
        } else if (state == CommitData::CopiedStagedFile && !checked) {
            const QString newFile = file.mid(file.indexOf(renameSeparator) + renameSeparator.count());
            filesToReset.append(newFile);
        } else if (state == CommitData::UpdatedStagedFile && !checked) {
            QTC_ASSERT(false, continue); // There should not be updated files when commiting!
        }

        if (state == CommitData::ModifiedFile && checked) {
            filesToReset.removeAll(file);
            filesToAdd.append(file);
        } else if (state == CommitData::AddedFile && checked) {
            QTC_ASSERT(false, continue); // these should be untracked!
        } else if (state == CommitData::DeletedFile && checked) {
            filesToReset.removeAll(file);
            filesToRemove.append(file);
        } else if (state == CommitData::RenamedFile && checked) {
            QTC_ASSERT(false, continue); // git mv directly stages.
        } else if (state == CommitData::CopiedFile && checked) {
            QTC_ASSERT(false, continue); // only is noticed after adding a new file to the index
        } else if (state == CommitData::UpdatedFile && checked) {
            QTC_ASSERT(false, continue); // There should not be updated files when commiting!
        }
    }

    if (!filesToReset.isEmpty() && !synchronousReset(repositoryDirectory, filesToReset))
        return false;

    if (!filesToRemove.isEmpty() && !synchronousDelete(repositoryDirectory, true, filesToRemove))
        return false;

    if (!filesToAdd.isEmpty() && !synchronousAdd(repositoryDirectory, false, filesToAdd))
        return false;

    // Do the final commit
    QStringList args;
    args << QLatin1String("commit")
         << QLatin1String("-F") << QDir::toNativeSeparators(messageFile);
    if (amend)
        args << QLatin1String("--amend");
    const QString &authorString =  data.authorString();
    if (!authorString.isEmpty())
         args << QLatin1String("--author") << authorString;

    QByteArray outputText;
    QByteArray errorText;

    const bool rc = fullySynchronousGit(repositoryDirectory, args, &outputText, &errorText);
    if (rc)
        outputWindow()->append(msgCommitted(amendSHA1, commitCount));
    else
        outputWindow()->appendError(tr("Cannot commit %n file(s): %1\n", 0, commitCount).arg(commandOutputFromLocal8Bit(errorText)));

    return rc;
}

/* Revert: This function can be called with a file list (to revert single
 * files)  or a single directory (revert all). Qt Creator currently has only
 * 'revert single' in its VCS menus, but the code is prepared to deal with
 * reverting a directory pending a sophisticated selection dialog in the
 * VcsBase plugin. */
GitClient::RevertResult GitClient::revertI(QStringList files,
                                           bool *ptrToIsDirectory,
                                           QString *errorMessage,
                                           bool revertStaging)
{
    if (files.empty())
        return RevertCanceled;

    // Figure out the working directory
    const QFileInfo firstFile(files.front());
    const bool isDirectory = firstFile.isDir();
    if (ptrToIsDirectory)
        *ptrToIsDirectory = isDirectory;
    const QString workingDirectory = isDirectory ? firstFile.absoluteFilePath() : firstFile.absolutePath();

    const QString repoDirectory = GitClient::findRepositoryForDirectory(workingDirectory);
    if (repoDirectory.isEmpty()) {
        *errorMessage = msgRepositoryNotFound(workingDirectory);
        return RevertFailed;
    }

    // Check for changes
    QString output;
    switch (gitStatus(repoDirectory, false, &output, errorMessage)) {
    case StatusChanged:
        break;
    case StatusUnchanged:
        return RevertUnchanged;
    case StatusFailed:
        return RevertFailed;
    }
    CommitData data;
    if (!data.parseFilesFromStatus(output)) {
        *errorMessage = msgParseFilesFailed();
        return RevertFailed;
    }

    // If we are looking at files, make them relative to the repository
    // directory to match them in the status output list.
    if (!isDirectory) {
        const QDir repoDir(repoDirectory);
        const QStringList::iterator cend = files.end();
        for (QStringList::iterator it = files.begin(); it != cend; ++it)
            *it = repoDir.relativeFilePath(*it);
    }

    // From the status output, determine all modified [un]staged files.
    const QStringList allStagedFiles = data.filterFiles(CommitData::ModifiedStagedFile);
    const QStringList allUnstagedFiles = data.filterFiles(CommitData::ModifiedFile);
    // Unless a directory was passed, filter all modified files for the
    // argument file list.
    QStringList stagedFiles = allStagedFiles;
    QStringList unstagedFiles = allUnstagedFiles;
    if (!isDirectory) {
        const QSet<QString> filesSet = files.toSet();
        stagedFiles = allStagedFiles.toSet().intersect(filesSet).toList();
        unstagedFiles = allUnstagedFiles.toSet().intersect(filesSet).toList();
    }
    if ((!revertStaging || stagedFiles.empty()) && unstagedFiles.empty())
        return RevertUnchanged;

    // Ask to revert (to do: Handle lists with a selection dialog)
    const QMessageBox::StandardButton answer
        = QMessageBox::question(m_core->mainWindow(),
                                tr("Revert"),
                                tr("The file has been changed. Do you want to revert it?"),
                                QMessageBox::Yes|QMessageBox::No,
                                QMessageBox::No);
    if (answer == QMessageBox::No)
        return RevertCanceled;

    // Unstage the staged files
    if (revertStaging && !stagedFiles.empty() && !synchronousReset(repoDirectory, stagedFiles, errorMessage))
        return RevertFailed;
    QStringList filesToRevert = unstagedFiles;
    if (revertStaging)
        filesToRevert += stagedFiles;
    // Finally revert!
    if (!synchronousCheckoutFiles(repoDirectory, filesToRevert, QString(), errorMessage, revertStaging))
        return RevertFailed;
    return RevertOk;
}

void GitClient::revert(const QStringList &files, bool revertStaging)
{
    bool isDirectory;
    QString errorMessage;
    switch (revertI(files, &isDirectory, &errorMessage, revertStaging)) {
    case RevertOk:
        GitPlugin::instance()->gitVersionControl()->emitFilesChanged(files);
        break;
    case RevertCanceled:
        break;
    case RevertUnchanged: {
        const QString msg = (isDirectory || files.size() > 1) ? msgNoChangedFiles() : tr("The file is not modified.");
        outputWindow()->append(msg);
    }
        break;
    case RevertFailed:
        outputWindow()->append(errorMessage);
        break;
    }
}

bool GitClient::synchronousFetch(const QString &workingDirectory, const QString &remote)
{
    QStringList arguments(QLatin1String("fetch"));
    if (!remote.isEmpty())
        arguments << remote;
    // Disable UNIX terminals to suppress SSH prompting.
    const unsigned flags = VcsBase::VcsBasePlugin::SshPasswordPrompt|VcsBase::VcsBasePlugin::ShowStdOutInLogWindow
                           |VcsBase::VcsBasePlugin::ShowSuccessMessage;
    const Utils::SynchronousProcessResponse resp = synchronousGit(workingDirectory, arguments, flags);
    return resp.result == Utils::SynchronousProcessResponse::Finished;
}

bool GitClient::synchronousPull(const QString &workingDirectory)
{
    return synchronousPull(workingDirectory, settings()->boolValue(GitSettings::pullRebaseKey));
}

bool GitClient::synchronousPull(const QString &workingDirectory, bool rebase)
{
    QStringList arguments(QLatin1String("pull"));
    if (rebase)
        arguments << QLatin1String("--rebase");
    // Disable UNIX terminals to suppress SSH prompting.
    const unsigned flags = VcsBase::VcsBasePlugin::SshPasswordPrompt|VcsBase::VcsBasePlugin::ShowStdOutInLogWindow;
    const Utils::SynchronousProcessResponse resp = synchronousGit(workingDirectory, arguments, flags);
    // Notify about changed files or abort the rebase.
    const bool ok = resp.result == Utils::SynchronousProcessResponse::Finished;
    if (ok) {
        GitPlugin::instance()->gitVersionControl()->emitRepositoryChanged(workingDirectory);
    } else {
        if (rebase)
            syncAbortPullRebase(workingDirectory);
    }
    return ok;
}

void GitClient::syncAbortPullRebase(const QString &workingDir)
{
    // Abort rebase to clean if something goes wrong
    VcsBase::VcsBaseOutputWindow *outwin = VcsBase::VcsBaseOutputWindow::instance();
    outwin->appendError(tr("The command 'git pull --rebase' failed, aborting rebase."));
    QStringList arguments;
    arguments << QLatin1String("rebase") << QLatin1String("--abort");
    QByteArray stdOut;
    QByteArray stdErr;
    const bool rc = fullySynchronousGit(workingDir, arguments, &stdOut, &stdErr, true);
    outwin->append(commandOutputFromLocal8Bit(stdOut));
    if (!rc)
        outwin->appendError(commandOutputFromLocal8Bit(stdErr));
}

// Subversion: git svn
void GitClient::synchronousSubversionFetch(const QString &workingDirectory)
{
    QStringList args;
    args << QLatin1String("svn") << QLatin1String("fetch");
    // Disable UNIX terminals to suppress SSH prompting.
    const unsigned flags = VcsBase::VcsBasePlugin::SshPasswordPrompt|VcsBase::VcsBasePlugin::ShowStdOutInLogWindow
                           |VcsBase::VcsBasePlugin::ShowSuccessMessage;
    const Utils::SynchronousProcessResponse resp = synchronousGit(workingDirectory, args, flags);
    // Notify about changes.
    if (resp.result == Utils::SynchronousProcessResponse::Finished)
        GitPlugin::instance()->gitVersionControl()->emitRepositoryChanged(workingDirectory);
}

void GitClient::subversionLog(const QString &workingDirectory)
{
    QStringList arguments;
    arguments << QLatin1String("svn") << QLatin1String("log");
    int logCount = settings()->intValue(GitSettings::logCountKey);
    if (logCount > 0)
         arguments << (QLatin1String("--limit=") + QString::number(logCount));

    // Create a command editor, no highlighting or interaction.
    const QString title = tr("Git SVN Log");
    const Core::Id editorId = Git::Constants::C_GIT_COMMAND_LOG_EDITOR;
    const QString sourceFile = VcsBase::VcsBaseEditorWidget::getSource(workingDirectory, QStringList());
    VcsBase::VcsBaseEditorWidget *editor = findExistingVCSEditor("svnLog", sourceFile);
    if (!editor)
        editor = createVcsEditor(editorId, title, sourceFile, CodecNone, "svnLog", sourceFile, 0);
    executeGit(workingDirectory, arguments, editor);
}

bool GitClient::synchronousPush(const QString &workingDirectory, const QString &remote)
{
    // Disable UNIX terminals to suppress SSH prompting.
    const unsigned flags = VcsBase::VcsBasePlugin::SshPasswordPrompt|VcsBase::VcsBasePlugin::ShowStdOutInLogWindow
                           |VcsBase::VcsBasePlugin::ShowSuccessMessage;
    QStringList arguments(QLatin1String("push"));
    if (!remote.isEmpty())
        arguments << remote;
    const Utils::SynchronousProcessResponse resp =
            synchronousGit(workingDirectory, arguments, flags);
    return resp.result == Utils::SynchronousProcessResponse::Finished;
}

QString GitClient::msgNoChangedFiles()
{
    return tr("There are no modified files.");
}

void GitClient::stashPop(const QString &workingDirectory)
{
    QStringList arguments(QLatin1String("stash"));
    arguments << QLatin1String("pop");
    VcsBase::Command *cmd = executeGit(workingDirectory, arguments, 0, true);
    connectRepositoryChanged(workingDirectory, cmd);
}

bool GitClient::synchronousStashRestore(const QString &workingDirectory,
                                        const QString &stash,
                                        const QString &branch /* = QString()*/,
                                        QString *errorMessage)
{
    QStringList arguments(QLatin1String("stash"));
    if (branch.isEmpty())
        arguments << QLatin1String("apply") << stash;
    else
        arguments << QLatin1String("branch") << branch << stash;
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString stdErr = commandOutputFromLocal8Bit(errorText);
        const QString nativeWorkingDir = QDir::toNativeSeparators(workingDirectory);
        const QString msg = branch.isEmpty() ?
                            tr("Cannot restore stash \"%1\": %2").
                            arg(nativeWorkingDir, stdErr) :
                            tr("Cannot restore stash \"%1\" to branch \"%2\": %3").
                            arg(nativeWorkingDir, branch, stdErr);
        if (errorMessage)
            *errorMessage = msg;
        else
            outputWindow()->append(msg);
        return false;
    }
    QString output = commandOutputFromLocal8Bit(outputText);
    if (!output.isEmpty())
        outputWindow()->append(output);
    GitPlugin::instance()->gitVersionControl()->emitRepositoryChanged(workingDirectory);
    return true;
}

bool GitClient::synchronousStashRemove(const QString &workingDirectory,
                            const QString &stash /* = QString() */,
                            QString *errorMessage /* = 0 */)
{
    QStringList arguments(QLatin1String("stash"));
    if (stash.isEmpty()) {
        arguments << QLatin1String("clear");
    } else {
        arguments << QLatin1String("drop") << stash;
    }
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString stdErr = commandOutputFromLocal8Bit(errorText);
        const QString nativeWorkingDir = QDir::toNativeSeparators(workingDirectory);
        const QString msg = stash.isEmpty() ?
                            tr("Cannot remove stashes of \"%1\": %2").
                            arg(nativeWorkingDir, stdErr) :
                            tr("Cannot remove stash \"%1\" of \"%2\": %3").
                            arg(stash, nativeWorkingDir, stdErr);
        if (errorMessage)
            *errorMessage = msg;
        else
            outputWindow()->append(msg);
        return false;
    }
    QString output = commandOutputFromLocal8Bit(outputText);
    if (!output.isEmpty())
        outputWindow()->append(output);
    return true;
}

void GitClient::branchList(const QString &workingDirectory)
{
    QStringList arguments(QLatin1String("branch"));
    arguments << QLatin1String("-r") << QLatin1String(noColorOption);
    executeGit(workingDirectory, arguments, 0, true);
}

void GitClient::stashList(const QString &workingDirectory)
{
    QStringList arguments(QLatin1String("stash"));
    arguments << QLatin1String("list") << QLatin1String(noColorOption);
    executeGit(workingDirectory, arguments, 0, true);
}

bool GitClient::synchronousStashList(const QString &workingDirectory,
                                     QList<Stash> *stashes,
                                     QString *errorMessage /* = 0 */)
{
    stashes->clear();
    QStringList arguments(QLatin1String("stash"));
    arguments << QLatin1String("list") << QLatin1String(noColorOption);
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString msg = tr("Cannot retrieve stash list of \"%1\": %2").
                            arg(QDir::toNativeSeparators(workingDirectory),
                                commandOutputFromLocal8Bit(errorText));
        if (errorMessage) {
            *errorMessage = msg;
        } else {
            outputWindow()->append(msg);
        }
        return false;
    }
    Stash stash;
    foreach(const QString &line, commandOutputLinesFromLocal8Bit(outputText))
        if (stash.parseStashLine(line))
            stashes->push_back(stash);
    return true;
}

QString GitClient::readConfig(const QString &workingDirectory, const QStringList &configVar) const
{
    QStringList arguments;
    arguments << QLatin1String("config") << configVar;

    QByteArray outputText;
    QByteArray errorText;
    if (fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText, false))
        return commandOutputFromLocal8Bit(outputText);
    return QString();
}

// Read a single-line config value, return trimmed
QString GitClient::readConfigValue(const QString &workingDirectory, const QString &configVar) const
{
    return readConfig(workingDirectory, QStringList(configVar)).remove(QLatin1Char('\n'));
}

bool GitClient::cloneRepository(const QString &directory,const QByteArray &url)
{
    QDir workingDirectory(directory);
    const unsigned flags = VcsBase::VcsBasePlugin::SshPasswordPrompt |
            VcsBase::VcsBasePlugin::ShowStdOutInLogWindow|
            VcsBase::VcsBasePlugin::ShowSuccessMessage;

    if (workingDirectory.exists()) {
        if (!synchronousInit(workingDirectory.path()))
            return false;

        QStringList arguments(QLatin1String("remote"));
        arguments << QLatin1String("add") << QLatin1String("origin") << url;
        if (!fullySynchronousGit(workingDirectory.path(), arguments, 0, 0, true))
            return false;

        arguments.clear();
        arguments << QLatin1String("fetch");
        const Utils::SynchronousProcessResponse resp =
                synchronousGit(workingDirectory.path(), arguments, flags);
        if (resp.result != Utils::SynchronousProcessResponse::Finished)
            return false;

        arguments.clear();
        arguments << QLatin1String("config")
                  << QLatin1String("branch.master.remote")
                  <<  QLatin1String("origin");
        if (!fullySynchronousGit(workingDirectory.path(), arguments, 0, 0, true))
            return false;

        arguments.clear();
        arguments << QLatin1String("config")
                  << QLatin1String("branch.master.merge")
                  << QLatin1String("refs/heads/master");
        if (!fullySynchronousGit(workingDirectory.path(), arguments, 0, 0, true))
            return false;

        return true;
    } else {
        QStringList arguments(QLatin1String("clone"));
        arguments << url << workingDirectory.dirName();
        workingDirectory.cdUp();
        const Utils::SynchronousProcessResponse resp =
                synchronousGit(workingDirectory.path(), arguments, flags);
        // TODO: Turn this into a VcsBaseClient and use resetCachedVcsInfo(...)
        Core::VcsManager *vcsManager = m_core->vcsManager();
        vcsManager->resetVersionControlForDirectory(workingDirectory.absolutePath());
        return (resp.result == Utils::SynchronousProcessResponse::Finished);
    }
}

QString GitClient::vcsGetRepositoryURL(const QString &directory)
{
    QStringList arguments(QLatin1String("config"));
    QByteArray outputText;

    arguments << QLatin1String("remote.origin.url");

    if (fullySynchronousGit(directory, arguments, &outputText, 0, false))
        return commandOutputFromLocal8Bit(outputText);
    return QString();
}

GitSettings *GitClient::settings() const
{
    return m_settings;
}

void GitClient::connectRepositoryChanged(const QString & repository, VcsBase::Command *cmd)
{
    // Bind command success termination with repository to changed signal
    if (!m_repositoryChangedSignalMapper) {
        m_repositoryChangedSignalMapper = new QSignalMapper(this);
        connect(m_repositoryChangedSignalMapper, SIGNAL(mapped(QString)),
                GitPlugin::instance()->gitVersionControl(), SIGNAL(repositoryChanged(QString)));
    }
    m_repositoryChangedSignalMapper->setMapping(cmd, repository);
    connect(cmd, SIGNAL(success()), m_repositoryChangedSignalMapper, SLOT(map()),
            Qt::QueuedConnection);
}

// determine version as '(major << 16) + (minor << 8) + patch' or 0.
unsigned GitClient::gitVersion(bool silent, QString *errorMessage) const
{
    const QString newGitBinary = gitBinaryPath();
    if (m_gitVersionForBinary != newGitBinary && !newGitBinary.isEmpty()) {
        // Do not execute repeatedly if that fails (due to git
        // not being installed) until settings are changed.
        m_cachedGitVersion = synchronousGitVersion(silent, errorMessage);
        m_gitVersionForBinary = newGitBinary;
    }
    return m_cachedGitVersion;
}

QString GitClient::gitVersionString(bool silent, QString *errorMessage) const
{
    if (const unsigned version = gitVersion(silent, errorMessage)) {
        QString rc;
        QTextStream(&rc) << (version >> 16) << '.'
                << (0xFF & (version >> 8)) << '.'
                << (version & 0xFF);
        return rc;
    }
    return QString();
}
// determine version as '(major << 16) + (minor << 8) + patch' or 0.
unsigned GitClient::synchronousGitVersion(bool silent, QString *errorMessage) const
{
    // run git --version
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(QString(), QStringList("--version"), &outputText, &errorText);
    if (!rc) {
        const QString msg = tr("Cannot determine git version: %1").arg(commandOutputFromLocal8Bit(errorText));
        if (errorMessage) {
            *errorMessage = msg;
        } else {
            if (silent) {
                outputWindow()->append(msg);
            } else {
                outputWindow()->appendError(msg);
            }
        }
        return 0;
    }
    // cut 'git version 1.6.5.1.sha'
    const QString output = commandOutputFromLocal8Bit(outputText);
    const QRegExp versionPattern(QLatin1String("^[^\\d]+([\\d])\\.([\\d])\\.([\\d]).*$"));
    QTC_ASSERT(versionPattern.isValid(), return 0);
    QTC_ASSERT(versionPattern.exactMatch(output), return 0);
    const unsigned major = versionPattern.cap(1).toUInt();
    const unsigned minor = versionPattern.cap(2).toUInt();
    const unsigned patch = versionPattern.cap(3).toUInt();
    return version(major, minor, patch);
}

} // namespace Internal
} // namespace Git

#include "gitclient.moc"
