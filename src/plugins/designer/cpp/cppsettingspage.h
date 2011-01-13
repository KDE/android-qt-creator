/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CPPSETTINGSPAGE_H
#define CPPSETTINGSPAGE_H

#include "ui_cppsettingspagewidget.h"
#include "qtdesignerformclasscodegenerator.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QtCore/QPointer>

namespace Designer {
namespace Internal {

class CppSettingsPageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CppSettingsPageWidget(QWidget *parent = 0);

    FormClassWizardGenerationParameters parameters() const;
    void setParameters(const FormClassWizardGenerationParameters &p);

    QString searchKeywords() const;

private:
    int uiEmbedding() const;
    void setUiEmbedding(int);

    Ui::CppSettingsPageWidget m_ui;
};

class CppSettingsPage : public Core::IOptionsPage
{
public:
    explicit CppSettingsPage(QObject *parent = 0);

    virtual QString id() const;
    virtual QString displayName() const;
    virtual QString category() const;
    virtual QString displayCategory() const;
    virtual QIcon categoryIcon() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish();
    virtual bool matches(const QString &s) const;

private:
    QPointer<CppSettingsPageWidget> m_widget;
    FormClassWizardGenerationParameters m_parameters;
    QString m_searchKeywords;
};

} // namespace Internal
} // namespace Designer

#endif // CPPSETTINGSPAGE_H
