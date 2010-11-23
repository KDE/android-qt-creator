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

#include "androidtoolchain.h"
#include "androidconstants.h"

#include <QtCore/QDir>
#include <QtCore/QStringBuilder>
#include <QtCore/QTextStream>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

AndroidToolChain::AndroidToolChain(const QString &targetRoot)
    : GccToolChain(targetRoot % QLatin1String("/bin/gcc"))
    , m_maddeInitialized(false)
    , m_sysrootInitialized(false)
    , m_targetRoot(targetRoot)
{
}

AndroidToolChain::~AndroidToolChain()
{
}

ProjectExplorer::ToolChainType AndroidToolChain::type() const
{
    return ProjectExplorer::ToolChain_GCC_ANDROID;
}

void AndroidToolChain::addToEnvironment(Utils::Environment &env)
{
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/bin")
        .arg(maddeRoot())));
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/bin")
        .arg(targetRoot())));

    // put this into environment to make pkg-config stuff work
    env.prependOrSet(QLatin1String("SYSROOT_DIR"), sysrootRoot());
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/madbin")
        .arg(maddeRoot())));
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/madlib")
        .arg(maddeRoot())));
    env.prependOrSet(QLatin1String("PERL5LIB"),
        QDir::toNativeSeparators(QString("%1/madlib/perl5").arg(maddeRoot())));
}

QString AndroidToolChain::makeCommand() const
{
    return QLatin1String("make" ANDROID_EXEC_SUFFIX);
}

bool AndroidToolChain::equals(const ToolChain *other) const
{
    const AndroidToolChain *toolChain = static_cast<const AndroidToolChain*> (other);
    return other->type() == type()
        && toolChain->sysrootRoot() == sysrootRoot()
        && toolChain->targetRoot() == targetRoot();
}

QString AndroidToolChain::maddeRoot() const
{
    if (!m_maddeInitialized)
        setMaddeRoot();
    return m_maddeRoot;
}

QString AndroidToolChain::madAdminCommand() const
{
    return maddeRoot() + QLatin1String("/bin/mad-admin");
}

QString AndroidToolChain::targetRoot() const
{
    return m_targetRoot;
}

QString AndroidToolChain::targetName() const
{
    return QDir(targetRoot()).dirName();
}

QString AndroidToolChain::sysrootRoot() const
{
    if (!m_sysrootInitialized)
        setSysroot();
    return m_sysrootRoot;
}

AndroidToolChain::AndroidVersion AndroidToolChain::version() const
{
#warning "HERE WE SHOULD return android versions"
        return android_8;
//    const QString &name = targetName();
//    if (name.startsWith(QLatin1String("fremantle")))
//        return Maemo5;
//    if (name.startsWith(QLatin1String("harmattan")))
//        return Maemo6;
//    qWarning("Unknown Maemo version!");
//    return static_cast<AndroidVersion>(-1);
}

void AndroidToolChain::setMaddeRoot() const
{
    QDir dir(targetRoot());
    dir.cdUp(); dir.cdUp();

    m_maddeInitialized = true;
    m_maddeRoot = dir.absolutePath();
}

void AndroidToolChain::setSysroot() const
{
    QFile file(QDir::cleanPath(targetRoot()) + QLatin1String("/information"));
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString &line = stream.readLine().trimmed();
            const QStringList &list = line.split(QLatin1Char(' '));
            if (list.count() <= 1)
                continue;
            if (list.at(0) == QLatin1String("sysroot")) {
                m_sysrootRoot = maddeRoot() + QLatin1String("/sysroots/")
                    + list.at(1);
            }
        }
    }

    m_sysrootInitialized = true;
}
