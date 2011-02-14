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
#include "androidconfigurations.h"

#include <QtCore/QDir>
#include <QtCore/QStringBuilder>
#include <QtCore/QTextStream>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

AndroidToolChain::AndroidToolChain(const QString &gccPath)
    : GccToolChain(gccPath)
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
#warning TODO this vars should be configurable in projects -> build tab
#warning TODO invalidate all .pro files !!!
    // this env vars are used by qmake mkspecs to generate makefiles (check QTDIR/mkspecs/android-g++/qmake.conf for more info)
    env.set(QLatin1String("ANDROID_NDK_ROOT")
                     ,QDir::toNativeSeparators(AndroidConfigurations::instance().config().NDKLocation));
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_PREFIX")
                     ,AndroidConfigurations::instance().config().NDKToolchainVersion.left(AndroidConfigurations::instance().config().NDKToolchainVersion.lastIndexOf('-')));
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_VERSION")
                     ,AndroidConfigurations::instance().config().NDKToolchainVersion.mid(AndroidConfigurations::instance().config().NDKToolchainVersion.lastIndexOf('-')+1));
}

QString AndroidToolChain::makeCommand() const
{
    return QLatin1String("make" ANDROID_EXEC_SUFFIX);
}

bool AndroidToolChain::equals(const ToolChain *other) const
{
    return other->type() == type();
}
