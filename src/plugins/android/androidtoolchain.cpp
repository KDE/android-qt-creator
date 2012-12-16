/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "androidtoolchain.h"
#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androidqtversion.h"

#include <debugger/debuggerkitinformation.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>

#include <projectexplorer/target.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/environment.h>

#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>

namespace Android {
namespace Internal {

using namespace ProjectExplorer;

static const char ANDROID_QT_VERSION_KEY[] = "Qt4ProjectManager.Android.QtVersion";


AndroidToolChain::AndroidToolChain(bool autodetected) :
    GccToolChain(QLatin1String(Constants::ANDROID_TOOLCHAIN_ID), autodetected),
    m_qtVersionId(-1)
{}

AndroidToolChain::AndroidToolChain(const AndroidToolChain &tc) :
    GccToolChain(tc),
    m_qtVersionId(tc.m_qtVersionId)
{ }

AndroidToolChain::~AndroidToolChain()
{ }

QString AndroidToolChain::type() const
{
    return QLatin1String("androidgcc");
}

QString AndroidToolChain::typeDisplayName() const
{
    return AndroidToolChainFactory::tr("Android GCC");
}

bool AndroidToolChain::isValid() const
{
    return GccToolChain::isValid() && m_qtVersionId >= 0 && targetAbi().isValid();
}

void AndroidToolChain::addToEnvironment(Utils::Environment &env) const
{

// TODO this vars should be configurable in projects -> build tab
// TODO invalidate all .pro files !!!

    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_PREFIX"), AndroidConfigurations::toolchainPrefix(targetAbi().architecture()));
    env.set(QLatin1String("ANDROID_NDK_TOOLS_PREFIX"), AndroidConfigurations::toolsPrefix(targetAbi().architecture()));
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_VERSION"), AndroidConfigurations::instance().config().ndkToolchainVersion);
}

bool AndroidToolChain::operator ==(const ToolChain &tc) const
{
    if (!ToolChain::operator ==(tc))
        return false;

    const AndroidToolChain *tcPtr = static_cast<const AndroidToolChain *>(&tc);
    return m_qtVersionId == tcPtr->m_qtVersionId;
}

ToolChainConfigWidget *AndroidToolChain::configurationWidget()
{
    return new AndroidToolChainConfigWidget(this);
}

QVariantMap AndroidToolChain::toMap() const
{
    QVariantMap result = GccToolChain::toMap();
    result.insert(QLatin1String(ANDROID_QT_VERSION_KEY), m_qtVersionId);
    return result;
}

bool AndroidToolChain::fromMap(const QVariantMap &data)
{
    if (!GccToolChain::fromMap(data))
        return false;

    m_qtVersionId = data.value(QLatin1String(ANDROID_QT_VERSION_KEY), -1).toInt();

    return isValid();
}

QList<Utils::FileName> AndroidToolChain::suggestedMkspecList() const
{
    return QList<Utils::FileName>()<< Utils::FileName::fromString(QLatin1String("android-g++"));
}

QString AndroidToolChain::makeCommand() const
{
#if defined(Q_OS_WIN)
    return QLatin1String("ma-make.exe");
#else
    return QLatin1String("make");
#endif
}

void AndroidToolChain::setQtVersionId(int id)
{
    if (id < 0) {
        setTargetAbi(Abi());
        m_qtVersionId = -1;
        toolChainUpdated();
        return;
    }

    QtSupport::BaseQtVersion *version = QtSupport::QtVersionManager::instance()->version(id);
    Q_ASSERT(version);
    m_qtVersionId = id;

    Q_ASSERT(version->qtAbis().count() == 1);
    setTargetAbi(version->qtAbis().at(0));

    toolChainUpdated();
    setDisplayName(AndroidToolChainFactory::tr("Android GCC for %1").arg(version->displayName()));
}

int AndroidToolChain::qtVersionId() const
{
    return m_qtVersionId;
}

QList<Abi> AndroidToolChain::detectSupportedAbis() const
{
    if (m_qtVersionId < 0)
        return QList<Abi>();

    AndroidQtVersion *aqv = dynamic_cast<AndroidQtVersion *>(QtSupport::QtVersionManager::instance()->version(m_qtVersionId));
    if (!aqv)
        return QList<Abi>();

    return aqv->qtAbis();
}

// --------------------------------------------------------------------------
// ToolChainConfigWidget
// --------------------------------------------------------------------------

AndroidToolChainConfigWidget::AndroidToolChainConfigWidget(AndroidToolChain *tc) :
   ToolChainConfigWidget(tc)
{
    QLabel *label = new QLabel(AndroidConfigurations::instance().config().ndkLocation.toUserOutput());
    m_mainLayout->addRow(tr("NDK Root:"), label);
}

// --------------------------------------------------------------------------
// ToolChainFactory
// --------------------------------------------------------------------------

AndroidToolChainFactory::AndroidToolChainFactory() :
    ToolChainFactory()
{ }

QString AndroidToolChainFactory::displayName() const
{
    return tr("Android GCC");
}

QString AndroidToolChainFactory::id() const
{
    return QLatin1String(Constants::ANDROID_TOOLCHAIN_ID);
}

QList<ToolChain *> AndroidToolChainFactory::autoDetect()
{
    QList<ToolChain *> result;

    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    connect(vm, SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SLOT(handleQtVersionChanges(QList<int>,QList<int>,QList<int>)));

    QList<int> versionList;
    foreach (QtSupport::BaseQtVersion *v, vm->versions())
        versionList.append(v->uniqueId());

    return createToolChainList(versionList);
}

bool AndroidToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::ANDROID_TOOLCHAIN_ID) + QLatin1Char(':'));
}

