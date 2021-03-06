/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidplugin.h"

#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androiddeploystepfactory.h"
#include "androiddevice.h"
#include "androiddevicefactory.h"
#include "androidconfigurations.h"
#include "androidmanager.h"
#include "androidpackagecreationfactory.h"
#include "androidpackageinstallationfactory.h"
#include "androidrunfactories.h"
#include "androidsettingspage.h"
#include "androidtoolchain.h"
#include "androidqtversionfactory.h"
#include "androiddeployconfiguration.h"

#include <QtPlugin>

#include <projectexplorer/devicesupport/devicemanager.h>

namespace Android {

AndroidPlugin::AndroidPlugin()
{ }

AndroidPlugin::~AndroidPlugin()
{ }

bool AndroidPlugin::initialize(const QStringList &arguments,
                               QString *error_message)
{
    Q_UNUSED(arguments);
    Q_UNUSED(error_message);

    Internal::AndroidConfigurations::instance(this);

    new Internal::AndroidManager(this);

    addAutoReleasedObject(new Internal::AndroidRunControlFactory);
    addAutoReleasedObject(new Internal::AndroidRunConfigurationFactory);
    addAutoReleasedObject(new Internal::AndroidPackageInstallationFactory);
    addAutoReleasedObject(new Internal::AndroidPackageCreationFactory);
    addAutoReleasedObject(new Internal::AndroidDeployStepFactory);
    addAutoReleasedObject(new Internal::AndroidSettingsPage);
    addAutoReleasedObject(new Internal::AndroidQtVersionFactory);
    addAutoReleasedObject(new Internal::AndroidToolChainFactory);
    addAutoReleasedObject(new Internal::AndroidDeployConfigurationFactory);
    addAutoReleasedObject(new Internal::AndroidDeviceFactory);
    ProjectExplorer::DeviceManager *dm = ProjectExplorer::DeviceManager::instance();
    if (dm->find(Core::Id(Constants::ANDROID_DEVICE_ID)).isNull())
        dm->addDevice(ProjectExplorer::IDevice::Ptr(new Internal::AndroidDevice));
    return true;
}

void AndroidPlugin::extensionsInitialized()
{ }

} // namespace Android

Q_EXPORT_PLUGIN(Android::AndroidPlugin)
