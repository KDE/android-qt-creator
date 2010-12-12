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

#include "vcsbaseplugin.h"
#include "vcsbasesubmiteditor.h"
#include "vcsplugin.h"
#include "commonvcssettings.h"
#include "vcsbaseoutputwindow.h"
#include "corelistener.h"

#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/vcsmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QSharedData>
#include <QtCore/QScopedPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>
#include <QtCore/QTextCodec>

#include <QtGui/QAction>
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>
#include <QtGui/QMainWindow>

enum { debug = 0, debugRepositorySearch = 0, debugExecution = 0 };

namespace VCSBase {

namespace Internal {

// Internal state created by the state listener and
// VCSBasePluginState.

struct State {
    void clearFile();
    void clearPatchFile();
    void clearProject();
    inline void clear();

    bool equals(const State &rhs) const;

    inline bool hasFile() const     { return !currentFile.isEmpty(); }
    inline bool hasProject() const  { return !currentProjectPath.isEmpty(); }
    inline bool isEmpty() const     { return !hasFile() && !hasProject(); }

    QString currentFile;
    QString currentFileName;
    QString currentPatchFile;
    QString currentPatchFileDisplayName;

    QString currentFileDirectory;
    QString currentFileTopLevel;

    QString currentProjectPath;
    QString currentProjectName;
    QString currentProjectTopLevel;
};

void State::clearFile()
{
    currentFile.clear();
    currentFileName.clear();
    currentFileDirectory.clear();
    currentFileTopLevel.clear();
}

void State::clearPatchFile()
{
    currentPatchFile.clear();
    currentPatchFileDisplayName.clear();
}

void State::clearProject()
{
    currentProjectPath.clear();
    currentProjectName.clear();
    currentProjectTopLevel.clear();
}

void State::clear()
{
    clearFile();
    clearPatchFile();
    clearProject();
}

bool State::equals(const State &rhs) const
{
    return currentFile == rhs.currentFile
            && currentFileName == rhs.currentFileName
            && currentPatchFile == rhs.currentPatchFile
            && currentPatchFileDisplayName == rhs.currentPatchFileDisplayName
            && currentFileTopLevel == rhs.currentFileTopLevel
            && currentProjectPath == rhs.currentProjectPath
            && currentProjectName == rhs.currentProjectName
            && currentProjectTopLevel == rhs.currentProjectTopLevel;
}

QDebug operator<<(QDebug in, const State &state)
{
    QDebug nospace = in.nospace();
    nospace << "State: ";
    if (state.isEmpty()) {
        nospace << "<empty>";
    } else {
        if (state.hasFile()) {
            nospace << "File=" << state.currentFile
                    << ',' << state.currentFileTopLevel;
        } else {
            nospace << "<no file>";
        }
        nospace << '\n';
        if (state.hasProject()) {
            nospace << "       Project=" << state.currentProjectName
            << ',' << state.currentProjectPath
            << ',' << state.currentProjectTopLevel;

        } else {
            nospace << "<no project>";
        }
        nospace << '\n';
    }
    return in;
}

// StateListener: Connects to the relevant signals, tries to find version
// controls and emits signals to the plugin instances.

class StateListener : public QObject {
    Q_OBJECT
public:
    explicit StateListener(QObject *parent);

signals:
    void stateChanged(const VCSBase::Internal::State &s, Core::IVersionControl *vc);

public slots:
    void slotStateChanged();
};

StateListener::StateListener(QObject *parent) :
        QObject(parent)
{
    Core::ICore *core = Core::ICore::instance();
    connect(core->fileManager(), SIGNAL(currentFileChanged(QString)),
            this, SLOT(slotStateChanged()));
    connect(core->editorManager()->instance(), SIGNAL(currentEditorStateChanged(Core::IEditor*)),
            this, SLOT(slotStateChanged()));

    if (ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance())
        connect(pe, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
                this, SLOT(slotStateChanged()));
}

static inline QString displayNameOfEditor(const QString &fileName)
{
    const QList<Core::IEditor*> editors = Core::EditorManager::instance()->editorsForFileName(fileName);
    if (!editors.isEmpty())
        return editors.front()->displayName();
    return QString();
}

void StateListener::slotStateChanged()
{
    const ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance();
    const Core::ICore *core = Core::ICore::instance();
    Core::VcsManager *vcsManager = core->vcsManager();

    // Get the current file. Are we on a temporary submit editor indicated by
    // temporary path prefix or does the file contains a hash, indicating a project
    // folder?
    State state;
    state.currentFile = core->fileManager()->currentFile();
    QScopedPointer<QFileInfo> currentFileInfo; // Instantiate QFileInfo only once if required.
    if (!state.currentFile.isEmpty()) {
        const bool isTempFile = state.currentFile.startsWith(QDir::tempPath());
        // Quick check: Does it look like a patch?
        const bool isPatch = state.currentFile.endsWith(QLatin1String(".patch"))
                             || state.currentFile.endsWith(QLatin1String(".diff"));
        if (isPatch) {
            // Patch: Figure out a name to display. If it is a temp file, it could be
            // Codepaster. Use the display name of the editor.
            state.currentPatchFile = state.currentFile;
            if (isTempFile)
                state.currentPatchFileDisplayName = displayNameOfEditor(state.currentPatchFile);
            if (state.currentPatchFileDisplayName.isEmpty()) {
                currentFileInfo.reset(new QFileInfo(state.currentFile));
                state.currentPatchFileDisplayName = currentFileInfo->fileName();
            }
        }
        // For actual version control operations on it:
        // Do not show temporary files and project folders ('#')
        if (isTempFile || state.currentFile.contains(QLatin1Char('#')))
            state.currentFile.clear();
    }

    // Get the file and its control. Do not use the file unless we find one
    Core::IVersionControl *fileControl = 0;
    if (!state.currentFile.isEmpty()) {
        if (currentFileInfo.isNull())
            currentFileInfo.reset(new QFileInfo(state.currentFile));
        state.currentFileDirectory = currentFileInfo->absolutePath();
        state.currentFileName = currentFileInfo->fileName();
        fileControl = vcsManager->findVersionControlForDirectory(state.currentFileDirectory,
                                                                 &state.currentFileTopLevel);
        if (!fileControl)
            state.clearFile();
    }
    // Check for project, find the control
    Core::IVersionControl *projectControl = 0;
    if (const ProjectExplorer::Project *currentProject = pe->currentProject()) {
        state.currentProjectPath = currentProject->projectDirectory();
        state.currentProjectName = currentProject->displayName();
        projectControl = vcsManager->findVersionControlForDirectory(state.currentProjectPath,
                                                                    &state.currentProjectTopLevel);
        if (projectControl) {
            // If we have both, let the file's one take preference
            if (fileControl && projectControl != fileControl) {
                state.clearProject();
            }
        } else {
            state.clearProject(); // No control found
        }
    }
    // Assemble state and emit signal.
    Core::IVersionControl *vc = state.currentFile.isEmpty() ? projectControl : fileControl;
    if (!vc) // Need a repository to patch
        state.clearPatchFile();
    if (debug)
        qDebug() << state << (vc ? vc->displayName() : QString(QLatin1String("No version control")));
    emit stateChanged(state, vc);
}

} // namespace Internal

class VCSBasePluginStateData : public QSharedData {
public:
    Internal::State m_state;
};

VCSBasePluginState::VCSBasePluginState() : data(new VCSBasePluginStateData)
{
}

VCSBasePluginState::VCSBasePluginState(const VCSBasePluginState &rhs) : data(rhs.data)
{
}

VCSBasePluginState &VCSBasePluginState::operator=(const VCSBasePluginState &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

VCSBasePluginState::~VCSBasePluginState()
{
}

QString VCSBasePluginState::currentFile() const
{
    return data->m_state.currentFile;
}

QString VCSBasePluginState::currentFileName() const
{
    return data->m_state.currentFileName;
}

QString VCSBasePluginState::currentFileTopLevel() const
{
    return data->m_state.currentFileTopLevel;
}

QString VCSBasePluginState::currentFileDirectory() const
{
    return data->m_state.currentFileDirectory;
}

QString VCSBasePluginState::relativeCurrentFile() const
{
    QTC_ASSERT(hasFile(), return QString())
    return QDir(data->m_state.currentFileTopLevel).relativeFilePath(data->m_state.currentFile);
}

QString VCSBasePluginState::currentPatchFile() const
{
    return data->m_state.currentPatchFile;
}

QString VCSBasePluginState::currentPatchFileDisplayName() const
{
    return data->m_state.currentPatchFileDisplayName;
}

QString VCSBasePluginState::currentProjectPath() const
{
    return data->m_state.currentProjectPath;
}

QString VCSBasePluginState::currentProjectName() const
{
    return data->m_state.currentProjectName;
}

QString VCSBasePluginState::currentProjectTopLevel() const
{
    return data->m_state.currentProjectTopLevel;
}

QStringList VCSBasePluginState::relativeCurrentProject() const
{
    QStringList rc;
    QTC_ASSERT(hasProject(), return rc)
    if (data->m_state.currentProjectTopLevel != data->m_state.currentProjectPath)
        rc.append(QDir(data->m_state.currentProjectTopLevel).relativeFilePath(data->m_state.currentProjectPath));
    return rc;
}

bool VCSBasePluginState::hasTopLevel() const
{
    return data->m_state.hasFile() || data->m_state.hasProject();
}

QString VCSBasePluginState::topLevel() const
{
    return hasFile() ? data->m_state.currentFileTopLevel : data->m_state.currentProjectTopLevel;
}

bool VCSBasePluginState::equals(const Internal::State &rhs) const
{
    return data->m_state.equals(rhs);
}

bool VCSBasePluginState::equals(const VCSBasePluginState &rhs) const
{
    return equals(rhs.data->m_state);
}

void VCSBasePluginState::clear()
{
    data->m_state.clear();
}

void VCSBasePluginState::setState(const Internal::State &s)
{
    data->m_state = s;
}

bool VCSBasePluginState::isEmpty() const
{
    return data->m_state.isEmpty();
}

bool VCSBasePluginState::hasFile() const
{
    return data->m_state.hasFile();
}

bool VCSBasePluginState::hasPatchFile() const
{
    return !data->m_state.currentPatchFile.isEmpty();
}

bool VCSBasePluginState::hasProject() const
{
    return data->m_state.hasProject();
}

VCSBASE_EXPORT QDebug operator<<(QDebug in, const VCSBasePluginState &state)
{
    in << state.data->m_state;
    return in;
}

//  VCSBasePlugin
struct VCSBasePluginPrivate {
    explicit VCSBasePluginPrivate(const QString &submitEditorId);

    inline bool supportsRepositoryCreation() const;

    const QString m_submitEditorId;
    Core::IVersionControl *m_versionControl;
    VCSBasePluginState m_state;
    int m_actionState;
    QAction *m_testSnapshotAction;
    QAction *m_testListSnapshotsAction;
    QAction *m_testRestoreSnapshotAction;
    QAction *m_testRemoveSnapshotAction;
    QString m_testLastSnapshot;

    static Internal::StateListener *m_listener;
};

VCSBasePluginPrivate::VCSBasePluginPrivate(const QString &submitEditorId) :
    m_submitEditorId(submitEditorId),
    m_versionControl(0),
    m_actionState(-1),
    m_testSnapshotAction(0),
    m_testListSnapshotsAction(0),
    m_testRestoreSnapshotAction(0),
    m_testRemoveSnapshotAction(0)
{
}

bool VCSBasePluginPrivate::supportsRepositoryCreation() const
{
    return m_versionControl && m_versionControl->supportsOperation(Core::IVersionControl::CreateRepositoryOperation);
}

Internal::StateListener *VCSBasePluginPrivate::m_listener = 0;

VCSBasePlugin::VCSBasePlugin(const QString &submitEditorId) :
    d(new VCSBasePluginPrivate(submitEditorId))
{
}

VCSBasePlugin::~VCSBasePlugin()
{
    delete d;
}

void VCSBasePlugin::initialize(Core::IVersionControl *vc)
{
    d->m_versionControl = vc;
    addAutoReleasedObject(vc);

    Internal::VCSPlugin *plugin = Internal::VCSPlugin::instance();
    connect(plugin->coreListener(), SIGNAL(submitEditorAboutToClose(VCSBaseSubmitEditor*,bool*)),
            this, SLOT(slotSubmitEditorAboutToClose(VCSBaseSubmitEditor*,bool*)));
    // First time: create new listener
    if (!VCSBasePluginPrivate::m_listener)
        VCSBasePluginPrivate::m_listener = new Internal::StateListener(plugin);
    connect(VCSBasePluginPrivate::m_listener,
            SIGNAL(stateChanged(VCSBase::Internal::State, Core::IVersionControl*)),
            this,
            SLOT(slotStateChanged(VCSBase::Internal::State,Core::IVersionControl*)));
}

void VCSBasePlugin::extensionsInitialized()
{
    // Initialize enable menus.
    VCSBasePluginPrivate::m_listener->slotStateChanged();
}

void VCSBasePlugin::slotSubmitEditorAboutToClose(VCSBaseSubmitEditor *submitEditor, bool *result)
{
    if (debug)
        qDebug() << this << d->m_submitEditorId << "Closing submit editor" << submitEditor << submitEditor->id();
    if (submitEditor->id() == d->m_submitEditorId)
        *result = submitEditorAboutToClose(submitEditor);
}

Core::IVersionControl *VCSBasePlugin::versionControl() const
{
    return d->m_versionControl;
}

void VCSBasePlugin::slotStateChanged(const VCSBase::Internal::State &newInternalState, Core::IVersionControl *vc)
{
    if (vc == d->m_versionControl) {
        // We are directly affected: Change state
        if (!d->m_state.equals(newInternalState)) {
            d->m_state.setState(newInternalState);
            updateActions(VCSEnabled);
        }
    } else {
        // Some other VCS plugin or state changed: Reset us to empty state.
        const ActionState newActionState = vc ? OtherVCSEnabled : NoVCSEnabled;
        if (d->m_actionState != newActionState || !d->m_state.isEmpty()) {
            d->m_actionState = newActionState;
            const VCSBasePluginState emptyState;
            d->m_state = emptyState;
            updateActions(newActionState);
        }
    }
}

const VCSBasePluginState &VCSBasePlugin::currentState() const
{
    return d->m_state;
}

bool VCSBasePlugin::enableMenuAction(ActionState as, QAction *menuAction) const
{
    if (debug)
        qDebug() << "enableMenuAction" << menuAction->text() << as;
    switch (as) {
    case VCSBase::VCSBasePlugin::NoVCSEnabled: {
        const bool supportsCreation = d->supportsRepositoryCreation();
        menuAction->setVisible(supportsCreation);
        menuAction->setEnabled(supportsCreation);
        return supportsCreation;
    }
    case VCSBase::VCSBasePlugin::OtherVCSEnabled:
        menuAction->setVisible(false);
        return false;
    case VCSBase::VCSBasePlugin::VCSEnabled:
        menuAction->setVisible(true);
        menuAction->setEnabled(true);
        break;
    }
    return true;
}

void VCSBasePlugin::promptToDeleteCurrentFile()
{
    const VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    const bool rc = Core::ICore::instance()->vcsManager()->promptToDelete(versionControl(), state.currentFile());
    if (!rc)
        QMessageBox::warning(0, tr("Version Control"),
                             tr("The file '%1' could not be deleted.").
                             arg(QDir::toNativeSeparators(state.currentFile())),
                             QMessageBox::Ok);
}

static inline bool ask(QWidget *parent, const QString &title, const QString &question, bool defaultValue = true)

{
    const QMessageBox::StandardButton defaultButton = defaultValue ? QMessageBox::Yes : QMessageBox::No;
    return QMessageBox::question(parent, title, question, QMessageBox::Yes|QMessageBox::No, defaultButton) == QMessageBox::Yes;
}

void VCSBasePlugin::createRepository()
{
    QTC_ASSERT(d->m_versionControl->supportsOperation(Core::IVersionControl::CreateRepositoryOperation), return);
    // Find current starting directory
    QString directory;
    if (const ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectExplorerPlugin::instance()->currentProject())
        directory = QFileInfo(currentProject->file()->fileName()).absolutePath();
    // Prompt for a directory that is not under version control yet
    QMainWindow *mw = Core::ICore::instance()->mainWindow();
    do {
        directory = QFileDialog::getExistingDirectory(mw, tr("Choose Repository Directory"), directory);
        if (directory.isEmpty())
            return;
        const Core::IVersionControl *managingControl = Core::ICore::instance()->vcsManager()->findVersionControlForDirectory(directory);
        if (managingControl == 0)
            break;
        const QString question = tr("The directory '%1' is already managed by a version control system (%2)."
                                    " Would you like to specify another directory?").arg(directory, managingControl->displayName());

        if (!ask(mw, tr("Repository already under version control"), question))
            return;
    } while (true);
    // Create
    const bool rc = d->m_versionControl->vcsCreateRepository(directory);
    const QString nativeDir = QDir::toNativeSeparators(directory);
    if (rc) {
        QMessageBox::information(mw, tr("Repository Created"),
                                 tr("A version control repository has been created in %1.").
                                 arg(nativeDir));
    } else {
        QMessageBox::warning(mw, tr("Repository Creation Failed"),
                                 tr("A version control repository could not be created in %1.").
                                 arg(nativeDir));
    }
}

// For internal tests: Create actions driving IVersionControl's snapshot interface.
QList<QAction*> VCSBasePlugin::createSnapShotTestActions()
{
    if (!d->m_testSnapshotAction) {
        d->m_testSnapshotAction = new QAction(QLatin1String("Take snapshot"), this);
        connect(d->m_testSnapshotAction, SIGNAL(triggered()), this, SLOT(slotTestSnapshot()));
        d->m_testListSnapshotsAction = new QAction(QLatin1String("List snapshots"), this);
        connect(d->m_testListSnapshotsAction, SIGNAL(triggered()), this, SLOT(slotTestListSnapshots()));
        d->m_testRestoreSnapshotAction = new QAction(QLatin1String("Restore snapshot"), this);
        connect(d->m_testRestoreSnapshotAction, SIGNAL(triggered()), this, SLOT(slotTestRestoreSnapshot()));
        d->m_testRemoveSnapshotAction = new QAction(QLatin1String("Remove snapshot"), this);
        connect(d->m_testRemoveSnapshotAction, SIGNAL(triggered()), this, SLOT(slotTestRemoveSnapshot()));
    }
    QList<QAction*> rc;
    rc << d->m_testSnapshotAction << d->m_testListSnapshotsAction << d->m_testRestoreSnapshotAction
       << d->m_testRemoveSnapshotAction;
    return rc;
}

void VCSBasePlugin::slotTestSnapshot()
{
    QTC_ASSERT(currentState().hasTopLevel(), return)
    d->m_testLastSnapshot = versionControl()->vcsCreateSnapshot(currentState().topLevel());
    qDebug() << "Snapshot " << d->m_testLastSnapshot;
    VCSBaseOutputWindow::instance()->append(QLatin1String("Snapshot: ") + d->m_testLastSnapshot);
    if (!d->m_testLastSnapshot.isEmpty())
        d->m_testRestoreSnapshotAction->setText(QLatin1String("Restore snapshot ") + d->m_testLastSnapshot);
}

void VCSBasePlugin::slotTestListSnapshots()
{
    QTC_ASSERT(currentState().hasTopLevel(), return)
    const QStringList snapshots = versionControl()->vcsSnapshots(currentState().topLevel());
    qDebug() << "Snapshots " << snapshots;
    VCSBaseOutputWindow::instance()->append(QLatin1String("Snapshots: ") + snapshots.join(QLatin1String(", ")));
}

void VCSBasePlugin::slotTestRestoreSnapshot()
{
    QTC_ASSERT(currentState().hasTopLevel() && !d->m_testLastSnapshot.isEmpty(), return)
    const bool ok = versionControl()->vcsRestoreSnapshot(currentState().topLevel(), d->m_testLastSnapshot);
    const QString msg = d->m_testLastSnapshot+ (ok ? QLatin1String(" restored") : QLatin1String(" failed"));
    qDebug() << msg;
    VCSBaseOutputWindow::instance()->append(msg);
}

void VCSBasePlugin::slotTestRemoveSnapshot()
{
    QTC_ASSERT(currentState().hasTopLevel() && !d->m_testLastSnapshot.isEmpty(), return)
    const bool ok = versionControl()->vcsRemoveSnapshot(currentState().topLevel(), d->m_testLastSnapshot);
    const QString msg = d->m_testLastSnapshot+ (ok ? QLatin1String(" removed") : QLatin1String(" failed"));
    qDebug() << msg;
    VCSBaseOutputWindow::instance()->append(msg);
    d->m_testLastSnapshot.clear();
}

// Find top level for version controls like git/Mercurial that have
// a directory at the top of the repository.
// Note that checking for the existence of files is preferred over directories
// since checking for directories can cause them to be created when
// AutoFS is used (due its automatically creating mountpoints when querying
// a directory). In addition, bail out when reaching the home directory
// of the user or root (generally avoid '/', where mountpoints are created).
QString VCSBasePlugin::findRepositoryForDirectory(const QString &dirS,
                                                  const QString &checkFile)
{
    if (debugRepositorySearch)
        qDebug() << ">VCSBasePlugin::findRepositoryForDirectory" << dirS << checkFile;
    QTC_ASSERT(!dirS.isEmpty() && !checkFile.isEmpty(), return QString());

    const QString root = QDir::rootPath();
    const QString home = QDir::homePath();

    QDir directory(dirS);
    do {
        const QString absDirPath = directory.absolutePath();
        if (absDirPath == root || absDirPath == home)
            break;

        if (QFileInfo(directory, checkFile).isFile()) {
            if (debugRepositorySearch)
                qDebug() << "<VCSBasePlugin::findRepositoryForDirectory> " << absDirPath;
            return absDirPath;
        }
    } while (directory.cdUp());
    if (debugRepositorySearch)
        qDebug() << "<VCSBasePlugin::findRepositoryForDirectory bailing out at " << directory.absolutePath();
    return QString();
}

// Is SSH prompt configured?
static inline QString sshPrompt()
{
    return VCSBase::Internal::VCSPlugin::instance()->settings().sshPasswordPrompt;
}

bool VCSBasePlugin::isSshPromptConfigured()
{
    return !sshPrompt().isEmpty();
}

void VCSBasePlugin::setProcessEnvironment(QProcessEnvironment *e, bool forceCLocale)
{
    if (forceCLocale)
        e->insert(QLatin1String("LANG"), QString(QLatin1Char('C')));
    const QString sshPromptBinary = sshPrompt();
    if (!sshPromptBinary.isEmpty())
        e->insert(QLatin1String("SSH_ASKPASS"), sshPromptBinary);
}

// Run a process fully synchronously, returning Utils::SynchronousProcessResponse
// response struct and using the VCSBasePlugin flags as applicable
static Utils::SynchronousProcessResponse
    runVCS_FullySynchronously(const QString &workingDir,
                              const QString &binary,
                              const QStringList &arguments,
                              int timeOutMS,
                              QProcessEnvironment env,
                              unsigned flags,
                              QTextCodec *outputCodec = 0)
{
    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();

    // Set up process
    unsigned processFlags = 0;
    if (VCSBasePlugin::isSshPromptConfigured() && (flags & VCSBasePlugin::SshPasswordPrompt))
        processFlags |= Utils::SynchronousProcess::UnixTerminalDisabled;
    QSharedPointer<QProcess> process = Utils::SynchronousProcess::createProcess(processFlags);
    if (!workingDir.isEmpty())
        process->setWorkingDirectory(workingDir);
    process->setProcessEnvironment(env);
    if (flags & VCSBasePlugin::MergeOutputChannels)
        process->setProcessChannelMode(QProcess::MergedChannels);

    // Start
    process->start(binary, arguments);
    Utils::SynchronousProcessResponse response;
    if (!process->waitForStarted()) {
        response.result = Utils::SynchronousProcessResponse::StartFailed;
        return response;
    }

    // process output
    QByteArray stdOut;
    QByteArray stdErr;
    const bool timedOut =
            !Utils::SynchronousProcess::readDataFromProcess(*process.data(), timeOutMS,
                                                            &stdOut, &stdErr, true);

    if (!stdErr.isEmpty()) {
        response.stdErr = QString::fromLocal8Bit(stdErr).remove('\r');
        if (!(flags & VCSBasePlugin::SuppressStdErrInLogWindow))
            outputWindow->append(response.stdErr);
    }

    if (!stdOut.isEmpty()) {
        response.stdOut = (outputCodec ? outputCodec->toUnicode(stdOut) : QString::fromLocal8Bit(stdOut))
                          .remove('\r');
        if (flags & VCSBasePlugin::ShowStdOutInLogWindow)
            outputWindow->append(response.stdOut);
    }

    // Result
    if (timedOut) {
        response.result = Utils::SynchronousProcessResponse::Hang;
    } else if (process->exitStatus() != QProcess::NormalExit) {
        response.result = Utils::SynchronousProcessResponse::TerminatedAbnormally;
    } else {
        response.result = process->exitCode() == 0 ?
                          Utils::SynchronousProcessResponse::Finished :
                          Utils::SynchronousProcessResponse::FinishedError;
    }
    return response;
}


Utils::SynchronousProcessResponse
        VCSBasePlugin::runVCS(const QString &workingDir,
                              const QString &binary,
                              const QStringList &arguments,
                              int timeOutMS,
                              unsigned flags,
                              QTextCodec *outputCodec /* = 0 */)
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    return runVCS(workingDir, binary, arguments, timeOutMS, env,
                  flags, outputCodec);
}

Utils::SynchronousProcessResponse
        VCSBasePlugin::runVCS(const QString &workingDir,
                              const QString &binary,
                              const QStringList &arguments,
                              int timeOutMS,
                              QProcessEnvironment env,
                              unsigned flags,
                              QTextCodec *outputCodec /* = 0 */)
{
    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();

    if (!(flags & SuppressCommandLogging))
        outputWindow->appendCommand(workingDir, binary, arguments);

    const bool sshPromptConfigured = VCSBasePlugin::isSshPromptConfigured();
    if (debugExecution) {
        QDebug nsp = qDebug().nospace();
        nsp << "VCSBasePlugin::runVCS" << workingDir << binary << arguments
                << timeOutMS;
        if (flags & ShowStdOutInLogWindow)
            nsp << "stdout";
        if (flags & SuppressStdErrInLogWindow)
            nsp << "suppress_stderr";
        if (flags & SuppressFailMessageInLogWindow)
            nsp << "suppress_fail_msg";
        if (flags & MergeOutputChannels)
            nsp << "merge_channels";
        if (flags & SshPasswordPrompt)
            nsp << "ssh (" << sshPromptConfigured << ')';
        if (flags & SuppressCommandLogging)
            nsp << "suppress_log";
        if (flags & ForceCLocale)
            nsp << "c_locale";
        if (flags & FullySynchronously)
            nsp << "fully_synchronously";
        if (outputCodec)
            nsp << " Codec: " << outputCodec->name();
    }

    VCSBase::VCSBasePlugin::setProcessEnvironment(&env, (flags & ForceCLocale));

    Utils::SynchronousProcessResponse response;

    if (flags & FullySynchronously) {
        response = runVCS_FullySynchronously(workingDir, binary, arguments, timeOutMS,
                                             env, flags, outputCodec);
    } else {
        // Run, connect stderr to the output window
        Utils::SynchronousProcess process;
        if (!workingDir.isEmpty())
            process.setWorkingDirectory(workingDir);

        process.setProcessEnvironment(env);
        process.setTimeout(timeOutMS);
        if (outputCodec)
            process.setStdOutCodec(outputCodec);

        // Suppress terminal on UNIX for ssh prompts if it is configured.
        if (sshPromptConfigured && (flags & SshPasswordPrompt))
            process.setFlags(Utils::SynchronousProcess::UnixTerminalDisabled);

        // connect stderr to the output window if desired
        if (flags & MergeOutputChannels) {
            process.setProcessChannelMode(QProcess::MergedChannels);
        } else {
            if (!(flags & SuppressStdErrInLogWindow)) {
                process.setStdErrBufferedSignalsEnabled(true);
                connect(&process, SIGNAL(stdErrBuffered(QString,bool)), outputWindow, SLOT(append(QString)));
            }
        }

        // connect stdout to the output window if desired
        if (flags & ShowStdOutInLogWindow) {
            process.setStdOutBufferedSignalsEnabled(true);
            connect(&process, SIGNAL(stdOutBuffered(QString,bool)), outputWindow, SLOT(append(QString)));
        }

        process.setTimeOutMessageBoxEnabled(true);

        // Run!
        response = process.run(binary, arguments);
    }

    // Success/Fail message in appropriate window?
    if (response.result == Utils::SynchronousProcessResponse::Finished) {
        if (flags & ShowSuccessMessage)
            outputWindow->append(response.exitMessage(binary, timeOutMS));
    } else {
        if (!(flags & SuppressFailMessageInLogWindow))
            outputWindow->appendError(response.exitMessage(binary, timeOutMS));
    }

    return response;
}
} // namespace VCSBase

#include "vcsbaseplugin.moc"
