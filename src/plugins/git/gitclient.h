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

#ifndef GITCLIENT_H
#define GITCLIENT_H

#include "gitsettings.h"

#include <coreplugin/editormanager/ieditor.h>
#include <vcsbase/command.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QErrorMessage;
class QSignalMapper;
class QDebug;
class QProcessEnvironment;
QT_END_NAMESPACE

namespace Core {
    class ICore;
}

namespace VcsBase {
    class VcsBaseEditorWidget;
    class SubmitFileModel;
}

namespace Utils {
    struct SynchronousProcessResponse;
}

namespace Git {
namespace Internal {

class GitPlugin;
class GitOutputWindow;
class CommitData;
struct GitSubmitEditorPanelData;
class Stash;

class GitClient : public QObject
{
    Q_OBJECT

public:
    static const char *stashNamePrefix;

    explicit GitClient(GitSettings *settings);
    ~GitClient();

    QString gitBinaryPath(bool *ok = 0, QString *errorMessage = 0) const;
    unsigned gitVersion(bool silent, QString *errorMessage = 0) const;
    QString gitVersionString(bool silent, QString *errorMessage = 0) const;

    QString findRepositoryForDirectory(const QString &dir);

    void diff(const QString &workingDirectory, const QStringList &diffArgs, const QString &fileName);
    void diff(const QString &workingDirectory, const QStringList &diffArgs,
              const QStringList &unstagedFileNames, const QStringList &stagedFileNames= QStringList());
    void diffBranch(const QString &workingDirectory,
                    const QStringList &diffArgs,
                    const QString &branchName);

    void status(const QString &workingDirectory);
    void graphLog(const QString &workingDirectory) { graphLog(workingDirectory, QString()); }
    void graphLog(const QString &workingDirectory, const QString &branch);
    void log(const QString &workingDirectory, const QStringList &fileNames = QStringList(),
             bool enableAnnotationContextMenu = false, const QStringList &args = QStringList());
    void blame(const QString &workingDirectory, const QStringList &args, const QString &fileName,
               const QString &revision = QString(), int lineNumber = -1);
    void checkout(const QString &workingDirectory, const QString &file);
    void checkoutBranch(const QString &workingDirectory, const QString &branch);
    void hardReset(const QString &workingDirectory, const QString &commit = QString());
    void addFile(const QString &workingDirectory, const QString &fileName);
    bool synchronousAdd(const QString &workingDirectory,
                        // Warning: Works only from 1.6.1 onwards
                        bool intendToAdd,
                        const QStringList &files);
    bool synchronousDelete(const QString &workingDirectory,
                           bool force,
                           const QStringList &files);
    bool synchronousMove(const QString &workingDirectory,
                         const QString &from,
                         const QString &to);
    bool synchronousReset(const QString &workingDirectory,
                          const QStringList &files = QStringList(),
                          QString *errorMessage = 0);
    bool synchronousCleanList(const QString &workingDirectory, QStringList *files, QString *errorMessage);
    bool synchronousApplyPatch(const QString &workingDirectory, const QString &file, QString *errorMessage);
    bool synchronousInit(const QString &workingDirectory);
    bool synchronousCheckoutFiles(const QString &workingDirectory,
                                  QStringList files = QStringList(),
                                  QString revision = QString(), QString *errorMessage = 0,
                                  bool revertStaging = true);
    // Checkout branch
    bool synchronousCheckoutBranch(const QString &workingDirectory, const QString &branch, QString *errorMessage = 0);

    // Do a stash and return identier.
    enum { StashPromptDescription = 0x1, StashImmediateRestore = 0x2, StashIgnoreUnchanged = 0x4 };
    QString synchronousStash(const QString &workingDirectory,
                             const QString &messageKeyword = QString(),
                             unsigned flags = 0, bool *unchanged = 0);

    bool executeSynchronousStash(const QString &workingDirectory,
                                 const QString &message = QString(),
                                 QString *errorMessage = 0);
    bool synchronousStashRestore(const QString &workingDirectory,
                                 const QString &stash,
                                 const QString &branch = QString(),
                                 QString *errorMessage = 0);
    bool synchronousStashRemove(const QString &workingDirectory,
                                const QString &stash = QString(),
                                QString *errorMessage = 0);
    bool synchronousBranchCmd(const QString &workingDirectory, QStringList branchArgs,
                              QString *output, QString *errorMessage);
    bool synchronousRemoteCmd(const QString &workingDirectory, QStringList remoteArgs,
                              QString *output, QString *errorMessage);
    bool synchronousShow(const QString &workingDirectory, const QString &id,
                              QString *output, QString *errorMessage);
    bool synchronousParentRevisions(const QString &workingDirectory,
                                    const QStringList &files /* = QStringList() */,
                                    const QString &revision,
                                    QStringList *parents,
                                    QString *errorMessage);
    bool synchronousShortDescription(const QString &workingDirectory, const QString &revision,
                                     QString *description, QString *errorMessage);
    bool synchronousShortDescription(const QString &workingDirectory, const QString &revision,
                                     const QString &format, QString *description, QString *errorMessage);
    bool synchronousShortDescriptions(const QString &workingDirectory, const QStringList &revisions,
                                      QStringList *descriptions, QString *errorMessage);
    bool synchronousTopRevision(const QString &workingDirectory, QString *revision = 0,
                                QString *branch = 0, QString *errorMessage = 0);

