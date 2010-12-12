/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef SHORTCUTSETTINGS_H
#define SHORTCUTSETTINGS_H

#include <coreplugin/actionmanager/commandmappings.h>

#include <QtGui/QKeySequence>

QT_BEGIN_NAMESPACE
class QKeyEvent;
QT_END_NAMESPACE

namespace Core {

class Command;

namespace Internal {

class ActionManagerPrivate;
class MainWindow;

struct ShortcutItem
{
    Command *m_cmd;
    QKeySequence m_key;
    QTreeWidgetItem *m_item;
};


class ShortcutSettings : public Core::CommandMappings
{
    Q_OBJECT

public:
    ShortcutSettings(QObject *parent = 0);
    ~ShortcutSettings();

    // IOptionsPage
    QString id() const;
    QString displayName() const;
    QString category() const;
    QString displayCategory() const;
    QIcon categoryIcon() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    bool matches(const QString &s) const;

protected:
    bool eventFilter(QObject *o, QEvent *e);

private slots:
    void commandChanged(QTreeWidgetItem *current);
    void targetIdentifierChanged();
    void resetTargetIdentifier();
    void removeTargetIdentifier();
    void importAction();
    void exportAction();
    void defaultAction();
    void initialize();

private:
    void setKeySequence(const QKeySequence &key);
    void clear();

    void handleKeyEvent(QKeyEvent *e);
    int translateModifiers(Qt::KeyboardModifiers state, const QString &text);
    void markPossibleCollisions(ShortcutItem *);
    void resetCollisionMarker(ShortcutItem *);
    void resetCollisionMarkers();

    QList<ShortcutItem *> m_scitems;
    int m_key[4], m_keyNum;

    QString m_searchKeywords;
    bool m_initialized;
};

} // namespace Internal
} // namespace Core

#endif // SHORTCUTSETTINGS_H
