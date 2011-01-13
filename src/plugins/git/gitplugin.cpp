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

#include "gitplugin.h"

#include "changeselectiondialog.h"
#include "commitdata.h"
#include "gitclient.h"
#include "gitconstants.h"
#include "giteditor.h"
#include "gitsubmiteditor.h"
#include "gitversioncontrol.h"
#include "branchdialog.h"
#include "clonewizard.h"
#include "gitoriousclonewizard.h"
#include "stashdialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/filemanager.h>

#include <utils/qtcassert.h>
#include <utils/parameteraction.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <vcsbase/cleandialog.h>
#include <locator/commandlocator.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>
#include <QtCore/QtPlugin>

#include <QtGui/QAction>
#include <QtGui/QFileDialog>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>

static const VCSBase::VCSBaseEditorParameters editorParameters[] = {
{
    VCSBase::RegularCommandOutput,
    Git::Constants::GIT_COMMAND_LOG_EDITOR_ID,
    Git::Constants::GIT_COMMAND_LOG_EDITOR_DISPLAY_NAME,
    Git::Constants::C_GIT_COMMAND_LOG_EDITOR,
    "application/vnd.nokia.text.scs_git_commandlog",
    "gitlog"},
{   VCSBase::LogOutput,
    Git::Constants::GIT_LOG_EDITOR_ID,
    Git::Constants::GIT_LOG_EDITOR_DISPLAY_NAME,
    Git::Constants::C_GIT_LOG_EDITOR,
    "application/vnd.nokia.text.scs_git_filelog",
    "gitfilelog"},
{   VCSBase::AnnotateOutput,
    Git::Constants::GIT_BLAME_EDITOR_ID,
    Git::Constants::GIT_BLAME_EDITOR_DISPLAY_NAME,
    Git::Constants::C_GIT_BLAME_EDITOR,
    "application/vnd.nokia.text.scs_git_annotation",
    "gitsannotate"},
{   VCSBase::DiffOutput,
    Git::Constants::GIT_DIFF_EDITOR_ID,
    Git::Constants::GIT_DIFF_EDITOR_DISPLAY_NAME,
    Git::Constants::C_GIT_DIFF_EDITOR,
    "text/x-patch","diff"}
};

// Utility to find a parameter set by type
static inline const VCSBase::VCSBaseEditorParameters *findType(int ie)
{
    const VCSBase::EditorContentType et = static_cast<VCSBase::EditorContentType>(ie);
    return  VCSBase::VCSBaseEditor::findType(editorParameters, sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters), et);
}

Q_DECLARE_METATYPE(Git::Internal::GitClientMemberFunc)

using namespace Git;
using namespace Git::Internal;

// GitPlugin

GitPlugin *GitPlugin::m_instance = 0;

GitPlugin::GitPlugin() :
    VCSBase::VCSBasePlugin(QLatin1String(Git::Constants::GITSUBMITEDITOR_ID)),
    m_core(0),
    m_commandLocator(0),
    m_showAction(0),
    m_submitCurrentAction(0),
    m_diffSelectedFilesAction(0),
    m_undoAction(0),
    m_redoAction(0),
    m_menuAction(0),
    m_applyCurrentFilePatchAction(0),
    m_gitClient(0),
    m_changeSelectionDialog(0),
    m_submitActionTriggered(false)
{
    m_instance = this;
    const int mid = qRegisterMetaType<GitClientMemberFunc>();
    Q_UNUSED(mid)
    m_fileActions.reserve(10);
    m_projectActions.reserve(10);
    m_repositoryActions.reserve(15);
}

GitPlugin::~GitPlugin()
{
    cleanCommitMessageFile();
    delete m_gitClient;
    m_instance = 0;
}

void GitPlugin::cleanCommitMessageFile()
{
    if (!m_commitMessageFileName.isEmpty()) {
        QFile::remove(m_commitMessageFileName);
        m_commitMessageFileName.clear();
    }
}

bool GitPlugin::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

GitPlugin *GitPlugin::instance()
{
    return m_instance;
}

static const VCSBase::VCSBaseSubmitEditorParameters submitParameters = {
    Git::Constants::SUBMIT_MIMETYPE,
    Git::Constants::GITSUBMITEDITOR_ID,
    Git::Constants::GITSUBMITEDITOR_DISPLAY_NAME,
    Git::Constants::C_GITSUBMITEDITOR
};

static Core::Command *createSeparator(Core::ActionManager *am,
                                       const Core::Context &context,
                                       const Core::Id &id,
                                       QObject *parent)
{
    QAction *a = new QAction(parent);
    a->setSeparator(true);
    return am->registerAction(a, id, context);
}

