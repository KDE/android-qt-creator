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

#include "projectwizardpage.h"
#include "ui_projectwizardpage.h"

#include <QtCore/QDir>
#include <QtCore/QTextStream>

using namespace ProjectExplorer;
using namespace Internal;

ProjectWizardPage::ProjectWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_ui(new Ui::WizardPage)
{
    m_ui->setupUi(this);
    connect(m_ui->projectComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotProjectChanged(int)));
    setProperty("shortTitle", tr("Summary"));
}

ProjectWizardPage::~ProjectWizardPage()
{
    delete m_ui;
}

void ProjectWizardPage::setProjects(const QStringList &p)
{
    m_ui->projectComboBox->clear();
    m_ui->projectComboBox->addItems(p);
    m_ui->projectComboBox->setEnabled(p.size() > 1);
    m_ui->projectLabel->setEnabled(p.size() > 1);
}

void ProjectWizardPage::setProjectToolTips(const QStringList &t)
{
    m_projectToolTips = t;
}

int ProjectWizardPage::currentProjectIndex() const
{
    return m_ui->projectComboBox->currentIndex();
}

void ProjectWizardPage::setCurrentProjectIndex(int idx)
{
    m_ui->projectComboBox->setCurrentIndex(idx);
}

void ProjectWizardPage::setNoneLabel(const QString &label)
{
    m_ui->projectComboBox->setItemText(0, label);
}

void ProjectWizardPage::setAdditionalInfo(const QString &text)
{
    m_ui->additionalInfo->setText(text);
    m_ui->additionalInfo->setVisible(!text.isEmpty());
}

void ProjectWizardPage::setVersionControls(const QStringList &vcs)
{
    m_ui->addToVersionControlComboBox->clear();
    m_ui->addToVersionControlComboBox->addItems(vcs);
}

int ProjectWizardPage::versionControlIndex() const
{
    return m_ui->addToVersionControlComboBox->currentIndex();
}

void ProjectWizardPage::setVersionControlIndex(int idx)
{
    m_ui->addToVersionControlComboBox->setCurrentIndex(idx);
}

void ProjectWizardPage::changeEvent(QEvent *e)
{
    QWizardPage::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void ProjectWizardPage::setFilesDisplay(const QString &commonPath, const QStringList &files)
{
    QString fileMessage;
    {
        QTextStream str(&fileMessage);
        str << "<qt>"
            << (commonPath.isEmpty() ? tr("Files to be added:") : tr("Files to be added in"))
            << "<pre>";
        if (commonPath.isEmpty()) {
            foreach(const QString &f, files)
                str << QDir::toNativeSeparators(f) << '\n';
        } else {
            str << QDir::toNativeSeparators(commonPath) << ":\n\n";
            const int prefixSize = commonPath.size() + 1;
            foreach(const QString &f, files)
                str << QDir::toNativeSeparators(f.right(f.size() - prefixSize)) << '\n';
        }
        str << "</pre>";
    }
    m_ui->filesLabel->setText(fileMessage);
}

void ProjectWizardPage::setProjectToolTip(const QString &tt)
{
    m_ui->projectComboBox->setToolTip(tt);
    m_ui->projectLabel->setToolTip(tt);
}

void ProjectWizardPage::slotProjectChanged(int index)
{
    setProjectToolTip(index >= 0 && index < m_projectToolTips.size() ?
                      m_projectToolTips.at(index) : QString());
}
