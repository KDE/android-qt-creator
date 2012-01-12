/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#ifndef MERCURIALCLIENT_H
#define MERCURIALCLIENT_H

#include "mercurialsettings.h"
#include <vcsbase/vcsbaseclient.h>

namespace Mercurial {
namespace Internal {
struct MercurialDiffParameters;

class MercurialClient : public VcsBase::VcsBaseClient
{
    Q_OBJECT
public:
    MercurialClient(MercurialSettings *settings);

    MercurialSettings *settings() const;

    virtual bool synchronousClone(const QString &workingDir,
                                  const QString &srcLocation,
                                  const QString &dstLocation,
                                  const QStringList &extraOptions = QStringList());
    bool manifestSync(const QString &repository, const QString &filename);
    QString branchQuerySync(const QString &repositoryRoot);
    bool parentRevisionsSync(const QString &workingDirectory,
                             const QString &file /* = QString() */,
                             const QString &revision,
                             QStringList *parents);
    bool shortDescriptionSync(const QString &workingDirectory, const QString &revision,
                              const QString &format /* = QString() */, QString *description);
    bool shortDescriptionSync(const QString &workingDirectory, const QString &revision,
                              QString *description);
    bool shortDescriptionsSync(const QString &workingDirectory, const QStringList &revisions,
                              QStringList *descriptions);
    void incoming(const QString &repositoryRoot, const QString &repository = QString());
    void outgoing(const QString &repositoryRoot);
    QString vcsGetRepositoryURL(const QString &directory);

    void annotate(const QString &workingDir, const QString &file,
                  const QString revision = QString(), int lineNumber = -1,
                  const QStringList &extraOptions = QStringList());
    void commit(const QString &repositoryRoot, const QStringList &files,
                const QString &commitMessageFile,
                const QStringList &extraOptions = QStringList());
    void diff(const QString &workingDir, const QStringList &files = QStringList(),
              const QStringList &extraOptions = QStringList());
    void import(const QString &repositoryRoot, const QStringList &files,
                const QStringList &extraOptions = QStringList());
    void revertAll(const QString &workingDir, const QString &revision = QString(),
                   const QStringList &extraOptions = QStringList());
    void view(const QString &source, const QString &id,
              const QStringList &extraOptions = QStringList());

public:
    QString findTopLevelForFile(const QFileInfo &file) const;

protected:
    QString vcsEditorKind(VcsCommand cmd) const;
    QStringList revisionSpec(const QString &revision) const;
    VcsBase::VcsBaseEditorParameterWidget *createDiffEditor(const QString &workingDir,
                                                            const QStringList &files,
                                                            const QStringList &extraOptions);
    StatusItem parseStatusLine(const QString &line) const;
};

} //namespace Internal
} //namespace Mercurial

#endif // MERCURIALCLIENT_H
