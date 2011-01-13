/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "bluetoothlistener_gui.h"
#include "bluetoothlistener.h"
#include "communicationstarter.h"

#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

namespace trk {

SYMBIANUTILS_EXPORT PromptStartCommunicationResult
    promptStartCommunication(BaseCommunicationStarter &starter,
                             const QString &msgBoxTitle,
                             const QString &msgBoxText,
                             QWidget *msgBoxParent,
                             QString *errorMessage)
{
    errorMessage->clear();
    // Initial connection attempt.
    switch (starter.start()) {
    case BaseCommunicationStarter::Started:
        break;
    case BaseCommunicationStarter::ConnectionSucceeded:
        return PromptStartCommunicationConnected;
    case BaseCommunicationStarter::StartError:
        *errorMessage = starter.errorString();
        return PromptStartCommunicationError;
    }
    // Run the starter with the event loop of a message box, have the box
    // closed by the signals of the starter.
    QMessageBox messageBox(QMessageBox::Information, msgBoxTitle, msgBoxText, QMessageBox::Cancel, msgBoxParent);
    QObject::connect(&starter, SIGNAL(connected()), &messageBox, SLOT(close()));
    QObject::connect(&starter, SIGNAL(timeout()), &messageBox, SLOT(close()));
    messageBox.exec();
    // Only starter.state() is reliable here to obtain the state.
    switch (starter.state()) {
    case AbstractBluetoothStarter::Running:
        *errorMessage = QCoreApplication::translate("trk::promptStartCommunication", "Connection on %1 canceled.").arg(starter.device());
        return PromptStartCommunicationCanceled;
    case AbstractBluetoothStarter::TimedOut:
        *errorMessage = starter.errorString();
        return PromptStartCommunicationError;
    case AbstractBluetoothStarter::Connected:
        break;
    }
    return PromptStartCommunicationConnected;
}

SYMBIANUTILS_EXPORT PromptStartCommunicationResult
    promptStartSerial(BaseCommunicationStarter &starter,
                         QWidget *msgBoxParent,
                         QString *errorMessage)
{
    const QString title = QCoreApplication::translate("trk::promptStartCommunication", "Waiting for App TRK");
    const QString message = QCoreApplication::translate("trk::promptStartCommunication", "Waiting for App TRK to start on %1...").arg(starter.device());
    return promptStartCommunication(starter, title, message, msgBoxParent, errorMessage);
}

SYMBIANUTILS_EXPORT PromptStartCommunicationResult
    promptStartBluetooth(BaseCommunicationStarter &starter,
                         QWidget *msgBoxParent,
                         QString *errorMessage)
{
    const QString title = QCoreApplication::translate("trk::promptStartCommunication", "Waiting for Bluetooth Connection");
    const QString message = QCoreApplication::translate("trk::promptStartCommunication", "Connecting to %1...").arg(starter.device());
    return promptStartCommunication(starter, title, message, msgBoxParent, errorMessage);
}

} // namespace trk
