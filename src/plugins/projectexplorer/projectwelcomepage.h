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

#ifndef PROJECTWELCOMEPAGE_H
#define PROJECTWELCOMEPAGE_H

#include <QtCore/QAbstractListModel>
#include <QtCore/QStringList>

#include <utils/iwelcomepage.h>
#include <coreplugin/icore.h>

QT_BEGIN_NAMESPACE
class QDeclarativeEngine;
QT_END_NAMESPACE

namespace ProjectExplorer {

class ProjectExplorerPlugin;
class SessionManager;

namespace Internal {

class SessionModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum { DefaultSessionRole = Qt::UserRole+1, LastSessionRole, ActiveSessionRole };

    SessionModel(SessionManager* manager, QObject* parent = 0);
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    Q_SCRIPTABLE bool isDefaultVirgin() const;

public slots:
    void resetSessions();

private:
    SessionManager *m_manager;
};


class ProjectModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum { FilePathRole = Qt::UserRole+1, PrettyFilePathRole };

    ProjectModel(ProjectExplorerPlugin* plugin, QObject* parent = 0);
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

public slots:
    void resetProjects();

private:
    ProjectExplorerPlugin *m_plugin;
};

class ProjectWelcomePage : public Utils::IWelcomePage
{
    Q_OBJECT
public:
    ProjectWelcomePage();

    void facilitateQml(QDeclarativeEngine *engine);
    QUrl pageLocation() const { return QUrl::fromLocalFile(Core::ICore::instance()->resourcePath() + QLatin1String("/welcomescreen/develop.qml")); }
    QWidget *page() { return 0; }
    QString title() const { return tr("Develop"); }
    int priority() const { return 20; }

    void reloadWelcomeScreenData();

signals:
    void requestProject(const QString &project);
    void requestSession(const QString &session);
    void manageSessions();
private:
    SessionModel *m_sessionModel;
    ProjectModel *m_projectModel;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTWELCOMEPAGE_H
