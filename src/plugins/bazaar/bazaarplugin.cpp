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

#include "bazaarplugin.h"
#include "constants.h"
#include "bazaarclient.h"
#include "bazaarcontrol.h"
#include "optionspage.h"
#include "bazaarcommitwidget.h"
#include "bazaareditor.h"
#include "pullorpushdialog.h"
#include "commiteditor.h"
#include "clonewizard.h"

#include "ui_revertdialog.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <locator/commandlocator.h>

#include <utils/parameteraction.h>
#include <utils/qtcassert.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbasesubmiteditor.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseoutputwindow.h>

#include <QtCore/QtPlugin>
#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QMainWindow>
#include <QtCore/QtDebug>
#include <QtCore/QtGlobal>
#include <QtCore/QDir>
#include <QtGui/QDialog>
#include <QtGui/QFileDialog>
#include <QtCore/QTemporaryFile>


using namespace Bazaar::Internal;
using namespace Bazaar;

static const VCSBase::VCSBaseEditorParameters editorParameters[] = {
    {
        VCSBase::RegularCommandOutput, //type
        Constants::COMMANDLOG_ID, // id
        Constants::COMMANDLOG_DISPLAY_NAME, // display name
        Constants::COMMANDLOG, // context
        Constants::COMMANDAPP, // mime type
        Constants::COMMANDEXT}, //extension

    {   VCSBase::LogOutput,
        Constants::FILELOG_ID,
        Constants::FILELOG_DISPLAY_NAME,
        Constants::FILELOG,
        Constants::LOGAPP,
        Constants::LOGEXT},

    {    VCSBase::AnnotateOutput,
         Constants::ANNOTATELOG_ID,
         Constants::ANNOTATELOG_DISPLAY_NAME,
         Constants::ANNOTATELOG,
         Constants::ANNOTATEAPP,
         Constants::ANNOTATEEXT},

    {   VCSBase::DiffOutput,
        Constants::DIFFLOG_ID,
        Constants::DIFFLOG_DISPLAY_NAME,
        Constants::DIFFLOG,
        Constants::DIFFAPP,
        Constants::DIFFEXT}
};

static const VCSBase::VCSBaseSubmitEditorParameters submitEditorParameters = {
    Constants::COMMITMIMETYPE,
    Constants::COMMIT_ID,
    Constants::COMMIT_DISPLAY_NAME,
    Constants::COMMIT_ID
};


BazaarPlugin *BazaarPlugin::m_instance = 0;

BazaarPlugin::BazaarPlugin()
    : VCSBase::VCSBasePlugin(QLatin1String(Constants::COMMIT_ID)),
      m_optionsPage(0),
      m_client(0),
      m_core(0),
      m_commandLocator(0),
      m_changeLog(0),
      m_addAction(0),
      m_deleteAction(0),
      m_menuAction(0)
{
    m_instance = this;
}

BazaarPlugin::~BazaarPlugin()
{
    if (m_client) {
        delete m_client;
        m_client = 0;
    }

    deleteCommitLog();

    m_instance = 0;
}

bool BazaarPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    typedef VCSBase::VCSEditorFactory<BazaarEditor> BazaarEditorFactory;

    m_client = new BazaarClient(&m_bazaarSettings);
    initializeVcs(new BazaarControl(m_client));

    m_core = Core::ICore::instance();
    m_actionManager = m_core->actionManager();

    m_optionsPage = new OptionsPage();
    addAutoReleasedObject(m_optionsPage);
    m_bazaarSettings.readSettings(m_core->settings());

    connect(m_client, SIGNAL(changed(QVariant)), versionControl(), SLOT(changed(QVariant)));

    static const char *describeSlot = SLOT(view(QString,QString));
    const int editorCount = sizeof(editorParameters) / sizeof(VCSBase::VCSBaseEditorParameters);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new BazaarEditorFactory(editorParameters + i, m_client, describeSlot));

    addAutoReleasedObject(new VCSBase::VCSSubmitEditorFactory<CommitEditor>(&submitEditorParameters));

    addAutoReleasedObject(new CloneWizard);

    const QString prefix = QLatin1String("bzr");
    m_commandLocator = new Locator::CommandLocator(QLatin1String("Bazaar"), prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    createMenu();

    createSubmitEditorActions();

    return true;
}

