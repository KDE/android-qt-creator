/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

namespace Utils {
class SshConnection;
class SshConnectionParameters;
class SshRemoteProcess;
}

QT_BEGIN_NAMESPACE
class QByteArray;
class QFile;
class QString;
QT_END_NAMESPACE

class Shell : public QObject
{
    Q_OBJECT
public:
    Shell(const Utils::SshConnectionParameters &parameters, QObject *parent = 0);
    ~Shell();

    void run();

private slots:
    void handleConnected();
    void handleConnectionError();
    void handleRemoteStdout(const QByteArray &output);
    void handleRemoteStderr(const QByteArray &output);
    void handleShellMessage(const QString &message);
    void handleChannelClosed(int exitStatus);
    void handleShellStarted();
    void handleStdin();

private:
    QSharedPointer<Utils::SshConnection> m_connection;
    QSharedPointer<Utils::SshRemoteProcess> m_shell;
    QFile * const m_stdin;
};
