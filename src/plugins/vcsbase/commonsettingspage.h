/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef COMMONOPTIONSPAGE_H
#define COMMONOPTIONSPAGE_H

#include "commonvcssettings.h"

#include "vcsbaseoptionspage.h"

#include <QtCore/QPointer>
#include <QtGui/QWidget>

namespace VcsBase {
namespace Internal {

namespace Ui { class CommonSettingsPage; }

class CommonSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CommonSettingsWidget(QWidget *parent = 0);
    ~CommonSettingsWidget();

    CommonVcsSettings settings() const;
    void setSettings(const CommonVcsSettings &s);

    QString searchKeyWordMatchString() const;

private:
    Ui::CommonSettingsPage *m_ui;
};

class CommonOptionsPage : public VcsBaseOptionsPage
{
    Q_OBJECT

public:
    explicit CommonOptionsPage(QObject *parent = 0);

    QString id() const;
    QString displayName() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }
    bool matches(const QString &key) const;

    CommonVcsSettings settings() const { return m_settings; }

signals:
    void settingsChanged(const VcsBase::Internal::CommonVcsSettings &s);

private:
    void updateNickNames();

    CommonSettingsWidget *m_widget;
    CommonVcsSettings m_settings;
    QString m_searchKeyWords;
};

} // namespace Internal
} // namespace VcsBase

#endif // COMMONOPTIONSPAGE_H
