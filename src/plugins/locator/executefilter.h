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

#ifndef EXECUTEFILTER_H
#define EXECUTEFILTER_H

#include "ilocatorfilter.h"

#include <utils/qtcprocess.h>

#include <QtCore/QQueue>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

namespace Locator {
namespace Internal {

class ExecuteFilter : public Locator::ILocatorFilter
{
    Q_OBJECT
    struct ExecuteData
    {
        QString executable;
        QString arguments;
        QString workingDirectory;
    };

public:
    ExecuteFilter();
    QString displayName() const { return tr("Execute Custom Commands"); }
    QString id() const { return "Execute custom commands"; }
    Locator::ILocatorFilter::Priority priority() const { return Locator::ILocatorFilter::Medium; }
    QList<Locator::FilterEntry> matchesFor(QFutureInterface<Locator::FilterEntry> &future,
                                           const QString &entry);
    void accept(Locator::FilterEntry selection) const;
    void refresh(QFutureInterface<void> &) {}

private slots:
    void finished(int exitCode, QProcess::ExitStatus status);
    void readStandardOutput();
    void readStandardError();
    void runHeadCommand();

private:
    QString headCommand() const;

private:
    QQueue<ExecuteData> m_taskQueue;
    QStringList m_commandHistory;
    Utils::QtcProcess *m_process;
    QTimer m_runTimer;
};

} // namespace Internal
} // namespace Locator

#endif // EXECUTEFILTER_H