ToolChain *AndroidToolChainFactory::restore(const QVariantMap &data)
{
    AndroidToolChain *tc = new AndroidToolChain(false);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

void AndroidToolChainFactory::handleQtVersionChanges(const QList<int> &added, const QList<int> &removed, const QList<int> &changed)
{
    QList<int> changes;
    changes << added << removed << changed;
    ToolChainManager *tcm = ToolChainManager::instance();
    QList<ToolChain *> tcList = createToolChainList(changes);
    foreach (ToolChain *tc, tcList)
        tcm->registerToolChain(tc);
}

QList<ToolChain *> AndroidToolChainFactory::createToolChainList(const QList<int> &changes)
{
    ToolChainManager *tcm = ToolChainManager::instance();
    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    QList<ToolChain *> result;

    foreach (int i, changes) {
        QtSupport::BaseQtVersion *v = vm->version(i);
        QList<ToolChain *> toRemove;
        foreach (ToolChain *tc, tcm->toolChains()) {
            if (tc->id() != QLatin1String(Constants::ANDROID_TOOLCHAIN_ID))
                continue;
            AndroidToolChain *aTc = static_cast<AndroidToolChain *>(tc);
            if (aTc->qtVersionId() == i)
                toRemove.append(aTc);
        }
        foreach (ToolChain *tc, toRemove)
            tcm->deregisterToolChain(tc);

        const AndroidQtVersion * const aqv = dynamic_cast<AndroidQtVersion *>(v);
        if (!aqv || !aqv->isValid())
            continue;

        AndroidToolChain *aTc = new AndroidToolChain(true);
        aTc->setQtVersionId(i);
        aTc->setDisplayName(tr("Android GCC (%1-%2)")
                            .arg(Abi::toString(aTc->targetAbi().architecture()))
                            .arg(AndroidConfigurations::instance().config().ndkToolchainVersion));
        aTc->setCompilerCommand(AndroidConfigurations::instance().gccPath(aTc->targetAbi().architecture()));
        result.append(aTc);
    }
    QTimer::singleShot(10, this, SLOT(createDefaultProfiles())); // must be called after toolchain list is processed by toolchan manager
    return result;
}

class AndroidProfileMatcher : public ProjectExplorer::KitMatcher
{
public:
    AndroidProfileMatcher()
    {
        m_profile = 0;
    }

    virtual bool matches(const ProjectExplorer::Kit *p) const
    {
        return m_profile && p
                && ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(p) == ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(m_profile)
                && ProjectExplorer::DeviceKitInformation::deviceId(p) == ProjectExplorer::DeviceKitInformation::deviceId(m_profile)
                && QtSupport::QtKitInformation::qtVersion(p) == QtSupport::QtKitInformation::qtVersion(m_profile)
                && ProjectExplorer::ToolChainKitInformation::toolChain(p) == ProjectExplorer::ToolChainKitInformation::toolChain(m_profile)
                && Debugger::DebuggerKitInformation::debuggerCommand(p) == Debugger::DebuggerKitInformation::debuggerCommand(m_profile);
    }

    void setProfile(ProjectExplorer::Kit *profile)
    {
        m_profile = profile;
    }

private:
    ProjectExplorer::Kit *m_profile;
};

void AndroidToolChainFactory::createDefaultProfiles()
{
    AndroidProfileMatcher apm;
    QList<QtSupport::BaseQtVersion *> qtValidVesions = QtSupport::QtVersionManager::instance()->validVersions();
    foreach (QtSupport::BaseQtVersion * bqv, qtValidVesions)
    {
        AndroidQtVersion *aqv = dynamic_cast<AndroidQtVersion *>(bqv);
        if (aqv)
        {
            ProjectExplorer::ToolChain * atc = aqv->preferredToolChain(aqv->mkspec());
            if (atc && atc->isValid())
            {
                ProjectExplorer::Kit *p = new ProjectExplorer::Kit;
                p->setDisplayName(aqv->displayName());
                p->setIconPath(QLatin1String(Constants::ANDROID_ICON));
                ProjectExplorer::DeviceTypeKitInformation::setDeviceTypeId(p, Core::Id(Constants::ANDROID_DEVICE_TYPE));
                ProjectExplorer::DeviceKitInformation::setDeviceId(p, Core::Id(Constants::ANDROID_DEVICE_ID));
                QtSupport::QtKitInformation::setQtVersion(p, aqv);
                ProjectExplorer::ToolChainKitInformation::setToolChain(p, atc);
                Debugger::DebuggerKitInformation::setDebuggerCommand(p, AndroidConfigurations::instance().gdbPath(ProjectExplorer::Abi::ArmArchitecture));
                Debugger::DebuggerKitInformation::setEngineType(p, Debugger::GdbEngineType);
                apm.setProfile(p);
                ProjectExplorer::KitManager * pm = ProjectExplorer::KitManager::instance();
                if (pm->find(&apm))
                    delete p;
                else
                    pm->registerKit(p);
            }
        }
    }
}

} // namespace Internal
} // namespace Android
