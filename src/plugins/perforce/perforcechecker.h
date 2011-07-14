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

#ifndef PERFORCECHECKER_H
#define PERFORCECHECKER_H

#include <QtCore/QObject>
#include <QtCore/QProcess>

namespace Perforce {
namespace Internal {

// Perforce checker:  Calls perforce asynchronously to do
// a check of the configuration and emits signals with the top level or
// an error message.

class PerforceChecker : public QObject
{
    Q_OBJECT
public:
    explicit PerforceChecker(QObject *parent = 0);
    virtual ~PerforceChecker();

public slots:
    void start(const QString &binary,
               const QStringList &basicArgs = QStringList(),
               int timeoutMS = -1);

    bool isRunning() const;

    bool useOverideCursor() const;
    void setUseOverideCursor(bool v);

signals:
    void succeeded(const QString &repositoryRoot);
    void failed(const QString &errorMessage);

private slots:
    void slotError(QProcess::ProcessError error);
    void slotFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void slotTimeOut();

private:
    void emitFailed(const QString &);
    void emitSucceeded(const QString &);
    void parseOutput(const QString &);
    inline void resetOverrideCursor();

    QProcess m_process;
    QString m_binary;
    int m_timeOutMS;
    bool m_timedOut;
    bool m_useOverideCursor;
    bool m_isOverrideCursor;
};

}
}

#endif // PERFORCECHECKER_H
