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

#ifndef PROJECTMANAGER_USERFILEACCESSOR_H
#define PROJECTMANAGER_USERFILEACCESSOR_H

#include <QtCore/QVariantMap>


namespace ProjectExplorer {

class Project;

namespace Internal {
class UserFileVersionHandler;
}

class SettingsAccessor
{
public:
    ~SettingsAccessor();

    static SettingsAccessor *instance();

    QVariantMap restoreSettings(Project *project) const;
    bool saveSettings(const Project *project, const QVariantMap &map) const;

private:
    SettingsAccessor();

    // Takes ownership of the handler!
    void addVersionHandler(Internal::UserFileVersionHandler *handler);

    // The relevant data from the settings currently in use.
    class SettingsData
    {
    public:
        SettingsData() : m_version(-1), m_usingBackup(false) {}
        SettingsData(const QVariantMap &map) : m_version(-1), m_usingBackup(false), m_map(map) {}

        void clear();
        bool isValid() const;

        int m_version;
        bool m_usingBackup;
        QVariantMap m_map;
        QString m_fileName;
    };

    // The entity which actually reads/writes to the settings file.
    class FileAccessor
    {
    public:
        FileAccessor(const QByteArray &id,
                     const QString &defaultSuffix,
                     const QString &environmentSuffix,
                     bool envSpecific,
                     bool versionStrict);

        bool readFile(Project *project, SettingsData *settings) const;
        bool writeFile(const Project *project, const SettingsData *settings) const;

    private:
        void assignSuffix(const QString &defaultSuffix, const QString &environmentSuffix);
        QString assembleFileName(const Project *project) const;
        bool findNewestCompatibleSetting(SettingsData *settings) const;

        QByteArray m_id;
        QString m_suffix;
        bool m_environmentSpecific;
        bool m_versionStrict;
    };

    static bool verifyEnvironmentId(const QString &id);

    QMap<int, Internal::UserFileVersionHandler *> m_handlers;
    int m_firstVersion;
    int m_lastVersion;
    const FileAccessor m_userFileAcessor;
    const FileAccessor m_sharedFileAcessor;
};

} // namespace ProjectExplorer

#endif // PROJECTMANAGER_USERFILEACCESSOR_H
