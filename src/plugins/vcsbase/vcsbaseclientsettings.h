/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Brian McGillion & Hugues Delorme
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef VCSBASECLIENTSETTINGS_H
#define VCSBASECLIENTSETTINGS_H

#include "vcsbase_global.h"
#include <QtCore/QString>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace VCSBase {

class VCSBASE_EXPORT VCSBaseClientSettings
{
public:
    VCSBaseClientSettings();

    QString binary() const;
    void setBinary(const QString &);

    // Calculated
    QStringList standardArguments() const;

    QString userName() const;
    void setUserName(const QString &);

    QString email() const;
    void setEmail(const QString &);

    int logCount() const;
    void setLogCount(int l);

    bool prompt() const;
    void setPrompt(bool b);

    int timeoutMilliSeconds() const;
    int timeoutSeconds() const;
    void setTimeoutSeconds(int s);

    virtual void writeSettings(QSettings *settings, const QString &group) const;
    virtual void readSettings(const QSettings *settings, const QString &group);

    virtual bool equals(const VCSBaseClientSettings &rhs) const;

private:
    QString m_binary;
    QStringList m_standardArguments;
    QString m_user;
    QString m_mail;
    int m_logCount;
    bool m_prompt;
    int m_timeoutSeconds;
};

inline bool operator==(const VCSBaseClientSettings &s1, const VCSBaseClientSettings &s2)
{ return s1.equals(s2); }
inline bool operator!=(const VCSBaseClientSettings &s1, const VCSBaseClientSettings &s2)
{ return !s1.equals(s2); }

} // namespace VCSBase

#endif // VCSBASECLIENTSETTINGS_H
