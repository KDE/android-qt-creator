/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#include "memcheckengine.h"

#include "valgrindsettings.h"

#include <analyzerbase/analyzersettings.h>

#include <valgrind/xmlprotocol/error.h>
#include <valgrind/xmlprotocol/status.h>

#include <utils/qtcassert.h>

using namespace Analyzer;
using namespace Valgrind::XmlProtocol;

namespace Valgrind {
namespace Internal {

MemcheckEngine::MemcheckEngine(IAnalyzerTool *tool, const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration)
    : ValgrindEngine(tool, sp, runConfiguration)
{
    connect(&m_parser, SIGNAL(error(Valgrind::XmlProtocol::Error)),
            SIGNAL(parserError(Valgrind::XmlProtocol::Error)));
    connect(&m_parser, SIGNAL(suppressionCount(QString,qint64)),
            SIGNAL(suppressionCount(QString,qint64)));
    connect(&m_parser, SIGNAL(internalError(QString)),
            SIGNAL(internalParserError(QString)));
    connect(&m_parser, SIGNAL(status(Valgrind::XmlProtocol::Status)),
            SLOT(status(Valgrind::XmlProtocol::Status)));

    m_progress->setProgressRange(0, XmlProtocol::Status::Finished + 1);
}

QString MemcheckEngine::progressTitle() const
{
    return tr("Analyzing Memory");
}

Valgrind::ValgrindRunner *MemcheckEngine::runner()
{
    return &m_runner;
}

bool MemcheckEngine::start()
{
    m_runner.setParser(&m_parser);

    emit outputReceived(tr("Analyzing memory of %1\n").arg(executable()),
                        Utils::NormalMessageFormat);
    return ValgrindEngine::start();
}

void MemcheckEngine::stop()
{
    disconnect(&m_parser, SIGNAL(internalError(QString)),
               this, SIGNAL(internalParserError(QString)));
    ValgrindEngine::stop();
}

QStringList MemcheckEngine::toolArguments() const
{
    QStringList arguments;
    arguments << QLatin1String("--gen-suppressions=all");

    ValgrindBaseSettings *memcheckSettings = m_settings->subConfig<ValgrindBaseSettings>();
    QTC_ASSERT(memcheckSettings, return arguments);

    if (memcheckSettings->trackOrigins())
        arguments << QLatin1String("--track-origins=yes");

    foreach (const QString &file, memcheckSettings->suppressionFiles())
        arguments << QString("--suppressions=%1").arg(file);

    arguments << QString("--num-callers=%1").arg(memcheckSettings->numCallers());
    return arguments;
}

QStringList MemcheckEngine::suppressionFiles() const
{
    return m_settings->subConfig<ValgrindBaseSettings>()->suppressionFiles();
}

void MemcheckEngine::status(const Status &status)
{
    m_progress->setProgressValue(status.state() + 1);
}

void MemcheckEngine::receiveLogMessage(const QByteArray &b)
{
    QString error = QString::fromLocal8Bit(b);
    // workaround https://bugs.kde.org/show_bug.cgi?id=255888
    error.remove(QRegExp("==*== </valgrindoutput>", Qt::CaseSensitive, QRegExp::Wildcard));

    error = error.trimmed();

    if (error.isEmpty())
        return;

    stop();

    QString file;
    int line = -1;

    const QRegExp suppressionError(QLatin1String("in suppressions file \"([^\"]+)\" near line (\\d+)"),
                                   Qt::CaseSensitive, QRegExp::RegExp2);
    if (suppressionError.indexIn(error) != -1) {
        file = suppressionError.cap(1);
        line = suppressionError.cap(2).toInt();
    }

    emit taskToBeAdded(ProjectExplorer::Task::Error, error, file, line);
}

} // namespace Internal
} // namespace Valgrind
