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

#ifndef LLDBSETTINGSPAGE_H
#define LLDBSETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>
#include "ui_lldboptionspagewidget.h"

#include <QtGui/QWidget>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QSettings>

namespace Debugger {
namespace Internal {

class LldbOptionsPageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LldbOptionsPageWidget(QWidget *parent, QSettings *s);

public slots:
    void save();
    void load();

private:
    Ui::LldbOptionsPageWidget m_ui;
    QSettings *s;
};

class LldbOptionsPage : public Core::IOptionsPage
{
    Q_DISABLE_COPY(LldbOptionsPage)
    Q_OBJECT
public:
    explicit LldbOptionsPage();
    virtual ~LldbOptionsPage();

    // IOptionsPage
    virtual QString id() const { return settingsId(); }
    virtual QString displayName() const;
    virtual QString category() const;
    virtual QString displayCategory() const;
    QIcon categoryIcon() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish();
    virtual bool matches(const QString &) const;

    static QString settingsId();

private:
    QPointer<LldbOptionsPageWidget> m_widget;
};

} // namespace Internal
} // namespace Debugger

#endif // LLDBSETTINGSPAGE_H
