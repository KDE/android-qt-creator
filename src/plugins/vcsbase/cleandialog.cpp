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

#include "cleandialog.h"
#include "ui_cleandialog.h"
#include "vcsbaseoutputwindow.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <QtGui/QStandardItemModel>
#include <QtGui/QMessageBox>
#include <QtGui/QApplication>
#include <QtGui/QStyle>
#include <QtGui/QIcon>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QFuture>
#include <QtCore/QtConcurrentRun>

namespace VcsBase {
namespace Internal {

enum { nameColumn, columnCount };
enum { fileNameRole = Qt::UserRole, isDirectoryRole = Qt::UserRole + 1 };

// Helper for recursively removing files.
static void removeFileRecursion(const QFileInfo &f, QString *errorMessage)
{
    // The version control system might list files/directory in arbitrary
    // order, causing files to be removed from parent directories.
    if (!f.exists())
        return;
    if (f.isDir()) {
        const QDir dir(f.absoluteFilePath());
        foreach(const QFileInfo &fi, dir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden))
            removeFileRecursion(fi, errorMessage);
        QDir parent = f.absoluteDir();
        if (!parent.rmdir(f.fileName()))
            errorMessage->append(VcsBase::CleanDialog::tr("The directory %1 could not be deleted.").
                                 arg(QDir::toNativeSeparators(f.absoluteFilePath())));
        return;
    }
    if (!QFile::remove(f.absoluteFilePath())) {
        if (!errorMessage->isEmpty())
            errorMessage->append(QLatin1Char('\n'));
        errorMessage->append(VcsBase::CleanDialog::tr("The file %1 could not be deleted.").
                             arg(QDir::toNativeSeparators(f.absoluteFilePath())));
    }
}

// A QFuture task for cleaning files in the background.
// Emits error signal if not all files can be deleted.
class CleanFilesTask : public QObject
{
    Q_OBJECT

public:
    explicit CleanFilesTask(const QString &repository, const QStringList &files);

    void run();

signals:
    void error(const QString &e);

private:
    const QString m_repository;
    const QStringList m_files;

    QString m_errorMessage;
};

CleanFilesTask::CleanFilesTask(const QString &repository, const QStringList &files) :
    m_repository(repository), m_files(files)
{
}

void CleanFilesTask::run()
{
    foreach(const QString &name, m_files)
        removeFileRecursion(QFileInfo(name), &m_errorMessage);
    if (!m_errorMessage.isEmpty()) {
        // Format and emit error.
        const QString msg = CleanDialog::tr("There were errors when cleaning the repository %1:").
                            arg(QDir::toNativeSeparators(m_repository));
        m_errorMessage.insert(0, QLatin1Char('\n'));
        m_errorMessage.insert(0, msg);
        emit error(m_errorMessage);
    }
    // Run in background, need to delete ourselves
    this->deleteLater();
}

// ---------------- CleanDialogPrivate ----------------

class CleanDialogPrivate
{
public:
    CleanDialogPrivate();

    Internal::Ui::CleanDialog ui;
    QStandardItemModel *m_filesModel;
    QString m_workingDirectory;
};

CleanDialogPrivate::CleanDialogPrivate() :
    m_filesModel(new QStandardItemModel(0, columnCount))
{
}

} // namespace Internal

/*!
    \class VcsBase::CleanDialog

    \brief File selector dialog for files not under version control.

    Completely clean a directory under version control
    from all files that are not under version control based on a list
    generated from the version control system. Presents the user with
    a checkable list of files and/or directories. Double click opens a file.
*/

CleanDialog::CleanDialog(QWidget *parent) :
    QDialog(parent),
    d(new Internal::CleanDialogPrivate)
{
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    d->ui.setupUi(this);
    d->ui.buttonBox->addButton(tr("Delete..."), QDialogButtonBox::AcceptRole);

    d->m_filesModel->setHorizontalHeaderLabels(QStringList(tr("Name")));
    d->ui.filesTreeView->setModel(d->m_filesModel);
    d->ui.filesTreeView->setUniformRowHeights(true);
    d->ui.filesTreeView->setSelectionMode(QAbstractItemView::NoSelection);
    d->ui.filesTreeView->setAllColumnsShowFocus(true);
    d->ui.filesTreeView->setRootIsDecorated(false);
    connect(d->ui.filesTreeView, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(slotDoubleClicked(QModelIndex)));
}

CleanDialog::~CleanDialog()
{
    delete d;
}

