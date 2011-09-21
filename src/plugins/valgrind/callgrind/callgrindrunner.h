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

#ifndef VALGRIND_CALLGRIND_CALLGRINDRUNNER_H
#define VALGRIND_CALLGRIND_CALLGRINDRUNNER_H

#include <valgrindrunner.h>

#include "callgrindcontroller.h"

namespace Valgrind {
namespace Callgrind {

class Parser;
class CallgrindController;

class CallgrindRunner : public ValgrindRunner
{
    Q_OBJECT

public:
    explicit CallgrindRunner(QObject *parent = 0);

    Parser *parser() const;

    CallgrindController *controller() const;

    bool isPaused() const;

    virtual void start();
    virtual void startRemotely(const Utils::SshConnectionParameters &sshParams);

signals:
    void statusMessage(const QString &message);

private slots:
    void localParseDataAvailable(const QString &file);

    void controllerFinished(Valgrind::Callgrind::CallgrindController::Option);

    void processFinished(int, QProcess::ExitStatus);

private:
    void triggerParse();

    QString tool() const;

    CallgrindController *m_controller;
    Parser *m_parser;

    bool m_paused;
};

} // namespace Callgrind
} // namespace Valgrind

#endif // VALGRIND_CALLGRIND_CALLGRINDRUNNER_H
