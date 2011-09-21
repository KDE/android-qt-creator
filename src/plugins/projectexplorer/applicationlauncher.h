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

#ifndef APPLICATIONLAUNCHER_H
#define APPLICATIONLAUNCHER_H

#include "projectexplorer_export.h"

#include <utils/outputformat.h>

#include <QtCore/QProcess>

namespace Utils {
class Environment;
}

namespace ProjectExplorer {
struct ApplicationLauncherPrivate;

// Documentation inside.
class PROJECTEXPLORER_EXPORT ApplicationLauncher : public QObject
{
    Q_OBJECT

public:
    enum Mode {
        Console,
        Gui
    };

    explicit ApplicationLauncher(QObject *parent = 0);
    ~ApplicationLauncher();

    void setWorkingDirectory(const QString &dir);
    void setEnvironment(const Utils::Environment &env);

    void start(Mode mode, const QString &program,
               const QString &args = QString());
    void stop();
    bool isRunning() const;
    qint64 applicationPID() const;

    static QString msgWinCannotRetrieveDebuggingOutput();

signals:
    void appendMessage(const QString &message, Utils::OutputFormat format);
    void processStarted();
    void processExited(int exitCode);
    void bringToForegroundRequested(qint64 pid);

private slots:
    void processStopped();
    void guiProcessError();
    void consoleProcessError(const QString &error);
    void readStandardOutput();
    void readStandardError();
#ifdef Q_OS_WIN
    void cannotRetrieveDebugOutput();
    void checkDebugOutput(qint64 pid, const QString &message);
#endif
    void processDone(int, QProcess::ExitStatus);
    void bringToForeground();

private:
    ApplicationLauncherPrivate *d;
};

} // namespace ProjectExplorer

#endif // APPLICATIONLAUNCHER_H