void CleanDialog::setFileList(const QString &workingDirectory, const QStringList &l)
{
    d->m_workingDirectory = workingDirectory;
    d->ui.groupBox->setTitle(tr("Repository: %1").
                             arg(QDir::toNativeSeparators(workingDirectory)));
    if (const int oldRowCount = d->m_filesModel->rowCount())
        d->m_filesModel->removeRows(0, oldRowCount);

    QStyle *style = QApplication::style();
    const QIcon folderIcon = style->standardIcon(QStyle::SP_DirIcon);
    const QIcon fileIcon = style->standardIcon(QStyle::SP_FileIcon);
    const QString diffSuffix = QLatin1String(".diff");
    const QString patchSuffix = QLatin1String(".patch");
    const QString proUserSuffix = QLatin1String(".pro.user");
    const QString qmlProUserSuffix = QLatin1String(".qmlproject.user");
    const QChar slash = QLatin1Char('/');
    // Do not initially check patches or 'pro.user' files for deletion.
    foreach (const QString &fileName, l) {
        const QFileInfo fi(workingDirectory + slash + fileName);
        const bool isDir = fi.isDir();
        QStandardItem *nameItem = new QStandardItem(QDir::toNativeSeparators(fileName));
        nameItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        nameItem->setIcon(isDir ? folderIcon : fileIcon);
        const bool saveFile = !isDir && (fileName.endsWith(diffSuffix)
                                        || fileName.endsWith(patchSuffix)
                                        || fileName.endsWith(proUserSuffix)
                                        || fileName.endsWith(qmlProUserSuffix));
        nameItem->setCheckable(true);
        nameItem->setCheckState(saveFile ? Qt::Unchecked : Qt::Checked);
        nameItem->setData(QVariant(fi.absoluteFilePath()), Internal::fileNameRole);
        nameItem->setData(QVariant(isDir), Internal::isDirectoryRole);
        // Tooltip with size information
        if (fi.isFile()) {
            const QString lastModified = fi.lastModified().toString(Qt::DefaultLocaleShortDate);
            nameItem->setToolTip(tr("%1 bytes, last modified %2")
                                 .arg(fi.size()).arg(lastModified));
        }
        d->m_filesModel->appendRow(nameItem);
    }

    for (int c = 0; c < d->m_filesModel->columnCount(); c++)
        d->ui.filesTreeView->resizeColumnToContents(c);
}

QStringList CleanDialog::checkedFiles() const
{
    QStringList rc;
    if (const int rowCount = d->m_filesModel->rowCount()) {
        for (int r = 0; r < rowCount; r++) {
            const QStandardItem *item = d->m_filesModel->item(r, 0);
            if (item->checkState() == Qt::Checked)
                rc.push_back(item->data(Internal::fileNameRole).toString());
        }
    }
    return rc;
}

void CleanDialog::accept()
{
    if (promptToDelete())
        QDialog::accept();
}

bool CleanDialog::promptToDelete()
{
    // Prompt the user and delete files
    const QStringList selectedFiles = checkedFiles();
    if (selectedFiles.isEmpty())
        return true;

    if (QMessageBox::question(this, tr("Delete"),
                              tr("Do you want to delete %n files?", 0, selectedFiles.size()),
                              QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
        return false;

    // Remove in background
    Internal::CleanFilesTask *cleanTask = new Internal::CleanFilesTask(d->m_workingDirectory, selectedFiles);
    connect(cleanTask, SIGNAL(error(QString)),
            VcsBase::VcsBaseOutputWindow::instance(), SLOT(appendSilently(QString)),
            Qt::QueuedConnection);

    QFuture<void> task = QtConcurrent::run(cleanTask, &Internal::CleanFilesTask::run);
    const QString taskName = tr("Cleaning %1").
                             arg(QDir::toNativeSeparators(d->m_workingDirectory));
    Core::ICore::instance()->progressManager()->addTask(task, taskName,
                                                        QLatin1String("VcsBase.cleanRepository"));
    return true;
}

void CleanDialog::slotDoubleClicked(const QModelIndex &index)
{
    // Open file on doubleclick
    if (const QStandardItem *item = d->m_filesModel->itemFromIndex(index))
        if (!item->data(Internal::isDirectoryRole).toBool()) {
            const QString fname = item->data(Internal::fileNameRole).toString();
            Core::EditorManager::instance()->openEditor(fname, Core::Id(), Core::EditorManager::ModeSwitch);
    }
}

void CleanDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        d->ui.retranslateUi(this);
        break;
    default:
        break;
    }
}

} // namespace VcsBase

#include "cleandialog.moc"
