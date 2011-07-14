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

#ifndef FORMWIZARDDIALOG_H
#define FORMWIZARDDIALOG_H

#include <utils/wizard.h>

namespace Utils {
    class FileWizardPage;
}

namespace Designer {
namespace Internal {

class FormTemplateWizardPage;

// Single-Page Wizard for new forms offering all types known to Qt Designer.
// To be used for Mode "CreateNewEditor" [not currently used]

class FormWizardDialog : public Utils::Wizard
{
    Q_DISABLE_COPY(FormWizardDialog)
    Q_OBJECT

public:
    typedef QList<QWizardPage *> WizardPageList;
    explicit FormWizardDialog(const WizardPageList &extensionPages,
                              QWidget *parent = 0);

    QString templateContents() const;

private:
    void init(const WizardPageList &extensionPages);

    FormTemplateWizardPage *m_formPage;
    mutable QString m_templateContents;
};

// Two-Page Wizard for new forms for mode "CreateNewFile". Gives
// FormWizardDialog an additional page with file and path fields,
// initially determined from the UI class chosen on page one.

class FormFileWizardDialog : public FormWizardDialog
{
    Q_DISABLE_COPY(FormFileWizardDialog)
    Q_OBJECT

public:
    explicit FormFileWizardDialog(const WizardPageList &extensionPages,
                                  QWidget *parent = 0);

    QString path() const;
    QString fileName() const;

public slots:
    void setPath(const QString &path);

private slots:
    void slotCurrentIdChanged(int id);

private:
    Utils::FileWizardPage *m_filePage;
};

} // namespace Internal
} // namespace Designer

#endif // FORMWIZARDDIALOG_H