BazaarPlugin *BazaarPlugin::instance()
{
    return m_instance;
}

BazaarClient *BazaarPlugin::client() const
{
    return m_client;
}

const BazaarSettings &BazaarPlugin::settings() const
{
    return m_bazaarSettings;
}

void BazaarPlugin::setSettings(const BazaarSettings &settings)
{
    if (settings != m_bazaarSettings) {
        const bool userIdChanged = !m_bazaarSettings.sameUserId(settings);
        m_bazaarSettings = settings;
        if (userIdChanged)
            client()->synchronousSetUserId();
        static_cast<BazaarControl *>(versionControl())->emitConfigurationChanged();
    }
}

void BazaarPlugin::createMenu()
{
    Core::Context context(Core::Constants::C_GLOBAL);

    // Create menu item for Bazaar
    m_bazaarContainer = m_actionManager->createMenu(Core::Id("Bazaar.BazaarMenu"));
    QMenu *menu = m_bazaarContainer->menu();
    menu->setTitle(tr("Bazaar"));

    createFileActions(context);
    createSeparator(context, Core::Id("Bazaar.FileDirSeperator"));
    createDirectoryActions(context);
    createSeparator(context, Core::Id("Bazaar.DirRepoSeperator"));
    createRepositoryActions(context);
    createSeparator(context, Core::Id("Bazaar.Repository Management"));

    // Request the Tools menu and add the Bazaar menu to it
    Core::ActionContainer *toolsMenu = m_actionManager->actionContainer(Core::Id(Core::Constants::M_TOOLS));
    toolsMenu->addMenu(m_bazaarContainer);
    m_menuAction = m_bazaarContainer->menu()->menuAction();
}