// Create a parameter action
ParameterActionCommandPair
        GitPlugin::createParameterAction(Core::ActionManager *am, Core::ActionContainer *ac,
                                         const QString &defaultText, const QString &parameterText,
                                         const QString &id, const Core::Context &context,
                                         bool addToLocator)
{
    Utils::ParameterAction *action = new Utils::ParameterAction(defaultText, parameterText,
                                                                Utils::ParameterAction::EnabledWithParameter,
                                                                this);
    Core::Command *command = am->registerAction(action, id, context);
    command->setAttribute(Core::Command::CA_UpdateText);
    ac->addAction(command);
    if (addToLocator)
        m_commandLocator->appendCommand(command);
    return ParameterActionCommandPair(action, command);
}

// Create an action to act on a file with a slot.
ParameterActionCommandPair
        GitPlugin::createFileAction(Core::ActionManager *am, Core::ActionContainer *ac,
                                    const QString &defaultText, const QString &parameterText,
                                    const QString &id, const Core::Context &context, bool addToLocator,
                                    const char *pluginSlot)
{
    const ParameterActionCommandPair rc = createParameterAction(am, ac, defaultText, parameterText, id, context, addToLocator);
    m_fileActions.push_back(rc.first);
    connect(rc.first, SIGNAL(triggered()), this, pluginSlot);
    return rc;
}

// Create an action to act on a project with slot.
ParameterActionCommandPair
        GitPlugin::createProjectAction(Core::ActionManager *am, Core::ActionContainer *ac,
                                       const QString &defaultText, const QString &parameterText,
                                       const QString &id, const Core::Context &context, bool addToLocator,
                                       const char *pluginSlot)
{
    const ParameterActionCommandPair rc = createParameterAction(am, ac, defaultText, parameterText, id, context, addToLocator);
    m_projectActions.push_back(rc.first);
    connect(rc.first, SIGNAL(triggered()), this, pluginSlot);
    return rc;
}

// Create an action to act on the repository
ActionCommandPair
        GitPlugin::createRepositoryAction(Core::ActionManager *am, Core::ActionContainer *ac,
                                          const QString &text, const QString &id,
                                          const Core::Context &context, bool addToLocator)
{
    QAction  *action = new QAction(text, this);
    Core::Command *command = am->registerAction(action, id, context);
    ac->addAction(command);
    m_repositoryActions.push_back(action);
    if (addToLocator)
        m_commandLocator->appendCommand(command);
    return ActionCommandPair(action, command);
}

// Create an action to act on the repository with slot
ActionCommandPair
        GitPlugin::createRepositoryAction(Core::ActionManager *am, Core::ActionContainer *ac,
                                          const QString &text, const QString &id,
                                          const Core::Context &context, bool addToLocator,
                                          const char *pluginSlot)
{
    const ActionCommandPair rc = createRepositoryAction(am, ac, text, id, context, addToLocator);
    connect(rc.first, SIGNAL(triggered()), this, pluginSlot);
    return rc;
}

// Action to act on the repository forwarded to a git client member function
// taking the directory. Store the member function as data on the action.
ActionCommandPair
        GitPlugin::createRepositoryAction(Core::ActionManager *am, Core::ActionContainer *ac,
                                          const QString &text, const QString &id,
                                          const Core::Context &context, bool addToLocator,
                                          GitClientMemberFunc func)
{
    // Set the member func as data and connect to generic slot
    const ActionCommandPair rc = createRepositoryAction(am, ac, text, id, context, addToLocator);
    rc.first->setData(qVariantFromValue(func));
    connect(rc.first, SIGNAL(triggered()), this, SLOT(gitClientMemberFuncRepositoryAction()));
    return rc;
}

