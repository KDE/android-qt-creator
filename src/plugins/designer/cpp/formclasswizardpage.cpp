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

#include "formclasswizardpage.h"
#include "ui_formclasswizardpage.h"
#include "formclasswizardparameters.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <cppeditor/cppeditorconstants.h>
#include <cpptools/cpptoolsconstants.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <QtGui/QAbstractButton>
#include <QtGui/QMessageBox>

namespace Designer {
namespace Internal {

// ----------------- FormClassWizardPage

FormClassWizardPage::FormClassWizardPage(QWidget * parent) :
    QWizardPage(parent),
    m_ui(new Ui::FormClassWizardPage),
    m_isValid(false)
{
    m_ui->setupUi(this);

    m_ui->newClassWidget->setBaseClassInputVisible(false);
    m_ui->newClassWidget->setNamespacesEnabled(true);
    m_ui->newClassWidget->setAllowDirectories(true);
    m_ui->newClassWidget->setClassTypeComboVisible(false);

    connect(m_ui->newClassWidget, SIGNAL(validChanged()), this, SLOT(slotValidChanged()));

    initFileGenerationSettings();
}

FormClassWizardPage::~FormClassWizardPage()
{
    delete m_ui;
}

// Retrieve settings of CppTools plugin.
bool FormClassWizardPage::lowercaseHeaderFiles()
{
    QString lowerCaseSettingsKey = QLatin1String(CppTools::Constants::CPPTOOLS_SETTINGSGROUP);
    lowerCaseSettingsKey += QLatin1Char('/');
    lowerCaseSettingsKey += QLatin1String(CppTools::Constants::LOWERCASE_CPPFILES_KEY);
    const bool lowerCaseDefault = CppTools::Constants::lowerCaseFilesDefault;
    return Core::ICore::instance()->settings()->value(lowerCaseSettingsKey, QVariant(lowerCaseDefault)).toBool();
}

// Set up new class widget from settings
void FormClassWizardPage::initFileGenerationSettings()
{
    Core::ICore *core = Core::ICore::instance();
    const Core::MimeDatabase *mdb = core->mimeDatabase();
    m_ui->newClassWidget->setHeaderExtension(mdb->preferredSuffixByType(QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE)));
    m_ui->newClassWidget->setSourceExtension(mdb->preferredSuffixByType(QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)));
    m_ui->newClassWidget->setLowerCaseFiles(lowercaseHeaderFiles());
}

void FormClassWizardPage::setClassName(const QString &suggestedClassName)
{
    // Is it valid, now?
    m_ui->newClassWidget->setClassName(suggestedClassName);
    slotValidChanged();
}

QString FormClassWizardPage::path() const
{
    return m_ui->newClassWidget->path();
}

void FormClassWizardPage::setPath(const QString &p)
{
    m_ui->newClassWidget->setPath(p);
}

void FormClassWizardPage::getParameters(FormClassWizardParameters *p) const
{
    p->className = m_ui->newClassWidget->className();
    p->path = path();
    p->sourceFile = m_ui->newClassWidget->sourceFileName();
    p->headerFile = m_ui->newClassWidget->headerFileName();
    p->uiFile = m_ui->newClassWidget-> formFileName();
}

void FormClassWizardPage::slotValidChanged()
{
    const bool validNow = m_ui->newClassWidget->isValid();
    if (m_isValid != validNow) {
        m_isValid = validNow;
        emit completeChanged();
    }
}

bool FormClassWizardPage::isComplete() const
{
    return m_isValid;
}

bool FormClassWizardPage::validatePage()
{
    QString errorMessage;
    const bool rc = m_ui->newClassWidget->isValid(&errorMessage);
    if (!rc) {
        QMessageBox::warning(this, tr("%1 - Error").arg(title()), errorMessage);
    }
    return rc;
}

} // namespace Internal
} // namespace Designer
