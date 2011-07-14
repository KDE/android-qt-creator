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

#include "formclasswizarddialog.h"
#include "formtemplatewizardpage.h"
#include "formclasswizardpage.h"
#include "formclasswizardparameters.h"

#include <coreplugin/basefilewizard.h>

#include <QtCore/QDebug>
#include <QtGui/QAbstractButton>

enum { FormPageId, ClassPageId };

namespace Designer {
namespace Internal {

// ----------------- FormClassWizardDialog
FormClassWizardDialog::FormClassWizardDialog(const WizardPageList &extensionPages,
                                             QWidget *parent) :
    Utils::Wizard(parent),
    m_formPage(new FormTemplateWizardPage),
    m_classPage(new FormClassWizardPage)
{
    setWindowTitle(tr("Qt Designer Form Class"));

    setPage(FormPageId, m_formPage);
    wizardProgress()->item(FormPageId)->setTitle(tr("Form Template"));
    connect(m_formPage, SIGNAL(templateActivated()),
            button(QWizard::NextButton), SLOT(animateClick()));

    setPage(ClassPageId, m_classPage);
    wizardProgress()->item(ClassPageId)->setTitle(tr("Class Details"));

    foreach (QWizardPage *p, extensionPages)
        Core::BaseFileWizard::applyExtensionPageShortTitle(this, addPage(p));
}

QString FormClassWizardDialog::path() const
{
    return m_classPage->path();
}

void FormClassWizardDialog::setPath(const QString &p)
{
    m_classPage->setPath(p);
}

bool FormClassWizardDialog::validateCurrentPage()
{
    return QWizard::validateCurrentPage();
}

void FormClassWizardDialog::initializePage(int id)
{
    QWizard::initializePage(id);
    // Switching from form to class page: store XML template and set a suitable
    // class name in the class page based on the form base class
    if (id == ClassPageId) {
        QString formBaseClass;
        QString uiClassName;
        m_rawFormTemplate = m_formPage->templateContents();
        // Strip namespaces from the ui class and suggest it as a new class
        // name
        if (FormTemplateWizardPage::getUIXmlData(m_rawFormTemplate, &formBaseClass, &uiClassName))
            m_classPage->setClassName(FormTemplateWizardPage::stripNamespaces(uiClassName));
    }
}

FormClassWizardParameters FormClassWizardDialog::parameters() const
{
    FormClassWizardParameters rc;
    m_classPage->getParameters(&rc);
    // Name the ui class in the Ui namespace after the class specified
    rc.uiTemplate = FormTemplateWizardPage::changeUiClassName(m_rawFormTemplate, rc.className);
    return rc;
}

} // namespace Internal
} // namespace Designer