bool GitPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    typedef VCSBase::VCSEditorFactory<GitEditor> GitEditorFactory;
    typedef VCSBase::VCSSubmitEditorFactory<GitSubmitEditor> GitSubmitEditorFactory;

    VCSBase::VCSBasePlugin::initialize(new GitVersionControl(m_gitClient));

    m_core = Core::ICore::instance();
    m_gitClient = new GitClient(this);
    // Create the globalcontext list to register actions accordingly
    Core::Context globalcontext(Core::Constants::C_GLOBAL);

    // Create the settings Page
    addAutoReleasedObject(new SettingsPage());

    static const char *describeSlot = SLOT(show(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new GitEditorFactory(editorParameters + i, m_gitClient, describeSlot));

    addAutoReleasedObject(new GitSubmitEditorFactory(&submitParameters));
    addAutoReleasedObject(new CloneWizard);
    addAutoReleasedObject(new Gitorious::Internal::GitoriousCloneWizard);

    const QString description = QLatin1String("Git");
    const QString prefix = QLatin1String("git");
    m_commandLocator = new Locator::CommandLocator(description, prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    //register actions
    Core::ActionManager *actionManager = m_core->actionManager();

    Core::ActionContainer *toolsContainer =
        actionManager->actionContainer(Core::Constants::M_TOOLS);

    Core::ActionContainer *gitContainer =
        actionManager->createMenu(description);
    gitContainer->menu()->setTitle(tr("&Git"));
    toolsContainer->addMenu(gitContainer);
    m_menuAction = gitContainer->menu()->menuAction();

    ParameterActionCommandPair parameterActionCommand
            = createFileAction(actionManager, gitContainer,
                               tr("Blame"), tr("Blame for \"%1\""),
                               QLatin1String("Git.Blame"),
                               globalcontext, true, SLOT(blameFile()));
    parameterActionCommand.second->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+B")));

    parameterActionCommand
            = createFileAction(actionManager, gitContainer,
                               tr("Diff Current File"), tr("Diff of \"%1\""),
                               QLatin1String("Git.Diff"), globalcontext, true,
                               SLOT(diffCurrentFile()));
    parameterActionCommand.second->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+D")));

    parameterActionCommand
            = createFileAction(actionManager, gitContainer,
                               tr("Log Current File"), tr("Log of \"%1\""),
                               QLatin1String("Git.Log"), globalcontext, true, SLOT(logFile()));
    parameterActionCommand.second->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+L")));


    // ------
    gitContainer->addAction(createSeparator(actionManager, globalcontext, Core::Id("Git.Sep.File"), this));

    parameterActionCommand
            = createFileAction(actionManager, gitContainer,
                               tr("Stage File for Commit"), tr("Stage \"%1\" for Commit"),
                               QLatin1String("Git.Stage"), globalcontext, true, SLOT(stageFile()));
    parameterActionCommand.second->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+A")));

    parameterActionCommand
            = createFileAction(actionManager, gitContainer,
                               tr("Unstage File from Commit"), tr("Unstage \"%1\" from Commit"),
                               QLatin1String("Git.Unstage"), globalcontext, true, SLOT(unstageFile()));

    parameterActionCommand
            = createFileAction(actionManager, gitContainer,
                               tr("Undo Unstaged Changes"), tr("Undo Unstaged Changes for \"%1\""),
                               QLatin1String("Git.UndoUnstaged"), globalcontext,
                               true, SLOT(undoUnstagedFileChanges()));

    parameterActionCommand
            = createFileAction(actionManager, gitContainer,
                               tr("Undo Uncommitted Changes"), tr("Undo Uncommitted Changes for \"%1\""),
                               QLatin1String("Git.Undo"), globalcontext,
                               true, SLOT(undoFileChanges()));
    parameterActionCommand.second->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+U")));


    // ------------
    gitContainer->addAction(createSeparator(actionManager, globalcontext, Core::Id("Git.Sep.Project"), this));

    parameterActionCommand
            = createProjectAction(actionManager, gitContainer,
                                  tr("Diff Current Project"), tr("Diff Project \"%1\""),
                                  QLatin1String("Git.DiffProject"),
                                  globalcontext, true,
                                  SLOT(diffCurrentProject()));
    parameterActionCommand.second->setDefaultKeySequence(QKeySequence("Alt+G,Alt+Shift+D"));

    parameterActionCommand
            = createProjectAction(actionManager, gitContainer,
                                  tr("Log Project"), tr("Log Project \"%1\""),
                                  QLatin1String("Git.LogProject"), globalcontext, true,
                                  SLOT(logProject()));
    parameterActionCommand.second->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+K")));

    parameterActionCommand
                = createProjectAction(actionManager, gitContainer,
                                      tr("Clean Project..."), tr("Clean Project \"%1\"..."),
                                      QLatin1String("Git.CleanProject"), globalcontext,
                                      true, SLOT(cleanProject()));


    // --------------
    gitContainer->addAction(createSeparator(actionManager, globalcontext, Core::Id("Git.Sep.Repository"), this));

    createRepositoryAction(actionManager, gitContainer,
                           tr("Diff"), QLatin1String("Git.DiffRepository"),
                           globalcontext, true, SLOT(diffRepository()));

    createRepositoryAction(actionManager, gitContainer,
                           tr("Log"), QLatin1String("Git.LogRepository"),
                           globalcontext, true, &GitClient::graphLog);

    createRepositoryAction(actionManager, gitContainer,
                           tr("Status"), QLatin1String("Git.StatusRepository"),
                           globalcontext, true, &GitClient::status);

    createRepositoryAction(actionManager, gitContainer,
                           tr("Undo Uncommited Changes..."), QLatin1String("Git.UndoRepository"),
                           globalcontext, false, SLOT(undoRepositoryChanges()));


    createRepositoryAction(actionManager, gitContainer,
                           tr("Clean..."), QLatin1String("Git.CleanRepository"),
                           globalcontext, true, SLOT(cleanRepository()));

    m_createRepositoryAction = new QAction(tr("Create Repository..."), this);
    Core::Command *createRepositoryCommand = actionManager->registerAction(m_createRepositoryAction, "Git.CreateRepository", globalcontext);
    connect(m_createRepositoryAction, SIGNAL(triggered()), this, SLOT(createRepository()));
    gitContainer->addAction(createRepositoryCommand);

    // --------------
    gitContainer->addAction(createSeparator(actionManager, globalcontext, Core::Id("Git.Sep.Info"), this));

    createRepositoryAction(actionManager, gitContainer,
                           tr("Launch gitk"), QLatin1String("Git.LaunchGitK"),
                           globalcontext, true, &GitClient::launchGitK);

    createRepositoryAction(actionManager, gitContainer,
                           tr("Branches..."), QLatin1String("Git.BranchList"),
                           globalcontext, false, SLOT(branchList()));

    m_showAction = new QAction(tr("Show Commit..."), this);
    Core::Command *showCommitCommand = actionManager->registerAction(m_showAction, "Git.ShowCommit", globalcontext);
    connect(m_showAction, SIGNAL(triggered()), this, SLOT(showCommit()));
    gitContainer->addAction(showCommitCommand);


    // --------------
    gitContainer->addAction(createSeparator(actionManager, globalcontext, Core::Id("Git.Sep.RarelyUsed"), this));

    Core::ActionContainer *patchMenu = actionManager->createMenu(Core::Id("Git.PatchMenu"));
    patchMenu->menu()->setTitle(tr("Patch"));
    gitContainer->addMenu(patchMenu);

    // Apply current file as patch is handled specially.
    parameterActionCommand =
            createParameterAction(actionManager, patchMenu,
                                  tr("Apply from Editor"), tr("Apply \"%1\""),
                                  QLatin1String("Git.ApplyCurrentFilePatch"),
                                  globalcontext, true);
    m_applyCurrentFilePatchAction = parameterActionCommand.first;
    connect(m_applyCurrentFilePatchAction, SIGNAL(triggered()), this,
            SLOT(applyCurrentFilePatch()));

    createRepositoryAction(actionManager, patchMenu,
                           tr("Apply from File..."), QLatin1String("Git.ApplyPatch"),
                           globalcontext, true, SLOT(promptApplyPatch()));

    Core::ActionContainer *stashMenu = actionManager->createMenu(Core::Id("Git.StashMenu"));
    stashMenu->menu()->setTitle(tr("Stash"));
    gitContainer->addMenu(stashMenu);

    createRepositoryAction(actionManager, stashMenu,
                           tr("Stashes..."), QLatin1String("Git.StashList"),
                           globalcontext, false, SLOT(stashList()));

    stashMenu->addAction(createSeparator(actionManager, globalcontext, Core::Id("Git.Sep.StashMenuPush"), this));

    ActionCommandPair actionCommand =
            createRepositoryAction(actionManager, stashMenu,
                                   tr("Stash"), QLatin1String("Git.Stash"),
                                   globalcontext, true, SLOT(stash()));
    actionCommand.first->setToolTip(tr("Saves the current state of your work and resets the repository."));

    actionCommand = createRepositoryAction(actionManager, stashMenu,
                                           tr("Take Snapshot..."), QLatin1String("Git.StashSnapshot"),
                                           globalcontext, true, SLOT(stashSnapshot()));
    actionCommand.first->setToolTip(tr("Saves the current state of your work."));

    stashMenu->addAction(createSeparator(actionManager, globalcontext, Core::Id("Git.Sep.StashMenuPop"), this));

    actionCommand = createRepositoryAction(actionManager, stashMenu,
                                           tr("Stash Pop"), QLatin1String("Git.StashPop"),
                                           globalcontext, true, &GitClient::stashPop);
    actionCommand.first->setToolTip(tr("Restores changes saved to the stash list using \"Stash\"."));

    Core::ActionContainer *subversionMenu = actionManager->createMenu(Core::Id("Git.Subversion"));
    subversionMenu->menu()->setTitle(tr("Subversion"));
    gitContainer->addMenu(subversionMenu);

    createRepositoryAction(actionManager, subversionMenu,
                           tr("Log"), QLatin1String("Git.Subversion.Log"),
                           globalcontext, false, &GitClient::subversionLog);

    createRepositoryAction(actionManager, subversionMenu,
                           tr("Fetch"), QLatin1String("Git.Subversion.Fetch"),
                           globalcontext, false, &GitClient::synchronousSubversionFetch);

    gitContainer->addAction(createSeparator(actionManager, globalcontext, Core::Id("Git.Sep.PushPull"), this));

    gitContainer->addAction(createSeparator(actionManager, globalcontext, Core::Id("Git.Sep.Global"), this));

    createRepositoryAction(actionManager, gitContainer,
                           tr("Fetch"), QLatin1String("Git.Fetch"),
                           globalcontext, true, SLOT(fetch()));

    createRepositoryAction(actionManager, gitContainer,
                           tr("Pull"), QLatin1String("Git.Pull"),
                           globalcontext, true, SLOT(pull()));

    actionCommand = createRepositoryAction(actionManager, gitContainer,
                                           tr("Push"), QLatin1String("Git.Push"),
                                           globalcontext, true, SLOT(push()));

    actionCommand = createRepositoryAction(actionManager, gitContainer,
                                           tr("Commit..."), QLatin1String("Git.Commit"),
                                           globalcontext, true, SLOT(startCommit()));
    actionCommand.second->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+C")));

    createRepositoryAction(actionManager, gitContainer,
                           tr("Amend Last Commit..."), QLatin1String("Git.AmendCommit"),
                           globalcontext, true, SLOT(startAmendCommit()));

    // Subversion in a submenu.
    gitContainer->addAction(createSeparator(actionManager, globalcontext, Core::Id("Git.Sep.Subversion"), this));

    if (0) {
        const QList<QAction*> snapShotActions = createSnapShotTestActions();
        const int count = snapShotActions.size();
        for (int i = 0; i < count; i++) {
            Core::Command *tCommand
                    = actionManager->registerAction(snapShotActions.at(i),
                                                    QString(QLatin1String("Git.Snapshot.") + QString::number(i)),
                                                    globalcontext);
            gitContainer->addAction(tCommand);
        }
    }

    // Submit editor
    Core::Context submitContext(Constants::C_GITSUBMITEDITOR);
    m_submitCurrentAction = new QAction(VCSBase::VCSBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    Core::Command *command = actionManager->registerAction(m_submitCurrentAction, Constants::SUBMIT_CURRENT, submitContext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_submitCurrentAction, SIGNAL(triggered()), this, SLOT(submitCurrentLog()));

    m_diffSelectedFilesAction = new QAction(VCSBase::VCSBaseSubmitEditor::diffIcon(), tr("Diff Selected Files"), this);
    command = actionManager->registerAction(m_diffSelectedFilesAction, Constants::DIFF_SELECTED, submitContext);

    m_undoAction = new QAction(tr("&Undo"), this);
    command = actionManager->registerAction(m_undoAction, Core::Constants::UNDO, submitContext);

    m_redoAction = new QAction(tr("&Redo"), this);
    command = actionManager->registerAction(m_redoAction, Core::Constants::REDO, submitContext);

    return true;
}

GitVersionControl *GitPlugin::gitVersionControl() const
{
    return static_cast<GitVersionControl *>(versionControl());
}

void GitPlugin::submitEditorDiff(const QStringList &unstaged, const QStringList &staged)
{
    m_gitClient->diff(m_submitRepository, QStringList(), unstaged, staged);
}

void GitPlugin::diffCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    m_gitClient->diff(state.currentFileTopLevel(), QStringList(), state.relativeCurrentFile());
}

void GitPlugin::diffCurrentProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    m_gitClient->diff(state.currentProjectTopLevel(), QStringList(), state.relativeCurrentProject());
}

