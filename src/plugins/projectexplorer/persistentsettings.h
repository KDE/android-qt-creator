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

#ifndef PERSISTENTSETTINGS_H
#define PERSISTENTSETTINGS_H

#include "projectexplorer_export.h"

#include <QtCore/QMap>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE
class QDomElement;
QT_END_NAMESPACE

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT PersistentSettingsReader
{
public:
    PersistentSettingsReader();
    QVariant restoreValue(const QString & variable) const;
    QVariantMap restoreValues() const;
    bool load(const QString & fileName);

private:
    QVariant readValue(const QDomElement &valElement) const;
    void readValues(const QDomElement &data);
    QMap<QString, QVariant> m_valueMap;
};

class PROJECTEXPLORER_EXPORT PersistentSettingsWriter
{
public:
    PersistentSettingsWriter();
    void saveValue(const QString & variable, const QVariant &value);
    bool save(const QString & fileName, const QString & docType);

private:
    void writeValue(QDomElement &ps, const QVariant &value);
    QMap<QString, QVariant> m_valueMap;
};

} // namespace ProjectExplorer

#endif // PERSISTENTSETTINGS_H
