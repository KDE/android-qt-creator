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

#ifndef PROXYACTION_H
#define PROXYACTION_H

#include "utils_global.h"

#include <QtCore/QPointer>
#include <QtGui/QAction>

namespace Utils {

class QTCREATOR_UTILS_EXPORT ProxyAction : public QAction
{
    Q_OBJECT
public:
    enum Attribute {
        Hide = 0x01,
        UpdateText = 0x02,
        UpdateIcon = 0x04
    };
    Q_DECLARE_FLAGS(Attributes, Attribute)

    explicit ProxyAction(QObject *parent = 0);

    void initialize(QAction *action);

    void setAction(QAction *action);
    QAction *action() const;

    bool shortcutVisibleInToolTip() const;
    void setShortcutVisibleInToolTip(bool visible);

    void setAttribute(Attribute attribute);
    void removeAttribute(Attribute attribute);
    bool hasAttribute(Attribute attribute);

    static QString stringWithAppendedShortcut(const QString &str, const QKeySequence &shortcut);

private slots:
    void actionChanged();
    void updateState();
    void updateToolTipWithKeySequence();

private:
    void disconnectAction();
    void connectAction();
    void update(QAction *action, bool initialize);

    QPointer<QAction> m_action;
    Attributes m_attributes;
    bool m_showShortcut;
    QString m_toolTip;
    bool m_block;
};

} // namespace Utils

Q_DECLARE_OPERATORS_FOR_FLAGS(Utils::ProxyAction::Attributes)

#endif // PROXYACTION_H
