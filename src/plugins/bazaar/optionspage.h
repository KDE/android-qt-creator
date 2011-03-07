/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef OPTIONSPAGE_H
#define OPTIONSPAGE_H

#include "ui_optionspage.h"

#include <vcsbase/vcsbaseoptionspage.h>

#include <QtGui/QWidget>
#include <QtCore/QPointer>

namespace Bazaar {
namespace Internal {

class BazaarSettings;

class OptionsPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OptionsPageWidget(QWidget *parent = 0);

    BazaarSettings settings() const;
    void setSettings(const BazaarSettings &s);
    QString searchKeywords() const;

private:
    Ui::OptionsPage m_ui;
};


class OptionsPage : public VCSBase::VCSBaseOptionsPage
{
    Q_OBJECT

public:
    OptionsPage();
    QString id() const;
    QString displayName() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }
    bool matches(const QString &s) const;

signals:
    void settingsChanged();

private:
    QString m_searchKeywords;
    QPointer<OptionsPageWidget> m_optionsPageWidget;
};

} // namespace Internal
} // namespace Bazaar

#endif // OPTIONSPAGE_H
