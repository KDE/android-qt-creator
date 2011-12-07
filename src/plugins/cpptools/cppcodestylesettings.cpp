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

#include "cppcodestylesettings.h"

#include <utils/settingsutils.h>

static const char *groupPostfix = "IndentSettings";
static const char *indentBlockBracesKey = "IndentBlockBraces";
static const char *indentBlockBodyKey = "IndentBlockBody";
static const char *indentClassBracesKey = "IndentClassBraces";
static const char *indentEnumBracesKey = "IndentEnumBraces";
static const char *indentNamespaceBracesKey = "IndentNamespaceBraces";
static const char *indentNamespaceBodyKey = "IndentNamespaceBody";
static const char *indentAccessSpecifiersKey = "IndentAccessSpecifiers";
static const char *indentDeclarationsRelativeToAccessSpecifiersKey = "IndentDeclarationsRelativeToAccessSpecifiers";
static const char *indentFunctionBodyKey = "IndentFunctionBody";
static const char *indentFunctionBracesKey = "IndentFunctionBraces";
static const char *indentSwitchLabelsKey = "IndentSwitchLabels";
static const char *indentStatementsRelativeToSwitchLabelsKey = "IndentStatementsRelativeToSwitchLabels";
static const char *indentBlocksRelativeToSwitchLabelsKey = "IndentBlocksRelativeToSwitchLabels";
static const char *indentControlFlowRelativeToSwitchLabelsKey = "IndentControlFlowRelativeToSwitchLabels";
static const char *extraPaddingForConditionsIfConfusingAlignKey = "ExtraPaddingForConditionsIfConfusingAlign";
static const char *alignAssignmentsKey = "AlignAssignments";

using namespace CppTools;

// ------------------ CppCodeStyleSettingsWidget

CppCodeStyleSettings::CppCodeStyleSettings() :
    indentBlockBraces(false)
  , indentBlockBody(true)
  , indentClassBraces(false)
  , indentEnumBraces(false)
  , indentNamespaceBraces(false)
  , indentNamespaceBody(false)
  , indentAccessSpecifiers(false)
  , indentDeclarationsRelativeToAccessSpecifiers(true)
  , indentFunctionBody(true)
  , indentFunctionBraces(false)
  , indentSwitchLabels(false)
  , indentStatementsRelativeToSwitchLabels(true)
  , indentBlocksRelativeToSwitchLabels(false)
  , indentControlFlowRelativeToSwitchLabels(true)
  , extraPaddingForConditionsIfConfusingAlign(true)
  , alignAssignments(false)
{
}

void CppCodeStyleSettings::toSettings(const QString &category, QSettings *s) const
{
    Utils::toSettings(QLatin1String(groupPostfix), category, s, this);
}

void CppCodeStyleSettings::fromSettings(const QString &category, const QSettings *s)
{
    *this = CppCodeStyleSettings(); // Assign defaults
    Utils::fromSettings(QLatin1String(groupPostfix), category, s, this);
}

void CppCodeStyleSettings::toMap(const QString &prefix, QVariantMap *map) const
{
    map->insert(prefix + QLatin1String(indentBlockBracesKey), indentBlockBraces);
    map->insert(prefix + QLatin1String(indentBlockBodyKey), indentBlockBody);
    map->insert(prefix + QLatin1String(indentClassBracesKey), indentClassBraces);
    map->insert(prefix + QLatin1String(indentEnumBracesKey), indentEnumBraces);
    map->insert(prefix + QLatin1String(indentNamespaceBracesKey), indentNamespaceBraces);
    map->insert(prefix + QLatin1String(indentNamespaceBodyKey), indentNamespaceBody);
    map->insert(prefix + QLatin1String(indentAccessSpecifiersKey), indentAccessSpecifiers);
    map->insert(prefix + QLatin1String(indentDeclarationsRelativeToAccessSpecifiersKey), indentDeclarationsRelativeToAccessSpecifiers);
    map->insert(prefix + QLatin1String(indentFunctionBodyKey), indentFunctionBody);
    map->insert(prefix + QLatin1String(indentFunctionBracesKey), indentFunctionBraces);
    map->insert(prefix + QLatin1String(indentSwitchLabelsKey), indentSwitchLabels);
    map->insert(prefix + QLatin1String(indentStatementsRelativeToSwitchLabelsKey), indentStatementsRelativeToSwitchLabels);
    map->insert(prefix + QLatin1String(indentBlocksRelativeToSwitchLabelsKey), indentBlocksRelativeToSwitchLabels);
    map->insert(prefix + QLatin1String(indentControlFlowRelativeToSwitchLabelsKey), indentControlFlowRelativeToSwitchLabels);
    map->insert(prefix + QLatin1String(extraPaddingForConditionsIfConfusingAlignKey), extraPaddingForConditionsIfConfusingAlign);
    map->insert(prefix + QLatin1String(alignAssignmentsKey), alignAssignments);
}

