/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef IOPTIONSPAGE_H
#define IOPTIONSPAGE_H

#include <coreplugin/core_global.h>

#include <QIcon>
#include <QObject>

namespace Core {

class CORE_EXPORT IOptionsPage : public QObject
{
    Q_OBJECT

public:
    IOptionsPage(QObject *parent = 0) : QObject(parent) {}

    QString id() const { return m_id; }
    QString displayName() const { return m_displayName; }
    QString category() const { return m_category; }
    QString displayCategory() const { return m_displayCategory; }
    QIcon categoryIcon() const { return QIcon(m_categoryIcon); }

    virtual bool matches(const QString & /* searchKeyWord*/) const { return false; }
    virtual QWidget *createPage(QWidget *parent) = 0;
    virtual void apply() = 0;
    virtual void finish() = 0;

protected:
    void setId(const QString &id) { m_id = id; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setCategory(const QString &category) { m_category = category; }
    void setDisplayCategory(const QString &displayCategory) { m_displayCategory = displayCategory; }
    void setCategoryIcon(const QString &categoryIcon) { m_categoryIcon = categoryIcon; }

    QString m_id;
    QString m_displayName;
    QString m_category;
    QString m_displayCategory;
    QString m_categoryIcon;
};

/*
    Alternative way for providing option pages instead of adding IOptionsPage
    objects into the plugin manager pool. Should only be used if creation of the
    actual option pages is not possible or too expensive at Qt Creator startup.
    (Like the designer integration, which needs to initialize designer plugins
    before the options pages get available.)
*/

class CORE_EXPORT IOptionsPageProvider : public QObject
{
    Q_OBJECT

public:
    IOptionsPageProvider(QObject *parent = 0) : QObject(parent) {}

    QString category() const { return m_category; }
    QString displayCategory() const { return m_displayCategory; }
    QIcon categoryIcon() const { return QIcon(m_categoryIcon); }

    virtual QList<IOptionsPage *> pages() const = 0;

protected:
    void setCategory(const QString &category) { m_category = category; }
    void setDisplayCategory(const QString &displayCategory) { m_displayCategory = displayCategory; }
    void setCategoryIcon(const QString &categoryIcon) { m_categoryIcon = categoryIcon; }

    QString m_category;
    QString m_displayCategory;
    QString m_categoryIcon;
};

} // namespace Core

#endif // IOPTIONSPAGE_H
