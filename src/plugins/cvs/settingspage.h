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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include "ui_settingspage.h"

#include <vcsbase/vcsbaseoptionspage.h>

#include <QtGui/QWidget>
#include <QtCore/QPointer>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CVS {
namespace Internal {

struct CVSSettings;

class SettingsPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPageWidget(QWidget *parent = 0);

    CVSSettings settings() const;
    void setSettings(const CVSSettings &);

    QString searchKeywords() const;

private:
    Ui::SettingsPage m_ui;
};


class SettingsPage : public VCSBase::VCSBaseOptionsPage
{
    Q_OBJECT

public:
    SettingsPage();

    QString id() const;
    QString displayName() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }
    virtual bool matches(const QString &) const;

private:
    QString m_searchKeywords;
    SettingsPageWidget* m_widget;
};

} // namespace CVS
} // namespace Internal

#endif // SETTINGSPAGE_H