void GitPlugin::diffRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    m_gitClient->diff(state.topLevel(), QStringList(), QStringList());
}

void GitPlugin::logFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    m_gitClient->log(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()), true);
}

void GitPlugin::blameFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    const int lineNumber = VCSBase::VCSBaseEditor::lineNumberOfCurrentEditor(state.currentFile());
    m_gitClient->blame(state.currentFileTopLevel(), QStringList(), state.relativeCurrentFile(), QString(), lineNumber);
}

void GitPlugin::logProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    m_gitClient->log(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void GitPlugin::undoFileChanges(bool revertStaging)
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    Core::FileChangeBlocker fcb(state.currentFile());
    m_gitClient->revert(QStringList(state.currentFile()), revertStaging);
}

void GitPlugin::undoUnstagedFileChanges()
{
    undoFileChanges(false);
}

void GitPlugin::undoRepositoryChanges()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    const QString msg = tr("Undo all pending changes to the repository\n%1?").arg(QDir::toNativeSeparators(state.topLevel()));
    const QMessageBox::StandardButton answer
            = QMessageBox::question(m_core->mainWindow(),
                                    tr("Undo Changes"), msg,
                                    QMessageBox::Yes|QMessageBox::No,
                                    QMessageBox::No);
    if (answer == QMessageBox::No)
        return;
    m_gitClient->hardReset(state.topLevel(), QString());
}

