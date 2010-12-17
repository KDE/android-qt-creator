/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "s60runconfigbluetoothstarter.h"
#include <symbianutils/bluetoothlistener.h>
#include <symbianutils/symbiandevicemanager.h>
#include <symbianutils/trkdevice.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

namespace Qt4ProjectManager {
namespace Internal {

S60RunConfigBluetoothStarter::S60RunConfigBluetoothStarter(const  TrkDevicePtr& trkDevice, QObject *parent) :
    trk::AbstractBluetoothStarter(trkDevice, parent)
{
}

trk::BluetoothListener *S60RunConfigBluetoothStarter::createListener()
{
    Core::ICore *core = Core::ICore::instance();
    trk::BluetoothListener *rc = new trk::BluetoothListener(core);
    rc->setMode(trk::BluetoothListener::Listen);
    connect(rc, SIGNAL(message(QString)), core->messageManager(), SLOT(printToOutputPane(QString)));
    return rc;
}

trk::PromptStartCommunicationResult
S60RunConfigBluetoothStarter::startCommunication(const TrkDevicePtr &trkDevice,
                                                 QWidget *msgBoxParent,
                                                 QString *errorMessage)
{
    // Bluetooth?
    if (trkDevice->serialFrame()) {
        BaseCommunicationStarter serialStarter(trkDevice);
        return trk::promptStartSerial(serialStarter, msgBoxParent, errorMessage);
    }
    S60RunConfigBluetoothStarter bluetoothStarter(trkDevice);
    return trk::promptStartBluetooth(bluetoothStarter, msgBoxParent, errorMessage);
}

} // namespace Internal
} // namespace Qt4ProjectManager
