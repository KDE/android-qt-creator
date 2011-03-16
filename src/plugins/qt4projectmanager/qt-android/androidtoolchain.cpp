/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

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
#ifdef __GNUC__
#warning TODO this vars should be configurable in projects -> build tab
#warning TODO invalidate all .pro files !!!
#endif
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