void GitPlugin::stageFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    m_gitClient->addFile(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void GitPlugin::unstageFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    m_gitClient->synchronousReset(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void GitPlugin::startAmendCommit()
{
    startCommit(true);
}

void GitPlugin::startCommit()
{
    startCommit(false);
}

void GitPlugin::startCommit(bool amend)
{

    if (VCSBase::VCSBaseSubmitEditor::raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VCSBase::VCSBaseOutputWindow::instance()->appendWarning(tr("Another submit is currently being executed."));
        return;
    }

    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    QString errorMessage, commitTemplate;
    CommitData data;
    if (!m_gitClient->getCommitData(state.topLevel(), amend, &commitTemplate, &data, &errorMessage)) {
        VCSBase::VCSBaseOutputWindow::instance()->append(errorMessage);
        return;
    }

    // Store repository for diff and the original list of
    // files to be able to unstage files the user unchecks
    m_submitRepository = data.panelInfo.repository;
    m_commitAmendSHA1 = data.amendSHA1;
    m_submitOrigCommitFiles = data.stagedFileNames();
    m_submitOrigDeleteFiles = data.stagedFileNames("deleted");

    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << data << commitTemplate;

    // Start new temp file with message template
    QTemporaryFile changeTmpFile;
    changeTmpFile.setAutoRemove(false);
    if (!changeTmpFile.open()) {
        VCSBase::VCSBaseOutputWindow::instance()->append(tr("Cannot create temporary file: %1").arg(changeTmpFile.errorString()));
        return;
    }
    m_commitMessageFileName = changeTmpFile.fileName();
    changeTmpFile.write(commitTemplate.toLocal8Bit());
    changeTmpFile.flush();
    // Keep the file alive, else it removes self and forgets
    // its name
    changeTmpFile.close();
    openSubmitEditor(m_commitMessageFileName, data, amend);
}

Core::IEditor *GitPlugin::openSubmitEditor(const QString &fileName, const CommitData &cd, bool amend)
{
    Core::IEditor *editor = m_core->editorManager()->openEditor(fileName, QLatin1String(Constants::GITSUBMITEDITOR_ID),
                                                                Core::EditorManager::ModeSwitch);
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << fileName << editor;
    GitSubmitEditor *submitEditor = qobject_cast<GitSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return 0);
    // The actions are for some reason enabled by the context switching
    // mechanism. Disable them correctly.
    submitEditor->registerActions(m_undoAction, m_redoAction, m_submitCurrentAction, m_diffSelectedFilesAction);
    submitEditor->setCommitData(cd);
    submitEditor->setCheckScriptWorkingDirectory(m_submitRepository);
    const QString title = amend ? tr("Amend %1").arg(cd.amendSHA1) : tr("Git Commit");
    submitEditor->setDisplayName(title);
    if (amend) // Allow for just correcting the message
        submitEditor->setEmptyFileListEnabled(true);
    connect(submitEditor, SIGNAL(diff(QStringList,QStringList)), this, SLOT(submitEditorDiff(QStringList,QStringList)));
    return editor;
}

void GitPlugin::submitCurrentLog()
{
    // Close the submit editor
    m_submitActionTriggered = true;
    QList<Core::IEditor*> editors;
    editors.push_back(m_core->editorManager()->currentEditor());
    m_core->editorManager()->closeEditors(editors);
}

bool GitPlugin::submitEditorAboutToClose(VCSBase::VCSBaseSubmitEditor *submitEditor)
{
    if (!isCommitEditorOpen())
        return false;
    Core::IFile *fileIFace = submitEditor->file();
    const GitSubmitEditor *editor = qobject_cast<GitSubmitEditor *>(submitEditor);
    if (!fileIFace || !editor)
        return true;
    // Submit editor closing. Make it write out the commit message
    // and retrieve files
    const QFileInfo editorFile(fileIFace->fileName());
    const QFileInfo changeFile(m_commitMessageFileName);
    // Paranoia!
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true;
    // Prompt user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    GitSettings settings = m_gitClient->settings();
    const bool wantedPrompt = settings.promptToSubmit;
    const VCSBase::VCSBaseSubmitEditor::PromptSubmitResult answer =
            editor->promptSubmit(tr("Closing Git Editor"),
                                 tr("Do you want to commit the change?"),
                                 tr("Git will not accept this commit. Do you want to continue to edit it?"),
                                 &settings.promptToSubmit, !m_submitActionTriggered, false);
    m_submitActionTriggered = false;
    switch (answer) {
    case VCSBase::VCSBaseSubmitEditor::SubmitCanceled:
        return false; // Keep editing and change file
    case VCSBase::VCSBaseSubmitEditor::SubmitDiscarded:
        cleanCommitMessageFile();
        return true; // Cancel all
    default:
        break;
    }
    if (wantedPrompt != settings.promptToSubmit)
        m_gitClient->setSettings(settings);
    // Go ahead!
    const QStringList fileList = editor->checkedFiles();
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << fileList;
    bool closeEditor = true;
    if (!fileList.empty() || !m_commitAmendSHA1.isEmpty()) {
        // get message & commit
        m_core->fileManager()->blockFileChange(fileIFace);
        fileIFace->save();
        m_core->fileManager()->unblockFileChange(fileIFace);

        closeEditor = m_gitClient->addAndCommit(m_submitRepository,
                                                editor->panelData(),
                                                m_commitAmendSHA1,
                                                m_commitMessageFileName,
                                                fileList,
                                                m_submitOrigCommitFiles,
                                                m_submitOrigDeleteFiles);
    }
    if (closeEditor)
        cleanCommitMessageFile();
    return closeEditor;
}

void GitPlugin::fetch()
{
    m_gitClient->synchronousFetch(currentState().topLevel());
}

void GitPlugin::pull()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)

    switch (m_gitClient->ensureStash(state.topLevel())) {
        case GitClient::StashUnchanged:
        case GitClient::Stashed:
        case GitClient::NotStashed:
            m_gitClient->synchronousPull(state.topLevel());
        default:
        break;
    }
}

