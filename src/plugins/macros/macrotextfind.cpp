/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
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

#include "macrotextfind.h"

#include <utils/qtcassert.h>

using namespace Macros;
using namespace Macros::Internal;

MacroTextFind::MacroTextFind(Find::IFindSupport *currentFind):
    Find::IFindSupport(),
    m_currentFind(currentFind)
{
}

bool MacroTextFind::supportsReplace() const
{
    QTC_ASSERT(m_currentFind, return false);
    return m_currentFind->supportsReplace();
}

Find::FindFlags MacroTextFind::supportedFindFlags() const
{
    QTC_ASSERT(m_currentFind, return 0);
    return m_currentFind->supportedFindFlags();
}

void MacroTextFind::resetIncrementalSearch()
{
    QTC_ASSERT(m_currentFind, return);
    m_currentFind->resetIncrementalSearch();
    emit incrementalSearchReseted();
}

void MacroTextFind::clearResults()
{
    QTC_ASSERT(m_currentFind, return);
    m_currentFind->clearResults();
}

QString MacroTextFind::currentFindString() const
{
    QTC_ASSERT(m_currentFind, return QString());
    return m_currentFind->currentFindString();
}

QString MacroTextFind::completedFindString() const
{
    QTC_ASSERT(m_currentFind, return QString());
    return m_currentFind->completedFindString();
}

void MacroTextFind::highlightAll(const QString &txt, Find::FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return);
    m_currentFind->highlightAll(txt, findFlags);
}

Find::IFindSupport::Result MacroTextFind::findIncremental(const QString &txt, Find::FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return IFindSupport::NotFound);
    Find::IFindSupport::Result result = m_currentFind->findIncremental(txt, findFlags);
    if (result == Found)
        emit incrementalFound(txt, findFlags);
    return result;
}

Find::IFindSupport::Result MacroTextFind::findStep(const QString &txt, Find::FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return IFindSupport::NotFound);
    Find::IFindSupport::Result result = m_currentFind->findStep(txt, findFlags);
    if (result == Found)
        emit stepFound(txt, findFlags);
    return result;
}

void MacroTextFind::replace(const QString &before, const QString &after, Find::FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return);
    m_currentFind->replace(before, after, findFlags);
    emit replaced(before, after, findFlags);
}

bool MacroTextFind::replaceStep(const QString &before, const QString &after, Find::FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return false);
    bool result = m_currentFind->replaceStep(before, after, findFlags);
    emit stepReplaced(before, after, findFlags);
    return result;
}

int MacroTextFind::replaceAll(const QString &before, const QString &after, Find::FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return 0);
    int result = m_currentFind->replaceAll(before, after, findFlags);
    emit allReplaced(before, after, findFlags);
    return result;
}

void MacroTextFind::defineFindScope()
{
    QTC_ASSERT(m_currentFind, return);
    m_currentFind->defineFindScope();
}

void MacroTextFind::clearFindScope()
{
    QTC_ASSERT(m_currentFind, return);
    m_currentFind->clearFindScope();
}
