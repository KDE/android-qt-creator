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

#ifndef GENERALSETTINGSPAGE_H
#define GENERALSETTINGSPAGE_H

#include "ui_generalsettingspage.h"
#include <coreplugin/dialogs/ioptionspage.h>

namespace Help {
namespace Internal {

class CentralWidget;

class GeneralSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    GeneralSettingsPage();

    QString id() const;
    QString displayName() const;
    QString category() const;
    QString displayCategory() const;
    QIcon categoryIcon() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    virtual bool matches(const QString &s) const;

signals:
    void fontChanged();
    void startOptionChanged();
    void returnOnCloseChanged();
    void contextHelpOptionChanged();

private slots:
    void setCurrentPage();
    void setBlankPage();
    void setDefaultPage();
    void importBookmarks();
    void exportBookmarks();

private:
    void updateFontSize();
    void updateFontStyle();
    void updateFontFamily();
    int closestPointSizeIndex(int desiredPointSize) const;

private:
    QFont m_font;
    QFontDatabase m_fontDatabase;

    QString m_homePage;
    int m_contextOption;

    bool m_returnOnClose;

    QString m_searchKeywords;
    Ui::GeneralSettingsPage *m_ui;
};

    }   // Internal
}   // Help

#endif  // GENERALSETTINGSPAGE_H