void GitPlugin::push()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    m_gitClient->synchronousPush(state.topLevel());
}

// Retrieve member function of git client stored as user data of action
static inline GitClientMemberFunc memberFunctionFromAction(const QObject *o)
{
    if (o) {
        if (const QAction *action = qobject_cast<const QAction *>(o)) {
            const QVariant v = action->data();
            if (qVariantCanConvert<GitClientMemberFunc>(v))
                return qVariantValue<GitClientMemberFunc>(v);
        }
    }
    return 0;
}

void GitPlugin::gitClientMemberFuncRepositoryAction()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    // Retrieve member function and invoke on repository
    GitClientMemberFunc func = memberFunctionFromAction(sender());
    QTC_ASSERT(func, return);
    (m_gitClient->*func)(state.topLevel());
}

void GitPlugin::cleanProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    cleanRepository(state.currentProjectPath());
}

void GitPlugin::cleanRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    cleanRepository(state.topLevel());
}

void GitPlugin::cleanRepository(const QString &directory)
{
    // Find files to be deleted
    QString errorMessage;
    QStringList files;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool gotFiles = m_gitClient->synchronousCleanList(directory, &files, &errorMessage);
    QApplication::restoreOverrideCursor();

    QWidget *parent = Core::ICore::instance()->mainWindow();
    if (!gotFiles) {
        QMessageBox::warning(parent, tr("Unable to retrieve file list"),
                             errorMessage);
        return;
    }
    if (files.isEmpty()) {
        QMessageBox::information(parent, tr("Repository Clean"),
                                 tr("The repository is clean."));
        return;
    }
    // Clean the trailing slash of directories
    const QChar slash = QLatin1Char('/');
    const QStringList::iterator end = files.end();
    for (QStringList::iterator it = files.begin(); it != end; ++it)
        if (it->endsWith(slash))
            it->truncate(it->size() - 1);

    // Show in dialog
    VCSBase::CleanDialog dialog(parent);
    dialog.setFileList(directory, files);
    dialog.exec();
}

