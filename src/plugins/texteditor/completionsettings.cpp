/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "completionsettings.h"

#include <QtCore/QSettings>

static const char * const groupPostfix = "Completion";
static const char * const caseSensitivityKey = "CaseSensitivity";
static const char * const completionTriggerKey = "CompletionTrigger";
static const char * const autoInsertBracesKey = "AutoInsertBraces";
static const char * const partiallyCompleteKey = "PartiallyComplete";
static const char * const spaceAfterFunctionNameKey = "SpaceAfterFunctionName";

using namespace TextEditor;

CompletionSettings::CompletionSettings()
    : m_caseSensitivity(CaseInsensitive)
    , m_completionTrigger(AutomaticCompletion)
    , m_autoInsertBrackets(true)
    , m_partiallyComplete(true)
    , m_spaceAfterFunctionName(false)
{
}

void CompletionSettings::toSettings(const QString &category, QSettings *s) const
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);

    s->beginGroup(group);
    s->setValue(QLatin1String(caseSensitivityKey), (int) m_caseSensitivity);
    s->setValue(QLatin1String(completionTriggerKey), (int) m_completionTrigger);
    s->setValue(QLatin1String(autoInsertBracesKey), m_autoInsertBrackets);
    s->setValue(QLatin1String(partiallyCompleteKey), m_partiallyComplete);
    s->setValue(QLatin1String(spaceAfterFunctionNameKey), m_spaceAfterFunctionName);
    s->endGroup();
}

void CompletionSettings::fromSettings(const QString &category, const QSettings *s)
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);
    group += QLatin1Char('/');

    *this = CompletionSettings(); // Assign defaults

    m_caseSensitivity = (CaseSensitivity) s->value(group + QLatin1String(caseSensitivityKey), m_caseSensitivity).toInt();
    m_completionTrigger = (CompletionTrigger) s->value(group + QLatin1String(completionTriggerKey), m_completionTrigger).toInt();
    m_autoInsertBrackets = s->value(group + QLatin1String(autoInsertBracesKey), m_autoInsertBrackets).toBool();
    m_partiallyComplete = s->value(group + QLatin1String(partiallyCompleteKey), m_partiallyComplete).toBool();
    m_spaceAfterFunctionName = s->value(group + QLatin1String(spaceAfterFunctionNameKey), m_spaceAfterFunctionName).toBool();
}

bool CompletionSettings::equals(const CompletionSettings &cs) const
{
    return m_caseSensitivity == cs.m_caseSensitivity
        && m_completionTrigger == cs.m_completionTrigger
        && m_autoInsertBrackets == cs.m_autoInsertBrackets
        && m_partiallyComplete == cs.m_partiallyComplete
        && m_spaceAfterFunctionName == cs.m_spaceAfterFunctionName
        ;
}
