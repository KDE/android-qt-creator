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

#ifndef COMMAND_P_H
#define COMMAND_P_H

#include "command.h"
#include "actionmanager_p.h"

#include <QtCore/QList>
#include <QtCore/QMultiMap>
#include <QtCore/QPointer>
#include <QtGui/QKeySequence>

namespace Core {
namespace Internal {

class CommandPrivate : public Core::Command
{
    Q_OBJECT
public:
    CommandPrivate(int id);
    virtual ~CommandPrivate() {}

    virtual QString name() const = 0;

    void setDefaultKeySequence(const QKeySequence &key);
    QKeySequence defaultKeySequence() const;

    void setKeySequence(const QKeySequence &key);

    void setDefaultText(const QString &text);
    QString defaultText() const;

    int id() const;

    QAction *action() const;
    QShortcut *shortcut() const;
    Context context() const;


    void setAttribute(CommandAttribute attr);
    void removeAttribute(CommandAttribute attr);
    bool hasAttribute(CommandAttribute attr) const;

    virtual bool setCurrentContext(const Context &context) = 0;

    QString stringWithAppendedShortcut(const QString &str) const;

protected:
    Context m_context;
    QString m_category;
    int m_attributes;
    int m_id;
    QKeySequence m_defaultKey;
    QString m_defaultText;
    bool m_isKeyInitialized;
};

class Shortcut : public CommandPrivate
{
    Q_OBJECT
public:
    Shortcut(int id);

    QString name() const;

    void setKeySequence(const QKeySequence &key);
    QKeySequence keySequence() const;

    virtual void setDefaultText(const QString &key);
    virtual QString defaultText() const;

    void setShortcut(QShortcut *shortcut);
    QShortcut *shortcut() const;

    void setContext(const Context &context);
    Context context() const;
    bool setCurrentContext(const Context &context);

    bool isActive() const;
private:
    QShortcut *m_shortcut;
    QString m_defaultText;
};

class Action : public CommandPrivate
{
    Q_OBJECT
public:
    Action(int id);

    QString name() const;

    void setKeySequence(const QKeySequence &key);
    QKeySequence keySequence() const;

    virtual void setAction(QAction *action);
    QAction *action() const;

    void setLocations(const QList<CommandLocation> &locations);
    QList<CommandLocation> locations() const;

    bool setCurrentContext(const Context &context);
    bool isActive() const;
    void addOverrideAction(QAction *action, const Context &context);
    void removeOverrideAction(QAction *action);
    bool isEmpty() const;

protected:
    void updateToolTipWithKeySequence();

private slots:
    void actionChanged();

private:
    void setActive(bool state);

    QAction *m_action;
    QList<CommandLocation> m_locations;
    QString m_toolTip;

    QPointer<QAction> m_currentAction;
    QMap<int, QPointer<QAction> > m_contextActionMap;
    bool m_active;
    bool m_contextInitialized;
};

} // namespace Internal
} // namespace Core

#endif // COMMAND_P_H
