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

#ifndef BEHAVIORSETTINGS_H
#define BEHAVIORSETTINGS_H

#include "texteditor_global.h"

#include <QtCore/QVariantMap>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

/**
 * Settings that describe how the text editor behaves. This does not include
 * the TabSettings and StorageSettings.
 */
class TEXTEDITOR_EXPORT BehaviorSettings
{
public:
    BehaviorSettings();

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    void toMap(const QString &prefix, QVariantMap *map) const;
    void fromMap(const QString &prefix, const QVariantMap &map);

    bool equals(const BehaviorSettings &bs) const;

    bool m_mouseNavigation;
    bool m_scrollWheelZooming;
    bool m_constrainTooltips;
    bool m_camelCaseNavigation;
};

inline bool operator==(const BehaviorSettings &t1, const BehaviorSettings &t2) { return t1.equals(t2); }
inline bool operator!=(const BehaviorSettings &t1, const BehaviorSettings &t2) { return !t1.equals(t2); }

} // namespace TextEditor

#endif // BEHAVIORSETTINGS_H