void CppCodeStyleSettings::fromMap(const QString &prefix, const QVariantMap &map)
{
    indentBlockBraces = map.value(prefix + QLatin1String(indentBlockBracesKey),
                                indentBlockBraces).toBool();
    indentBlockBody = map.value(prefix + QLatin1String(indentBlockBodyKey),
                                indentBlockBody).toBool();
    indentClassBraces = map.value(prefix + QLatin1String(indentClassBracesKey),
                                indentClassBraces).toBool();
    indentEnumBraces = map.value(prefix + QLatin1String(indentEnumBracesKey),
                                indentEnumBraces).toBool();
    indentNamespaceBraces = map.value(prefix + QLatin1String(indentNamespaceBracesKey),
                                indentNamespaceBraces).toBool();
    indentNamespaceBody = map.value(prefix + QLatin1String(indentNamespaceBodyKey),
                                indentNamespaceBody).toBool();
    indentAccessSpecifiers = map.value(prefix + QLatin1String(indentAccessSpecifiersKey),
                                indentAccessSpecifiers).toBool();
    indentDeclarationsRelativeToAccessSpecifiers = map.value(prefix + QLatin1String(indentDeclarationsRelativeToAccessSpecifiersKey),
                                indentDeclarationsRelativeToAccessSpecifiers).toBool();
    indentFunctionBody = map.value(prefix + QLatin1String(indentFunctionBodyKey),
                                indentFunctionBody).toBool();
    indentFunctionBraces = map.value(prefix + QLatin1String(indentFunctionBracesKey),
                                indentFunctionBraces).toBool();
    indentSwitchLabels = map.value(prefix + QLatin1String(indentSwitchLabelsKey),
                                indentSwitchLabels).toBool();
    indentStatementsRelativeToSwitchLabels = map.value(prefix + QLatin1String(indentStatementsRelativeToSwitchLabelsKey),
                                indentStatementsRelativeToSwitchLabels).toBool();
    indentBlocksRelativeToSwitchLabels = map.value(prefix + QLatin1String(indentBlocksRelativeToSwitchLabelsKey),
                                indentBlocksRelativeToSwitchLabels).toBool();
    indentControlFlowRelativeToSwitchLabels = map.value(prefix + QLatin1String(indentControlFlowRelativeToSwitchLabelsKey),
                                indentControlFlowRelativeToSwitchLabels).toBool();
    extraPaddingForConditionsIfConfusingAlign = map.value(prefix + QLatin1String(extraPaddingForConditionsIfConfusingAlignKey),
                                extraPaddingForConditionsIfConfusingAlign).toBool();
    alignAssignments = map.value(prefix + QLatin1String(alignAssignmentsKey),
                                alignAssignments).toBool();
}

bool CppCodeStyleSettings::equals(const CppCodeStyleSettings &rhs) const
{
    return indentBlockBraces == rhs.indentBlockBraces
           && indentBlockBody == rhs.indentBlockBody
           && indentClassBraces == rhs.indentClassBraces
           && indentEnumBraces == rhs.indentEnumBraces
           && indentNamespaceBraces == rhs.indentNamespaceBraces
           && indentNamespaceBody == rhs.indentNamespaceBody
           && indentAccessSpecifiers == rhs.indentAccessSpecifiers
           && indentDeclarationsRelativeToAccessSpecifiers == rhs.indentDeclarationsRelativeToAccessSpecifiers
           && indentFunctionBody == rhs.indentFunctionBody
           && indentFunctionBraces == rhs.indentFunctionBraces
           && indentSwitchLabels == rhs.indentSwitchLabels
           && indentStatementsRelativeToSwitchLabels == rhs.indentStatementsRelativeToSwitchLabels
           && indentBlocksRelativeToSwitchLabels == rhs.indentBlocksRelativeToSwitchLabels
           && indentControlFlowRelativeToSwitchLabels == rhs.indentControlFlowRelativeToSwitchLabels
           && extraPaddingForConditionsIfConfusingAlign == rhs.extraPaddingForConditionsIfConfusingAlign
           && alignAssignments == rhs.alignAssignments;
}

