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

#ifndef DESIGNER_SETTINGSPAGE_H
#define DESIGNER_SETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>

QT_BEGIN_NAMESPACE
class QDesignerOptionsPageInterface;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

class SettingsPageWidget;

class SettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit SettingsPage(QDesignerOptionsPageInterface *designerPage);
    virtual ~SettingsPage();

    QString id() const;
    QString displayName() const;
    QString category() const;
    QString displayCategory() const;
    QIcon categoryIcon() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish();

private:
    QDesignerOptionsPageInterface *m_designerPage;
    bool m_initialized;
};

class SettingsPageProvider : public Core::IOptionsPageProvider
{
    Q_OBJECT

public:
    SettingsPageProvider(QObject *parent = 0);
    ~SettingsPageProvider();

    QString category() const;
    QString displayCategory() const;
    QIcon categoryIcon() const;

    QList<Core::IOptionsPage *> pages() const;

private:
    mutable bool m_initialized;
};

} // namespace Internal
} // namespace Designer

#endif // DESIGNER_SETTINGSPAGE_H
