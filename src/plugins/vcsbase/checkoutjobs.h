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

#ifndef CHECKOUTJOB_H
#define CHECKOUTJOB_H

#include "vcsbase_global.h"

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>

namespace VcsBase {

namespace Internal { class ProcessCheckoutJobPrivate; }

/* Abstract base class for a job creating an initial project checkout.
 * It should be something that runs in the background producing log
 * messages. */

class VCSBASE_EXPORT AbstractCheckoutJob : public QObject
{
    Q_OBJECT

public:
    virtual void start() = 0;
    virtual void cancel() = 0;

protected:
    explicit AbstractCheckoutJob(QObject *parent = 0);

signals:
    void succeeded();
    void failed(const QString &why);
    void output(const QString &what);
};

class VCSBASE_EXPORT ProcessCheckoutJob : public AbstractCheckoutJob
{
    Q_OBJECT

public:
    explicit ProcessCheckoutJob(QObject *parent = 0);
    ~ProcessCheckoutJob();

    void addStep(const QString &binary,
                 const QStringList &args,
                 const QString &workingDirectory = QString(),
                 const QProcessEnvironment &env = QProcessEnvironment::systemEnvironment());

    void start();
    void cancel();

private slots:
    void slotError(QProcess::ProcessError error);
    void slotFinished (int exitCode, QProcess::ExitStatus exitStatus);
    void slotOutput();
    void slotNext();

private:
    Internal::ProcessCheckoutJobPrivate *const d;
};

} // namespace VcsBase

#endif // CHECKOUTJOB_H
