/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
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

#ifndef MAEMOSSHRUNNER_H
#define MAEMOSSHRUNNER_H

#include "androidconfigurations.h"

#include <utils/environment.h>

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QProcess>


namespace Qt4ProjectManager {
namespace Internal {
class AndroidRunConfiguration;

class AndroidRunner : public QObject
{
    Q_OBJECT
public:
    AndroidRunner(QObject *parent, AndroidRunConfiguration *runConfig,
        bool debugging);
    ~AndroidRunner();

    QString displayName() const;

public slots:
    void start();
    void stop();


signals:
    void remoteProcessStarted(int gdbServerPort=-1, int qmlPort=-1);
    void remoteProcessFinished(const QString &errString="");

    void remoteOutput(const QByteArray &output);
    void remoteErrorOutput(const QByteArray &output);

private slots:
    void killPID();
    void checkPID();
    void logcatReadStandardError();
    void logcatReadStandardOutput();

private:
    int m_exitStatus;
    bool    m_debugingMode;
    QProcess m_adbLogcatProcess;
    QByteArray m_logcat;
    QString m_intentName;
    QString m_packageName;
    QString m_deviceSerialNumber;
    qint64 m_processPID;
    qint64 m_gdbserverPID;
    QTimer m_checkPIDTimer;
    AndroidRunConfiguration *m_runConfig;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOSSHRUNNER_H
