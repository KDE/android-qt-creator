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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "androidmanager.h"

#include "androidconstants.h"
#include "androiddeploystepfactory.h"
#include "androidconfigurations.h"
#include "androidpackagecreationfactory.h"
#include "androidrunfactories.h"
#include "androidsettingspage.h"
#include "androidtoolchain.h"
#include "qt4androidtargetfactory.h"

#include <extensionsystem/pluginmanager.h>
#include <qt4projectmanager/qtversionmanager.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

using namespace ExtensionSystem;
using namespace ProjectExplorer;

namespace Qt4ProjectManager {
    namespace Internal {

AndroidManager *AndroidManager::m_instance = 0;

AndroidManager::AndroidManager()
    : QObject(0)
    , m_runControlFactory(new AndroidRunControlFactory(this))
    , m_runConfigurationFactory(new AndroidRunConfigurationFactory(this))
    , m_packageCreationFactory(new AndroidPackageCreationFactory(this))
    , m_deployStepFactory(new AndroidDeployStepFactory(this))
    , m_settingsPage(new AndroidSettingsPage(this))
    , m_androidTargetFactory(new Qt4AndroidTargetFactory())
{
    Q_ASSERT(!m_instance);

    m_instance = this;
    AndroidConfigurations::instance(this);

    PluginManager *pluginManager = PluginManager::instance();
    pluginManager->addObject(m_runControlFactory);
    pluginManager->addObject(m_runConfigurationFactory);
    pluginManager->addObject(m_packageCreationFactory);
    pluginManager->addObject(m_deployStepFactory);
    pluginManager->addObject(m_settingsPage);
    pluginManager->addObject(m_androidTargetFactory);
}

AndroidManager::~AndroidManager()
{
    PluginManager *pluginManager = PluginManager::instance();
    pluginManager->removeObject(m_runControlFactory);
    pluginManager->removeObject(m_runConfigurationFactory);
    pluginManager->removeObject(m_deployStepFactory);
    pluginManager->removeObject(m_packageCreationFactory);
    pluginManager->removeObject(m_settingsPage);
    pluginManager->removeObject(m_androidTargetFactory);

    m_instance = 0;
}

AndroidManager &AndroidManager::instance()
{
    Q_ASSERT(m_instance);
    return *m_instance;
}

bool AndroidManager::isValidAndroidQtVersion(const QtVersion *version) const
{
    return version->mkspec().contains("android-g++");
}

ToolChain* AndroidManager::androidToolChain() const
{
#warning TODO FIX Android
    return new AndroidToolChain(AndroidConfigurations::instance().config().NDKLocation +"/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-gcc");
}

    } // namespace Internal
} // namespace Qt4ProjectManager