    bool cloneRepository(const QString &directory, const QByteArray &url);
    QString vcsGetRepositoryURL(const QString &directory);
    bool synchronousFetch(const QString &workingDirectory, const QString &remote);
    bool synchronousPull(const QString &workingDirectory);
    bool synchronousPush(const QString &workingDirectory, const QString &remote = QString());

    // git svn support (asynchronous).
    void synchronousSubversionFetch(const QString &workingDirectory);
    void subversionLog(const QString &workingDirectory);

    void stashPop(const QString &workingDirectory);
    void revert(const QStringList &files, bool revertStaging);
    void branchList(const QString &workingDirectory);
    void stashList(const QString &workingDirectory);
    bool synchronousStashList(const QString &workingDirectory,
                              QList<Stash> *stashes,
                              QString *errorMessage = 0);
    // Resolve a stash name from message (for IVersionControl's names).
    bool stashNameFromMessage(const QString &workingDirectory,
                              const QString &messge, QString *name,
                              QString *errorMessage = 0);

    QString readConfig(const QString &workingDirectory, const QStringList &configVar) const;

    QString readConfigValue(const QString &workingDirectory, const QString &configVar) const;

    enum StashResult { StashUnchanged, StashCanceled, StashFailed,
                       Stashed, NotStashed /* User did not want it */ };
    StashResult ensureStash(const QString &workingDirectory, QString *errorMessage);
    StashResult ensureStash(const QString &workingDirectory);

    bool getCommitData(const QString &workingDirectory, bool amend,
                       QString *commitTemplate, CommitData *commitData,
                       QString *errorMessage);

    bool addAndCommit(const QString &workingDirectory,
                      const GitSubmitEditorPanelData &data,
                      const QString &amendSHA1,
                      const QString &messageFile,
                      VcsBase::SubmitFileModel *model);

    enum StatusResult { StatusChanged, StatusUnchanged, StatusFailed };
    StatusResult gitStatus(const QString &workingDirectory,
                           bool untracked = false,
                           QString *output = 0,
                           QString *errorMessage = 0, bool *onBranch = 0);

    void launchGitK(const QString &workingDirectory);
    QStringList synchronousRepositoryBranches(const QString &repositoryURL);

    GitSettings *settings() const;

    QProcessEnvironment processEnvironment() const;

    static QString msgNoChangedFiles();

    static const char *noColorOption;
    static const char *decorateOption;

public slots:
    void show(const QString &source, const QString &id, const QStringList &args = QStringList());
    void saveSettings();

private slots:
    void slotBlameRevisionRequested(const QString &source, QString change, int lineNumber);
    void appendOutputData(const QByteArray &data) const;
    void appendOutputDataSilently(const QByteArray &data) const;

private:
    QTextCodec *getSourceCodec(const QString &file) const;
    VcsBase::VcsBaseEditorWidget *findExistingVCSEditor(const char *registerDynamicProperty,
                                                  const QString &dynamicPropertyValue) const;
    enum CodecType { CodecSource, CodecLogOutput, CodecNone };
    VcsBase::VcsBaseEditorWidget *createVcsEditor(const Core::Id &kind,
                                            QString title,
                                            const QString &source,
                                            CodecType codecType,
                                            const char *registerDynamicProperty,
                                            const QString &dynamicPropertyValue,
                                            QWidget *configWidget) const;

    VcsBase::Command *createCommand(const QString &workingDirectory,
                             VcsBase::VcsBaseEditorWidget* editor = 0,
                             bool useOutputToWindow = false,
                             int editorLineNumber = -1);

    VcsBase::Command *executeGit(const QString &workingDirectory,
                           const QStringList &arguments,
                           VcsBase::VcsBaseEditorWidget* editor = 0,
                           bool useOutputToWindow = false,
                           VcsBase::Command::TerminationReportMode tm = VcsBase::Command::NoReport,
                           int editorLineNumber = -1,
                           bool unixTerminalDisabled = false);

    // Fully synchronous git execution (QProcess-based).
    bool fullySynchronousGit(const QString &workingDirectory,
                        const QStringList &arguments,
                        QByteArray* outputText,
                        QByteArray* errorText,
                        bool logCommandToWindow = true) const;

    // Synchronous git execution using Utils::SynchronousProcess, with
    // log windows updating (using VcsBasePlugin::runVcs with flags).
    inline Utils::SynchronousProcessResponse
            synchronousGit(const QString &workingDirectory, const QStringList &arguments,
                           unsigned flags = 0, QTextCodec *outputCodec = 0);

    // determine version as '(major << 16) + (minor << 8) + patch' or 0.
    unsigned synchronousGitVersion(bool silent, QString *errorMessage = 0) const;

    enum RevertResult { RevertOk, RevertUnchanged, RevertCanceled, RevertFailed };
    RevertResult revertI(QStringList files,
                         bool *isDirectory,
                         QString *errorMessage,
                         bool revertStaging);
    void connectRepositoryChanged(const QString & repository, VcsBase::Command *cmd);
    bool synchronousPull(const QString &workingDirectory, bool rebase);
    void syncAbortPullRebase(const QString &workingDir);
    bool tryLauchingGitK(const QProcessEnvironment &env,
                         const QString &workingDirectory,
                         const QString &gitBinDirectory,
                         bool silent);

    mutable QString m_gitVersionForBinary;
    mutable unsigned m_cachedGitVersion;

    const QString m_msgWait;
    Core::ICore *m_core;
    QSignalMapper *m_repositoryChangedSignalMapper;
    GitSettings *m_settings;
};

} // namespace Internal
} // namespace Git

#endif // GITCLIENT_H
