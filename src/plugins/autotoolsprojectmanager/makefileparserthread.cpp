/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010-2011 Openismus GmbH.
**   Authors: Peter Penz (ppenz@openismus.com)
**            Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#include "makefileparserthread.h"

#include <QtCore/QMutexLocker>

using namespace AutotoolsProjectManager::Internal;

MakefileParserThread::MakefileParserThread(const QString &makefile) :
    QThread(),
    m_parser(makefile),
    m_mutex(),
    m_hasError(false),
    m_executable(),
    m_sources(),
    m_makefiles(),
    m_includePaths()
{
    connect(&m_parser, SIGNAL(status(QString)), this, SIGNAL(status(QString)));
}

QStringList MakefileParserThread::sources() const
{
    QMutexLocker locker(&m_mutex);
    return m_sources;
}

QStringList MakefileParserThread::makefiles() const
{
    QMutexLocker locker(&m_mutex);
    return m_makefiles;
}

QString MakefileParserThread::executable() const
{
    QMutexLocker locker(&m_mutex);
    return m_executable;
}

QStringList MakefileParserThread::includePaths() const
{
    QMutexLocker locker(&m_mutex);
    return m_includePaths;
}

bool MakefileParserThread::hasError() const
{
    QMutexLocker locker(&m_mutex);
    return m_hasError;
}

bool MakefileParserThread::isCanceled() const
{
    // MakefileParser::isCanceled() is thread-safe
    return m_parser.isCanceled();
}

void MakefileParserThread::cancel()
{
    m_parser.cancel();
}

void MakefileParserThread::run()
{
    const bool success = m_parser.parse();

    // Important: Start locking the mutex _after_ the parsing has been finished, as
    // this prevents long locks if the caller reads a value before the signal
    // finished() has been emitted.
    QMutexLocker locker(&m_mutex);
    m_hasError = !success;
    m_executable = m_parser.executable();
    m_sources = m_parser.sources();
    m_makefiles = m_parser.makefiles();
    m_includePaths = m_parser.includePaths();
}
