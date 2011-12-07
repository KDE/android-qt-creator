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

#include "executefilter.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/variablemanager.h>

#include <QtGui/QMessageBox>

using namespace Core;
using namespace Locator;
using namespace Locator::Internal;

ExecuteFilter::ExecuteFilter()
{
    setShortcutString(QString(QLatin1Char('!')));
    setIncludedByDefault(false);

    m_process = new Utils::QtcProcess(this);
    m_process->setEnvironment(Utils::Environment::systemEnvironment());
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this,
            SLOT(finished(int, QProcess::ExitStatus)));
    connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readStandardOutput()));
    connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(readStandardError()));

    m_runTimer.setSingleShot(true);
    connect(&m_runTimer, SIGNAL(timeout()), this, SLOT(runHeadCommand()));
}

QList<FilterEntry> ExecuteFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future,
                                             const QString &entry)
{
    QList<FilterEntry> value;
    if (!entry.isEmpty()) // avoid empty entry
        value.append(FilterEntry(this, entry, QVariant()));
    QList<FilterEntry> others;
    foreach (const QString& i, m_commandHistory) {
        if (future.isCanceled())
            break;
        if (i == entry) // avoid repeated entry
            continue;
        if (i.startsWith(entry))
            value.append(FilterEntry(this, i, QVariant()));
        else
            others.append(FilterEntry(this, i, QVariant()));
    }
    value.append(others);
    return value;
}

void ExecuteFilter::accept(FilterEntry selection) const
{
    ExecuteFilter *p = const_cast<ExecuteFilter *>(this);

    const QString value = selection.displayName.trimmed();
    const int index = m_commandHistory.indexOf(value);
    if (index != -1 && index != 0)
        p->m_commandHistory.removeAt(index);
    if (index != 0)
        p->m_commandHistory.prepend(value);

    VariableManager *vm = Core::VariableManager::instance();
    bool found;
    QString workingDirectory = vm->value(QLatin1String("CurrentDocument:Path"), &found);
    if (!found || workingDirectory.isEmpty())
        workingDirectory = vm->value(QLatin1String("CurrentProject:Path"), &found);

    ExecuteData d;
    d.workingDirectory = workingDirectory;
    const int pos = value.indexOf(QLatin1Char(' '));
    if (pos == -1) {
        d.executable = value;
    } else {
        d.executable = value.left(pos);
        d.arguments = value.right(value.length() - pos - 1);
    }

    if (m_process->state() != QProcess::NotRunning) {
        const QString info(tr("Previous command is still running ('%1').\nDo you want to kill it?")
                           .arg(p->headCommand()));
        int r = QMessageBox::question(0, tr("Kill previous process?"), info,
                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                      QMessageBox::Yes);
        if (r == QMessageBox::Yes)
            m_process->kill();
        if (r != QMessageBox::Cancel)
            p->m_taskQueue.enqueue(d);
        return;
    }

    p->m_taskQueue.enqueue(d);
    p->runHeadCommand();
}

void ExecuteFilter::finished(int exitCode, QProcess::ExitStatus status)
{
    QString log = QString("'%1' ").arg(headCommand());
    if (status == QProcess::NormalExit && exitCode == 0)
        ICore::instance()->messageManager()->printToOutputPane(log + tr("finished"), true);
    else
        ICore::instance()->messageManager()->printToOutputPane(log + tr("failed"), true);

    m_taskQueue.dequeue();
    if (!m_taskQueue.isEmpty())
        m_runTimer.start(500);
}

void ExecuteFilter::readStandardOutput()
{
    QByteArray data = m_process->readAllStandardOutput();
    ICore::instance()->messageManager()->printToOutputPane(data, true);
}

void ExecuteFilter::readStandardError()
{
    QByteArray data = m_process->readAllStandardError();
    ICore::instance()->messageManager()->printToOutputPane(data, true);
}

void ExecuteFilter::runHeadCommand()
{
    if (!m_taskQueue.isEmpty()) {
        const ExecuteData &d = m_taskQueue.head();
        const QString fullPath = Utils::Environment::systemEnvironment().searchInPath(d.executable);
        if (fullPath.isEmpty()) {
            const QString log = tr("Could not find executable for '%1'").arg(d.executable);
            ICore::instance()->messageManager()->printToOutputPane(log, true);
            m_taskQueue.dequeue();
            runHeadCommand();
            return;
        }
        QString log(tr("Starting command '%1'").arg(headCommand()));
        ICore::instance()->messageManager()->printToOutputPane(log, true);
        m_process->setWorkingDirectory(d.workingDirectory);
        m_process->setCommand(fullPath, d.arguments);
        m_process->start();
        m_process->closeWriteChannel();
    }
}

QString ExecuteFilter::headCommand() const
{
    if (m_taskQueue.isEmpty())
        return QString();
    const ExecuteData &data = m_taskQueue.head();
    if (data.arguments.isEmpty())
        return data.executable;
    else
        return data.executable + QLatin1Char(' ') + data.arguments;
}
