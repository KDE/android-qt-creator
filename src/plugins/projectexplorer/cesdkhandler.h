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

#ifndef CE_SDK_HANDLER_H
#define CE_SDK_HANDLER_H

#include <projectexplorer/projectexplorer.h>

#include <QtCore/QStringList>
#include <QtCore/QDir>

namespace Utils {
class Environment;
}

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT CeSdkInfo
{
public:
    CeSdkInfo();
    inline QString                  name() const;
    inline QString                  binPath() const;
    inline QString                  includePath() const;
    inline QString                  libPath() const;
    void                            addToEnvironment(Utils::Environment &env);
    inline bool                     isValid() const;
    inline int                      majorVersion() const;
    inline int                      minorVersion() const;
    inline bool                     isSupported() const;
private:
    friend class                    CeSdkHandler;
    QString                         m_name;
    QString                         m_bin;
    QString                         m_include;
    QString                         m_lib;
    int                             m_major;
    int                             m_minor;
};

inline QString CeSdkInfo::name() const { return m_name; }
inline QString CeSdkInfo::binPath() const { return m_bin; }
inline QString CeSdkInfo::includePath() const { return m_include; }
inline QString CeSdkInfo::libPath() const { return m_lib; }
inline bool CeSdkInfo::isValid() const { return !m_name.isEmpty() && !m_bin.isEmpty() && !m_include.isEmpty() && !m_lib.isEmpty(); }
inline int CeSdkInfo::majorVersion() const { return m_major; }
inline int CeSdkInfo::minorVersion() const { return m_minor; }
inline bool CeSdkInfo::isSupported() const { return m_major >= 5; }

class PROJECTEXPLORER_EXPORT CeSdkHandler
{
public:
                                    CeSdkHandler();
    bool                            parse(const QString &path);
    inline QList<CeSdkInfo>         listAll() const;
    CeSdkInfo                       find(const QString &name) const;
    static QString                  platformName(const QString &qtpath);
private:
    inline QString                  fixPaths(QString path) const;
    QList<CeSdkInfo>                m_list;
    QString                         VCInstallDir;
    QString                         VSInstallDir;
};

inline QList<CeSdkInfo> CeSdkHandler::listAll() const
{
    return m_list;
}

inline QString CeSdkHandler::fixPaths(QString path) const
{
    const char vcInstallMacro[] = "$(VCInstallDir)";
    const char vsInstallMacro[] = "$(VSInstallDir)";

    path.replace(QLatin1String(vcInstallMacro), VCInstallDir);
    path.replace(QLatin1String(vsInstallMacro), VSInstallDir);
    path.remove(QLatin1String(";$(PATH)"));
    return QDir::toNativeSeparators(path);
}

} // namespace Qt4ProjectManager

#endif // CE_SDK_HANDLER_H
