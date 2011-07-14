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

#ifndef PERFORCEVERSIONCONTROL_H
#define PERFORCEVERSIONCONTROL_H

#include <coreplugin/iversioncontrol.h>

namespace Perforce {
namespace Internal {
class PerforcePlugin;

// Just a proxy for PerforcePlugin
class PerforceVersionControl : public Core::IVersionControl
{
    Q_OBJECT
public:
    explicit PerforceVersionControl(PerforcePlugin *plugin);

    QString displayName() const;
    QString id() const;

    bool managesDirectory(const QString &directory, QString *topLevel = 0) const;

    bool isConfigured() const;
    bool supportsOperation(Operation operation) const;
    bool vcsOpen(const QString &fileName);
    SettingsFlags settingsFlags() const;
    bool vcsAdd(const QString &fileName);
    bool vcsDelete(const QString &filename);
    bool vcsMove(const QString &from, const QString &to);
    bool vcsCreateRepository(const QString &directory);
    bool vcsCheckout(const QString &directory, const QByteArray &url);
    QString vcsGetRepositoryURL(const QString &directory);
    QString vcsCreateSnapshot(const QString &topLevel);
    QStringList vcsSnapshots(const QString &topLevel);
    bool vcsRestoreSnapshot(const QString &topLevel, const QString &name);
    bool vcsRemoveSnapshot(const QString &topLevel, const QString &name);
    bool vcsAnnotate(const QString &file, int line);

    void emitRepositoryChanged(const QString &s);
    void emitFilesChanged(const QStringList &l);
    void emitConfigurationChanged();

private:
    bool m_enabled;
    PerforcePlugin *m_plugin;
};

} // Internal
} // Perforce

#endif // PERFORCEVERSIONCONTROL_H
