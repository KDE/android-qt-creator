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

#ifndef GITORIOUS_H
#define GITORIOUS_H

#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>
#include <QtCore/QUrl>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
class QDebug;
class QUrl;
class QSettings;
QT_END_NAMESPACE

namespace Gitorious {
namespace Internal {

struct GitoriousRepository
{
    enum Type {
        MainLineRepository,
        CloneRepository,
        BaselineRepository, // Nokia extension
        SharedRepository,   // Nokia extension
        PersonalRepository // Nokia extension
    };

    GitoriousRepository();

    QString name;
    QString owner;
    QUrl pushUrl;
    QUrl cloneUrl;
    QString description;
    Type type;
    int id;
};

struct GitoriousProject
{
    QString name;
    QString description;
    QList<GitoriousRepository> repositories;
};

struct GitoriousCategory
{
    typedef QList<QSharedPointer<GitoriousProject > > ProjectList;

    GitoriousCategory(const QString &name = QString());

    QString name;
};

struct GitoriousHost
{
    enum State { ProjectsQueryRunning, ProjectsComplete, Error };
    typedef QList<QSharedPointer<GitoriousCategory> > CategoryList;
    typedef QList<QSharedPointer<GitoriousProject > > ProjectList;

    GitoriousHost(const QString &hostName = QString(), const QString &description = QString());
    int findCategory(const QString &) const;

    QString hostName;
    QString description;
    CategoryList categories;
    ProjectList projects;
    State state;
};

QDebug operator<<(QDebug d, const GitoriousRepository &r);
QDebug operator<<(QDebug d, const GitoriousProject &p);
QDebug operator<<(QDebug d, const GitoriousCategory &p);
QDebug operator<<(QDebug d, const GitoriousHost &p);

/* Singleton that manages a list of gitorious hosts, running network queries
 * in the background. It models hosts with a flat list of projects (Gitorious
 * has a concept of categories, but this is not enforced, and there is no
 * way to query them).
 * As 24.07.2009, the only supported XML request of the host is a paginated
 * "list-all-projects".  */

class Gitorious : public QObject
{
    Q_DISABLE_COPY(Gitorious)
    Q_OBJECT

public:
    static Gitorious &instance();

    const QList<GitoriousHost> &hosts() const { return m_hosts; }
    int hostCount() const                     { return m_hosts.size(); }
    int categoryCount(int hostIndex) const    { return m_hosts.at(hostIndex).categories.size(); }
    int projectCount(int hostIndex) const     { return m_hosts.at(hostIndex).projects.size(); }
    GitoriousHost::State hostState(int hostIndex) const { return m_hosts.at(hostIndex).state; }

    // If no projects are set, start an asynchronous request querying
    // the projects/categories  of the host.
    void addHost(const QString &addr, const QString &description = QString());
    void addHost(const GitoriousHost &host);
    void removeAt(int index);

    int findByHostName(const QString &hostName) const;
    QString hostName(int i) const              { return m_hosts.at(i).hostName; }
    QString categoryName(int hostIndex, int categoryIndex) const { return m_hosts.at(hostIndex).categories.at(categoryIndex)->name; }

    QString hostDescription(int index) const;
    void setHostDescription(int index, const QString &s);

    void saveSettings(const QString &group, QSettings *s);
    void restoreSettings(const QString &group, const QSettings *s);

    // Return predefined entry for "gitorious.org".
    static GitoriousHost gitoriousOrg();

signals:
    void error(const QString &);
    void projectListReceived(int hostIndex);
    void projectListPageReceived(int hostIndex, int page);
    void categoryListReceived(int index);
    void hostAdded(int index);
    void hostRemoved(int index);

public slots:
    void updateProjectList(int hostIndex);
    void updateCategories(int index);

private slots:
    void slotReplyFinished();

private:
    Gitorious();
    void listProjectsReply(int hostIndex, int page, const QByteArray &data);
    void listCategoriesReply(int index, QByteArray data);
    void emitError(const QString &e);
    QNetworkReply *createRequest(const QUrl &url, int protocol, int hostIndex, int page = -1);
    void startProjectsRequest(int index, int page = 1);

    QList<GitoriousHost> m_hosts;
    QNetworkAccessManager *m_networkManager;
};

} // namespace Internal
} // namespace Gitorious

#endif // GITORIOUS_H