// If the file is modified in an editor, make sure it is saved.
static bool ensureFileSaved(const QString &fileName)
{
    const QList<Core::IEditor*> editors = Core::EditorManager::instance()->editorsForFileName(fileName);
    if (editors.isEmpty())
        return true;
    Core::IFile *file = editors.front()->file();
    if (!file || !file->isModified())
        return true;
    Core::FileManager *fm = Core::ICore::instance()->fileManager();
    bool canceled;
    QList<Core::IFile *> files;
    files << file;
    fm->saveModifiedFiles(files, &canceled);
    return !canceled;
}

void GitPlugin::applyCurrentFilePatch()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasPatchFile() && state.hasTopLevel(), return);
    const QString patchFile = state.currentPatchFile();
    if (!ensureFileSaved(patchFile))
        return;
    applyPatch(state.topLevel(), patchFile);
}

void GitPlugin::promptApplyPatch()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    applyPatch(state.topLevel(), QString());
}

void GitPlugin::applyPatch(const QString &workingDirectory, QString file)
{
    // Ensure user has been notified about pending changes
    switch (m_gitClient->ensureStash(workingDirectory)) {
    case GitClient::StashUnchanged:
    case GitClient::Stashed:
    case GitClient::NotStashed:
        break;
    default:
        return;
    }
    // Prompt for file
    if (file.isEmpty()) {
        const QString filter = tr("Patches (*.patch *.diff)");
        file =  QFileDialog::getOpenFileName(Core::ICore::instance()->mainWindow(),
                                             tr("Choose Patch"),
                                             QString(), filter);
        if (file.isEmpty())
            return;
    }
    // Run!
    VCSBase::VCSBaseOutputWindow *outwin = VCSBase::VCSBaseOutputWindow::instance();
    QString errorMessage;
    if (m_gitClient->synchronousApplyPatch(workingDirectory, file, &errorMessage)) {
        if (errorMessage.isEmpty()) {
            outwin->append(tr("Patch %1 successfully applied to %2").arg(file, workingDirectory));
        } else {
            outwin->append(errorMessage);
        }
    } else {
        outwin->appendError(errorMessage);
    }
}

