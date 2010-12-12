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

#ifndef HELPMODE_H
#define HELPMODE_H

#include "helpmode.h"
#include "helpconstants.h"

#include <coreplugin/imode.h>

#include <QtCore/QString>
#include <QtGui/QIcon>

namespace Help {
namespace Internal {

class HelpMode : public Core::IMode
{
public:
    HelpMode(QObject *parent = 0);

    QString displayName() const { return tr("Help"); }
    QIcon icon() const { return m_icon; }
    int priority() const { return Constants::P_MODE_HELP; }
    QWidget *widget() { return m_widget; }
    QString id() const { return QLatin1String(Constants::ID_MODE_HELP); }
    QString type() const { return QString(); }
    Core::Context context() const { return Core::Context(Constants::C_MODE_HELP); }
    QString contextHelpId() const { return QString(); }
    void setWidget(QWidget *widget) { m_widget = widget; }

private:
    QWidget *m_widget;
    QIcon m_icon;
};

} // namespace Internal
} // namespace Help

#endif // HELPMODE_H