void BazaarPlugin::createFileActions(const Core::Context &context)
{
    Core::Command *command;

    m_annotateFile = new Utils::ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = m_actionManager->registerAction(m_annotateFile, Core::Id(Constants::ANNOTATE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_annotateFile, SIGNAL(triggered()), this, SLOT(annotateCurrentFile()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_diffFile = new Utils::ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = m_actionManager->registerAction(m_diffFile, Core::Id(Constants::DIFF), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("ALT+Z,Alt+D")));
    connect(m_diffFile, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logFile = new Utils::ParameterAction(tr("Log Current File"), tr("Log \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = m_actionManager->registerAction(m_logFile, Core::Id(Constants::LOG), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("ALT+Z,Alt+L")));
    connect(m_logFile, SIGNAL(triggered()), this, SLOT(logCurrentFile()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusFile = new Utils::ParameterAction(tr("Status Current File"), tr("Status \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = m_actionManager->registerAction(m_statusFile, Core::Id(Constants::STATUS), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("ALT+Z,Alt+S")));
    connect(m_statusFile, SIGNAL(triggered()), this, SLOT(statusCurrentFile()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    createSeparator(context, Core::Id("Bazaar.FileDirSeperator1"));

    m_addAction = new Utils::ParameterAction(tr("Add"), tr("Add \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = m_actionManager->registerAction(m_addAction, Core::Id(Constants::ADD), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_addAction, SIGNAL(triggered()), this, SLOT(addCurrentFile()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new Utils::ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), Utils::ParameterAction::EnabledWithParameter, this);
    command = m_actionManager->registerAction(m_deleteAction, Core::Id(Constants::DELETE), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(promptToDeleteCurrentFile()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertFile = new Utils::ParameterAction(tr("Revert Current File..."), tr("Revert \"%1\"..."), Utils::ParameterAction::EnabledWithParameter, this);
    command = m_actionManager->registerAction(m_revertFile, Core::Id(Constants::REVERT), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_revertFile, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}

void BazaarPlugin::addCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->synchronousAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void BazaarPlugin::annotateCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void BazaarPlugin::diffCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void BazaarPlugin::logCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->log(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()),
                  QStringList(), true);
}

void BazaarPlugin::revertCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    QDialog dialog;
    Ui::RevertDialog revertUi;
    revertUi.setupUi(&dialog);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->revertFile(state.currentFileTopLevel(),
                         state.relativeCurrentFile(),
                         revertUi.revisionLineEdit->text());
}

void BazaarPlugin::statusCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->status(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void BazaarPlugin::createDirectoryActions(const Core::Context &context)
{
    QAction *action;
    Core::Command *command;

    action = new QAction(tr("Diff"), this);
    m_repositoryActionList.append(action);
    command = m_actionManager->registerAction(action, Core::Id(Constants::DIFFMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(diffRepository()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Log"), this);
    m_repositoryActionList.append(action);
    command = m_actionManager->registerAction(action, Core::Id(Constants::LOGMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(logRepository()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Revert..."), this);
    m_repositoryActionList.append(action);
    command = m_actionManager->registerAction(action, Core::Id(Constants::REVERTMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(revertAll()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Status"), this);
    m_repositoryActionList.append(action);
    command = m_actionManager->registerAction(action, Core::Id(Constants::STATUSMULTI), context);
    connect(action, SIGNAL(triggered()), this, SLOT(statusMulti()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);
}


void BazaarPlugin::diffRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->diff(state.topLevel());
}

void BazaarPlugin::logRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    QStringList extraOptions;
    extraOptions += QString("--limit=%1").arg(settings().intValue(BazaarSettings::logCountKey));
    m_client->log(state.topLevel(), QStringList(), extraOptions);
}

void BazaarPlugin::revertAll()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QDialog dialog;
    Ui::RevertDialog revertUi;
    revertUi.setupUi(&dialog);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->revertAll(state.topLevel(), revertUi.revisionLineEdit->text());
}

void BazaarPlugin::statusMulti()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->status(state.topLevel());
}

void BazaarPlugin::createRepositoryActions(const Core::Context &context)
{
    QAction *action = 0;
    Core::Command *command = 0;

    action = new QAction(tr("Pull..."), this);
    m_repositoryActionList.append(action);
    command = m_actionManager->registerAction(action, Core::Id(Constants::PULL), context);
    connect(action, SIGNAL(triggered()), this, SLOT(pull()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Push..."), this);
    m_repositoryActionList.append(action);
    command = m_actionManager->registerAction(action, Core::Id(Constants::PUSH), context);
    connect(action, SIGNAL(triggered()), this, SLOT(push()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Update..."), this);
    m_repositoryActionList.append(action);
    command = m_actionManager->registerAction(action, Core::Id(Constants::UPDATE), context);
    connect(action, SIGNAL(triggered()), this, SLOT(update()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    action = new QAction(tr("Commit..."), this);
    m_repositoryActionList.append(action);
    command = m_actionManager->registerAction(action, Core::Id(Constants::COMMIT), context);
    command->setDefaultKeySequence(QKeySequence(tr("ALT+Z,Alt+C")));
    connect(action, SIGNAL(triggered()), this, SLOT(commit()));
    m_bazaarContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    QAction *createRepositoryAction = new QAction(tr("Create Repository..."), this);
    command = m_actionManager->registerAction(createRepositoryAction, Core::Id(Constants::CREATE_REPOSITORY), context);
    connect(createRepositoryAction, SIGNAL(triggered()), this, SLOT(createRepository()));
    m_bazaarContainer->addAction(command);
}

void BazaarPlugin::pull()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    PullOrPushDialog dialog(PullOrPushDialog::PullMode);
    if (dialog.exec() != QDialog::Accepted)
        return;
    QStringList extraOptions;
    if (dialog.isRememberOptionEnabled())
        extraOptions += QLatin1String("--remember");
    if (dialog.isOverwriteOptionEnabled())
        extraOptions += QLatin1String("--overwrite");
    if (dialog.isLocalOptionEnabled())
        extraOptions += QLatin1String("--local");
    if (!dialog.revision().isEmpty())
        extraOptions << QLatin1String("-r") << dialog.revision();
    m_client->synchronousPull(state.topLevel(), dialog.branchLocation(), extraOptions);
}

void BazaarPlugin::push()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    PullOrPushDialog dialog(PullOrPushDialog::PushMode);
    if (dialog.exec() != QDialog::Accepted)
        return;
    QStringList extraOptions;
    if (dialog.isRememberOptionEnabled())
        extraOptions += QLatin1String("--remember");
    if (dialog.isOverwriteOptionEnabled())
        extraOptions += QLatin1String("--overwrite");
    if (dialog.isUseExistingDirectoryOptionEnabled())
        extraOptions += QLatin1String("--use-existing-dir");
    if (dialog.isCreatePrefixOptionEnabled())
        extraOptions += QLatin1String("--create-prefix");
    if (!dialog.revision().isEmpty())
        extraOptions << QLatin1String("-r") << dialog.revision();
    m_client->synchronousPush(state.topLevel(), dialog.branchLocation(), extraOptions);
}

void BazaarPlugin::update()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QDialog dialog;
    Ui::RevertDialog revertUi;
    revertUi.setupUi(&dialog);
    dialog.setWindowTitle(tr("Update"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_client->update(state.topLevel(), revertUi.revisionLineEdit->text());
}

void BazaarPlugin::createSubmitEditorActions()
{
    Core::Context context(Constants::COMMIT_ID);
    Core::Command *command;

    m_editorCommit = new QAction(VCSBase::VCSBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = m_actionManager->registerAction(m_editorCommit, Core::Id(Constants::COMMIT), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_editorCommit, SIGNAL(triggered()), this, SLOT(commitFromEditor()));

    m_editorDiff = new QAction(VCSBase::VCSBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    command = m_actionManager->registerAction(m_editorDiff, Core::Id(Constants::DIFFEDITOR), context);

    m_editorUndo = new QAction(tr("&Undo"), this);
    command = m_actionManager->registerAction(m_editorUndo, Core::Id(Core::Constants::UNDO), context);

    m_editorRedo = new QAction(tr("&Redo"), this);
    command = m_actionManager->registerAction(m_editorRedo, Core::Id(Core::Constants::REDO), context);
}

void BazaarPlugin::commit()
{
    if (VCSBase::VCSBaseSubmitEditor::raiseSubmitEditor())
        return;

    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    m_submitRepository = state.topLevel();

    connect(m_client, SIGNAL(parsedStatus(QList<VCSBase::VCSBaseClient::StatusItem>)),
            this, SLOT(showCommitWidget(QList<VCSBase::VCSBaseClient::StatusItem>)));
    // The "--short" option allows to easily parse status output
    m_client->emitParsedStatus(m_submitRepository, QStringList(QLatin1String("--short")));
}

void BazaarPlugin::showCommitWidget(const QList<VCSBase::VCSBaseClient::StatusItem> &status)
{
    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();
    //Once we receive our data release the connection so it can be reused elsewhere
    disconnect(m_client, SIGNAL(parsedStatus(QList<VCSBase::VCSBaseClient::StatusItem>)),
               this, SLOT(showCommitWidget(QList<VCSBase::VCSBaseClient::StatusItem>)));

    if (status.isEmpty()) {
        outputWindow->appendError(tr("There are no changes to commit."));
        return;
    }

    deleteCommitLog();

    // Open commit log
    QString changeLogPattern = QDir::tempPath();
    if (!changeLogPattern.endsWith(QLatin1Char('/')))
        changeLogPattern += QLatin1Char('/');
    changeLogPattern += QLatin1String("qtcreator-bzr-XXXXXX.msg");
    m_changeLog = new QTemporaryFile(changeLogPattern, this);
    if (!m_changeLog->open()) {
        outputWindow->appendError(tr("Unable to generate a temporary file for the commit editor."));
        return;
    }

    Core::IEditor *editor = m_core->editorManager()->openEditor(m_changeLog->fileName(),
                                                                Constants::COMMIT_ID,
                                                                Core::EditorManager::ModeSwitch);
    if (!editor) {
        outputWindow->appendError(tr("Unable to create an editor for the commit."));
        return;
    }

    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(editor);

    if (!commitEditor) {
        outputWindow->appendError(tr("Unable to create a commit editor."));
        return;
    }

    commitEditor->registerActions(m_editorUndo, m_editorRedo, m_editorCommit, m_editorDiff);
    connect(commitEditor, SIGNAL(diffSelectedFiles(QStringList)),
            this, SLOT(diffFromEditorSelected(QStringList)));
    commitEditor->setCheckScriptWorkingDirectory(m_submitRepository);

    const QString msg = tr("Commit changes for \"%1\".").
            arg(QDir::toNativeSeparators(m_submitRepository));
    commitEditor->setDisplayName(msg);

    const BranchInfo branch = m_client->synchronousBranchQuery(m_submitRepository);
    commitEditor->setFields(m_submitRepository, branch,
                            m_bazaarSettings.stringValue(BazaarSettings::userNameKey),
                            m_bazaarSettings.stringValue(BazaarSettings::userEmailKey), status);
}

void BazaarPlugin::diffFromEditorSelected(const QStringList &files)
{
    m_client->diff(m_submitRepository, files);
}

void BazaarPlugin::commitFromEditor()
{
    if (!m_changeLog)
        return;

    //use the same functionality than if the user closes the file without completing the commit
    m_core->editorManager()->closeEditors(m_core->editorManager()->editorsForFileName(m_changeLog->fileName()));
}

bool BazaarPlugin::submitEditorAboutToClose(VCSBase::VCSBaseSubmitEditor *submitEditor)
{
    if (!m_changeLog)
        return true;
    Core::IFile *editorFile = submitEditor->file();
    const CommitEditor *commitEditor = qobject_cast<const CommitEditor *>(submitEditor);
    if (!editorFile || !commitEditor)
        return true;

    bool dummyPrompt = m_bazaarSettings.boolValue(BazaarSettings::promptOnSubmitKey);
    const VCSBase::VCSBaseSubmitEditor::PromptSubmitResult response =
            commitEditor->promptSubmit(tr("Close Commit Editor"), tr("Do you want to commit the changes?"),
                                       tr("Message check failed. Do you want to proceed?"),
                                       &dummyPrompt, dummyPrompt);

    switch (response) {
    case VCSBase::VCSBaseSubmitEditor::SubmitCanceled:
        return false;
    case VCSBase::VCSBaseSubmitEditor::SubmitDiscarded:
        deleteCommitLog();
        return true;
    default:
        break;
    }

    QStringList files = commitEditor->checkedFiles();
    if (!files.empty()) {
        //save the commit message
        if (!m_core->fileManager()->saveFile(editorFile))
            return false;

        //rewrite entries of the form 'file => newfile' to 'newfile' because
        //this would mess the commit command
        for (QStringList::iterator iFile = files.begin(); iFile != files.end(); ++iFile) {
            const QStringList parts = iFile->split(" => ", QString::SkipEmptyParts);
            if (!parts.isEmpty())
                *iFile = parts.last();
        }

        const BazaarCommitWidget *commitWidget = commitEditor->commitWidget();
        QStringList extraOptions;
        // Author
        if (!commitWidget->committer().isEmpty())
            extraOptions.append(QLatin1String("--author=") + commitWidget->committer());
        // Fixed bugs
        foreach (const QString &fix, commitWidget->fixedBugs()) {
            if (!fix.isEmpty())
                extraOptions << QLatin1String("--fixes") << fix;
        }
        // Whether local commit or not
        if (commitWidget->isLocalOptionEnabled())
            extraOptions += QLatin1String("--local");
        m_client->commit(m_submitRepository, files, editorFile->fileName(), extraOptions);
    }
    return true;
}

void BazaarPlugin::deleteCommitLog()
{
    if (m_changeLog) {
        delete m_changeLog;
        m_changeLog = 0;
    }
}

void BazaarPlugin::createSeparator(const Core::Context &context, const Core::Id &id)
{
    QAction *action = new QAction(this);
    action->setSeparator(true);
    m_bazaarContainer->addAction(m_actionManager->registerAction(action, id, context));
}

void BazaarPlugin::updateActions(VCSBase::VCSBasePlugin::ActionState as)
{
    if (!enableMenuAction(as, m_menuAction)) {
        m_commandLocator->setEnabled(false);
        return;
    }
    const QString filename = currentState().currentFileName();
    const bool repoEnabled = currentState().hasTopLevel();
    m_commandLocator->setEnabled(repoEnabled);

    m_annotateFile->setParameter(filename);
    m_diffFile->setParameter(filename);
    m_logFile->setParameter(filename);
    m_addAction->setParameter(filename);
    m_deleteAction->setParameter(filename);
    m_revertFile->setParameter(filename);
    m_statusFile->setParameter(filename);

    foreach (QAction *repoAction, m_repositoryActionList)
        repoAction->setEnabled(repoEnabled);
}

Q_EXPORT_PLUGIN(BazaarPlugin)
