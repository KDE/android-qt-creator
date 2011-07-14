/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef IVERSIONCONTROL_H
#define IVERSIONCONTROL_H

#include "core_global.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QFlags>

namespace Core {

class CORE_EXPORT IVersionControl : public QObject
{
    Q_OBJECT
    Q_ENUMS(SettingsFlag Operation)
public:
    enum SettingsFlag {
        AutoOpen = 0x1
    };
    Q_DECLARE_FLAGS(SettingsFlags, SettingsFlag)

    enum Operation {
        AddOperation, DeleteOperation, OpenOperation, MoveOperation,
        CreateRepositoryOperation,
        SnapshotOperations,
        AnnotateOperation,
        CheckoutOperation,
        GetRepositoryRootOperation
        };

    explicit IVersionControl(QObject *parent = 0) : QObject(parent) {}
    virtual ~IVersionControl() {}

    virtual QString displayName() const = 0;
    virtual QString id() const = 0;

    /*!
     * Returns whether files in this directory should be managed with this
     * version control.
     * If \a topLevel is non-null, it should return the topmost directory,
     * for which this IVersionControl should be used. The VcsManager assumes
     * that all files in the returned directory are managed by the same IVersionControl.
     */

    virtual bool managesDirectory(const QString &filename, QString *topLevel = 0) const = 0;

    /*!
     * Returns true is the VCS is configured to run.
     */
    virtual bool isConfigured() const = 0;
    /*!
     * Called to query whether a VCS supports the respective operations.
     *
     * Return false if the VCS is not configured yet.
     */
    virtual bool supportsOperation(Operation operation) const = 0;

    /*!
     * Called prior to save, if the file is read only. Should be implemented if
     * the scc requires a operation before editing the file, e.g. 'p4 edit'
     *
     * \note The EditorManager calls this for the editors.
     */
    virtual bool vcsOpen(const QString &fileName) = 0;

    /*!
     * Returns settings.
     */

    virtual SettingsFlags settingsFlags() const { return 0; }

    /*!
     * Called after a file has been added to a project If the version control
     * needs to know which files it needs to track you should reimplement this
     * function, e.g. 'p4 add', 'cvs add', 'svn add'.
     *
     * \note This function should be called from IProject subclasses after
     *       files are added to the project.
     */
    virtual bool vcsAdd(const QString &filename) = 0;

    /*!
     * Called after a file has been removed from the project (if the user
     * wants), e.g. 'p4 delete', 'svn delete'.
     */
    virtual bool vcsDelete(const QString &filename) = 0;

    /*!
     * Called to rename a file, should do the actual on disk renaming
     * (e.g. git mv, svn move, p4 move)
     */
    virtual bool vcsMove(const QString &from, const QString &to) = 0;

    /*!
     * Called to initialize the version control system in a directory.
     */
    virtual bool vcsCreateRepository(const QString &directory) = 0;

    /*!
     * Called to clone/checkout the version control system in a directory.
     */
    virtual bool vcsCheckout(const QString &directory, const QByteArray &url) = 0;

    /*!
     * Called to get the version control repository root.
     */
    virtual QString vcsGetRepositoryURL(const QString &director) = 0;

    /*!
     * Create a snapshot of the current state and return an identifier or
     * an empty string in case of failure.
     */
    virtual QString vcsCreateSnapshot(const QString &topLevel) = 0;

    /*!
     * List snapshots.
     */
    virtual QStringList vcsSnapshots(const QString &topLevel) = 0;

    /*!
     * Restore a snapshot.
     */
    virtual bool vcsRestoreSnapshot(const QString &topLevel, const QString &name) = 0;

    /*!
     * Remove a snapshot.
     */
    virtual bool vcsRemoveSnapshot(const QString &topLevel, const QString &name) = 0;

    /*!
     * Display annotation for a file and scroll to line
     */
    virtual bool vcsAnnotate(const QString &file, int line) = 0;

signals:
    void repositoryChanged(const QString &repository);
    void filesChanged(const QStringList &files);
    void configurationChanged();

    // TODO: ADD A WAY TO DETECT WHETHER A FILE IS MANAGED, e.g
    // virtual bool sccManaged(const QString &filename) = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IVersionControl::SettingsFlags)

} // namespace Core

#endif // IVERSIONCONTROL_H
