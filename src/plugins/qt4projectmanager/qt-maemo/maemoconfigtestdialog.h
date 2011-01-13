/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAEMOCONFIGTESTDIALOG_H
#define MAEMOCONFIGTESTDIALOG_H

#include <QtCore/QSharedPointer>
#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
class QPushButton;
class Ui_MaemoConfigTestDialog;
QT_END_NAMESPACE

namespace Core {
    class SshRemoteProcessRunner;
} // namespace Core

namespace Qt4ProjectManager {
namespace Internal {

class MaemoDeviceConfig;
class MaemoUsedPortsGatherer;

/**
 * A dialog that runs a test of a device configuration.
 */
class MaemoConfigTestDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MaemoConfigTestDialog(const MaemoDeviceConfig &config, QWidget *parent = 0);
    ~MaemoConfigTestDialog();

private slots:
    void stopConfigTest();
    void processSshOutput(const QByteArray &output);
    void handleConnectionError();
    void handleTestProcessFinished(int exitStatus);
    void handlePortListReady();
    void handlePortListFailure(const QString &errMsg);

private:
    void startConfigTest();
    QString parseTestOutput();
    void handleGeneralTestResult(int exitStatus);
    void handleMadDeveloperTestResult(int exitStatus);
    void finish();

    Ui_MaemoConfigTestDialog *m_ui;
    QPushButton *m_closeButton;

    const MaemoDeviceConfig &m_config;
    QSharedPointer<Core::SshRemoteProcessRunner> m_testProcessRunner;
    QString m_deviceTestOutput;
    bool m_qtVersionOk;
    MaemoUsedPortsGatherer *const m_portsGatherer;

    enum DeviceTest { GeneralTest, MadDeveloperTest };
    DeviceTest m_currentTest;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOCONFIGTESTDIALOG_H
