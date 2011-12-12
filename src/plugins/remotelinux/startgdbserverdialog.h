/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef STARTGDBSERVERDIALOG_H
#define STARTGDBSERVERDIALOG_H

#include "remotelinux_export.h"

#include <QtGui/QDialog>

namespace RemoteLinux {

namespace Internal { class StartGdbServerDialogPrivate; }

class REMOTELINUX_EXPORT StartGdbServerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartGdbServerDialog(QWidget *parent = 0);
    ~StartGdbServerDialog();

    void startGdbServer();
    void attachToRemoteProcess();

signals:
    void processAborted();

private slots:
    void attachToDevice(int index);
    void handleRemoteError(const QString &errorMessage);
    void handleProcessListUpdated();
    void updateProcessList();
    void attachToProcess();
    void handleProcessKilled();
    void updateButtons();
    void portGathererError(const QString &errorMessage);
    void portListReady();

    void handleProcessClosed(int);
    void handleProcessErrorOutput(const QByteArray &data);
    void handleProcessOutputAvailable(const QByteArray &data);
    void handleProcessStarted();
    void handleConnectionError();

private:
    void startGdbServerOnPort(int port, int pid);
    void reportOpenPort(int port);
    void reportFailure();
    void logMessage(const QString &line);
    Internal::StartGdbServerDialogPrivate *d;
};

} // namespace RemoteLinux

#endif // STARTGDBSERVERDIALOG_H
