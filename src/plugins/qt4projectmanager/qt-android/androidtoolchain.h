/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef ANDROIDTOOLCHAIN_H
#define ANDROIDTOOLCHAIN_H

#include <projectexplorer/toolchain.h>

namespace Qt4ProjectManager {
    class QtVersion;
    namespace Internal {

class AndroidToolChain : public ProjectExplorer::GccToolChain
{
public:
    AndroidToolChain(const QString &targetRoot);
    virtual ~AndroidToolChain();

    void addToEnvironment(Utils::Environment &env);
    ProjectExplorer::ToolChainType type() const;
    QString makeCommand() const;

    QString maddeRoot() const;
    QString targetRoot() const;
    QString targetName() const;
    QString sysrootRoot() const;
    QString madAdminCommand() const;

    enum AndroidVersion { android_4, android_5, android_8};
    AndroidVersion version() const;
    bool allowsRemoteMounts() const { return version() == android_8; }
    bool allowsPackagingDisabling() const { return version() == android_8; }
    bool allowsQmlDebugging() const { return version() == android_8; }

protected:
    bool equals(const ToolChain *other) const;

private:
    void setMaddeRoot() const;
    void setSysroot() const;

private:
    mutable QString m_maddeRoot;
    mutable bool m_maddeInitialized;

    mutable QString m_sysrootRoot;
    mutable bool m_sysrootInitialized;

    const QString m_targetRoot;
};

    } // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDTOOLCHAIN_H
