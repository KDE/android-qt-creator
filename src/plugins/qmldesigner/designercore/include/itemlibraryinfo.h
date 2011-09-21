/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef ITEMLIBRARYINFO_H
#define ITEMLIBRARYINFO_H

#include "corelib_global.h"

#include "propertycontainer.h"
#include <QSharedPointer>

namespace QmlDesigner {

namespace Internal {

class ItemLibraryEntryData;
class ItemLibraryInfoPrivate;
class MetaInfoPrivate;
}

class ItemLibraryEntry;

CORESHARED_EXPORT QDataStream& operator<<(QDataStream& stream, const ItemLibraryEntry &itemLibraryEntry);
CORESHARED_EXPORT QDataStream& operator>>(QDataStream& stream, ItemLibraryEntry &itemLibraryEntry);

class CORESHARED_EXPORT ItemLibraryEntry
{
    //friend class QmlDesigner::MetaInfo;
    //friend class QmlDesigner::Internal::MetaInfoParser;
    friend CORESHARED_EXPORT QDataStream& QmlDesigner::operator<<(QDataStream& stream, const ItemLibraryEntry &itemLibraryEntry);
    friend CORESHARED_EXPORT QDataStream& QmlDesigner::operator>>(QDataStream& stream, ItemLibraryEntry &itemLibraryEntry);

public:
    ItemLibraryEntry();
    ~ItemLibraryEntry();

    QString name() const;
    QString typeName() const;
    QIcon icon() const;
    QString iconPath() const;
    int majorVersion() const;
    int minorVersion() const;
    QString category() const;
    QIcon dragIcon() const;
    QString qml() const;
    QString requiredImport() const;
    bool forceImport() const;

    ItemLibraryEntry(const ItemLibraryEntry &other);
    ItemLibraryEntry& operator=(const ItemLibraryEntry &other);

    typedef QmlDesigner::PropertyContainer Property;

    QList<Property> properties() const;

    void setType(const QString &typeName, int majorVersion, int minorVersion);
    void setName(const QString &name);
    void setIconPath(const QString &iconPath);
    void addProperty(const Property &p);
    void addProperty(QString &name, QString &type, QString &value);
    void setDragIcon(const QIcon &icon);
    void setIcon(const QIcon &icon);
    void setCategory(const QString &category);
    void setQml(const QString &qml);
    void setRequiredImport(const QString &requiredImport);
    void setForceImport(bool b);
private:
    QExplicitlySharedDataPointer<Internal::ItemLibraryEntryData> m_data;
};

class CORESHARED_EXPORT ItemLibraryInfo : public QObject
{
    Q_OBJECT

    friend class Internal::MetaInfoPrivate;
public:
    ~ItemLibraryInfo();

    QList<ItemLibraryEntry> entries() const;
    QList<ItemLibraryEntry> entriesForType(const QString &typeName, int majorVersion, int minorVersion) const;
    ItemLibraryEntry entry(const QString &name) const;

    void addEntry(const ItemLibraryEntry &entry);
    bool containsEntry(const ItemLibraryEntry &entry);
    void clearEntries();

signals:
    void entriesChanged();

private:
    ItemLibraryInfo(QObject *parent = 0);
    void setBaseInfo(ItemLibraryInfo *baseInfo);
    Internal::ItemLibraryInfoPrivate *d;
};

} // namespace QmlDesigner

#endif // ITEMLIBRARYINFO_H
