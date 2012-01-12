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

#ifndef BAZAARCONTROL_H
#define BAZAARCONTROL_H

#include <coreplugin/iversioncontrol.h>

QT_BEGIN_NAMESPACE
class QVariant;
QT_END_NAMESPACE

namespace Bazaar {
namespace Internal {

class BazaarClient;

//Implements just the basics of the Version Control Interface
//BazaarClient handles all the work
class BazaarControl: public Core::IVersionControl
{
    Q_OBJECT

public:
    explicit BazaarControl(BazaarClient *bazaarClient);

    QString displayName() const;
    Core::Id id() const;

    bool managesDirectory(const QString &filename, QString *topLevel = 0) const;
    bool isConfigured() const;
    bool supportsOperation(Operation operation) const;
    bool vcsOpen(const QString &fileName);
    bool vcsAdd(const QString &filename);
    bool vcsDelete(const QString &filename);
    bool vcsMove(const QString &from, const QString &to);
    bool vcsCreateRepository(const QString &directory);
    bool vcsCheckout(const QString &directory, const QByteArray &url);
    QString vcsGetRepositoryURL(const QString &directory);
    QString vcsCreateSnapshot(const QString &topLevel);
    QStringList vcsSnapshots(const QString &topLevel);
    bool vcsRestoreSnapshot(const QString &topLevel, const QString &name);
    bool vcsRemoveSnapshot(const QString &topLevel, const QString &name);
    virtual bool vcsAnnotate(const QString &file, int line);

public slots:
    // To be connected to the VCSTask's success signal to emit the repository/
    // files changed signals according to the variant's type:
    // String -> repository, StringList -> files
    void changed(const QVariant &);
    void emitConfigurationChanged();

private:
    BazaarClient *m_bazaarClient;
};

} // namespace Internal
} // namespace Bazaar

#endif // BAZAARCONTROL_H