void GitPlugin::stash()
{
    // Simple stash without prompt, reset repo.
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    const QString id = m_gitClient->synchronousStash(state.topLevel(), QString(), 0);
    if (!id.isEmpty() && m_stashDialog)
        m_stashDialog->refresh(state.topLevel(), true);
}

void GitPlugin::stashSnapshot()
{
    // Prompt for description, restore immediately and keep on working.
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    const QString id = m_gitClient->synchronousStash(state.topLevel(), QString(), GitClient::StashImmediateRestore|GitClient::StashPromptDescription);
    if (!id.isEmpty() && m_stashDialog)
        m_stashDialog->refresh(state.topLevel(), true);
}

// Create a non-modal dialog with refresh method or raise if it exists
template <class NonModalDialog>
    inline void showNonModalDialog(const QString &topLevel,
                                   QPointer<NonModalDialog> &dialog)
{
    if (dialog) {
        dialog->show();
        dialog->raise();
    } else {
        dialog = new NonModalDialog(Core::ICore::instance()->mainWindow());
        dialog->refresh(topLevel, true);
        dialog->show();
    }
}

void GitPlugin::branchList()
{
   showNonModalDialog(currentState().topLevel(), m_branchDialog);
}

void GitPlugin::stashList()
{
    showNonModalDialog(currentState().topLevel(), m_stashDialog);
}

void GitPlugin::updateActions(VCSBase::VCSBasePlugin::ActionState as)
{
    const bool repositoryEnabled = currentState().hasTopLevel();
    if (m_stashDialog)
        m_stashDialog->refresh(currentState().topLevel(), false);
    if (m_branchDialog)
        m_branchDialog->refresh(currentState().topLevel(), false);

    m_commandLocator->setEnabled(repositoryEnabled);
    if (!enableMenuAction(as, m_menuAction))
        return;
    // Note: This menu is visible if there is no repository. Only
    // 'Create Repository'/'Show' actions should be available.
    const QString fileName = currentState().currentFileName();
    foreach (Utils::ParameterAction *fileAction, m_fileActions)
        fileAction->setParameter(fileName);
    // If the current file looks like a patch, offer to apply
    m_applyCurrentFilePatchAction->setParameter(currentState().currentPatchFileDisplayName());

    const QString projectName = currentState().currentProjectName();
    foreach (Utils::ParameterAction *projectAction, m_projectActions)
        projectAction->setParameter(projectName);

    foreach (QAction *repositoryAction, m_repositoryActions)
        repositoryAction->setEnabled(repositoryEnabled);

    // Prompts for repo.
    m_showAction->setEnabled(true);
}

void GitPlugin::showCommit()
{
    const VCSBase::VCSBasePluginState state = currentState();

    if (!m_changeSelectionDialog)
        m_changeSelectionDialog = new ChangeSelectionDialog();

    if (state.hasTopLevel())
        m_changeSelectionDialog->setRepository(state.topLevel());

    if (m_changeSelectionDialog->exec() != QDialog::Accepted)
        return;
    const QString change = m_changeSelectionDialog->change();
    if (change.isEmpty())
        return;

    m_gitClient->show(m_changeSelectionDialog->repository(), change);
}

GitSettings GitPlugin::settings() const
{
    return m_gitClient->settings();
}

void GitPlugin::setSettings(const GitSettings &s)
{
    m_gitClient->setSettings(s);
}

GitClient *GitPlugin::gitClient() const
{
    return m_gitClient;
}

Q_EXPORT_PLUGIN(GitPlugin)
