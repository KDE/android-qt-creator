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

#include "filesselectionwizardpage.h"

#include "genericprojectwizard.h"
#include "selectablefilesmodel.h"

#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;

FilesSelectionWizardPage::FilesSelectionWizardPage(GenericProjectWizardDialog *genericProjectWizard, QWidget *parent)
    : QWizardPage(parent), m_genericProjectWizardDialog(genericProjectWizard), m_model(0), m_finished(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *hbox = new QHBoxLayout;
    m_filterLabel = new QLabel;
    m_filterLabel->setText(tr("Hide files matching:"));
    m_filterLabel->hide();
    hbox->addWidget(m_filterLabel);
    m_filterLineEdit = new QLineEdit;

    m_filterLineEdit->setText("Makefile*; *.o; *.obj; *~; *.files; *.config; *.creator; *.user; *.includes");
    m_filterLineEdit->hide();
    hbox->addWidget(m_filterLineEdit);
    m_applyFilterButton = new QPushButton(tr("Apply Filter"), this);
    m_applyFilterButton->hide();
    hbox->addWidget(m_applyFilterButton);
    layout->addLayout(hbox);

    m_view = new QTreeView;
    m_view->setMinimumSize(500, 400);
    m_view->setHeaderHidden(true);
    m_view->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    m_label = new QLabel;
    m_label->setMaximumWidth(500);

    layout->addWidget(m_view);
    layout->addWidget(m_label);

    connect(m_applyFilterButton, SIGNAL(clicked()), this, SLOT(applyFilter()));
}

void FilesSelectionWizardPage::initializePage()
{
    m_view->setModel(0);
    delete m_model;
    Core::MimeDatabase *mimeDatabase = Core::ICore::instance()->mimeDatabase();
    m_model = new SelectableFilesModel(m_genericProjectWizardDialog->path(), this);
    m_model->setSuffixes(mimeDatabase->suffixes().toSet());
    connect(m_model, SIGNAL(parsingProgress(QString)),
            this, SLOT(parsingProgress(QString)));
    connect(m_model, SIGNAL(parsingFinished()),
            this, SLOT(parsingFinished()));
    m_model->startParsing();
    m_filterLabel->setVisible(false);
    m_filterLineEdit->setVisible(false);
    m_applyFilterButton->setVisible(false);
    m_view->setVisible(false);
    m_label->setVisible(true);
    m_view->setModel(m_model);
}

void FilesSelectionWizardPage::cleanupPage()
{
    m_model->cancel();
    m_model->waitForFinished();
}

void FilesSelectionWizardPage::parsingProgress(const QString &text)
{
    m_label->setText(tr("Generating file list...\n\n%1").arg(text));
}

void FilesSelectionWizardPage::parsingFinished()
{
    m_finished = true;
    m_filterLabel->setVisible(true);
    m_filterLineEdit->setVisible(true);
    m_applyFilterButton->setVisible(true);
    m_view->setVisible(true);
    m_label->setVisible(false);
    m_view->expand(m_view->model()->index(0,0, QModelIndex()));
    emit completeChanged();
    applyFilter();
    // work around qt
    m_genericProjectWizardDialog->setTitleFormat(m_genericProjectWizardDialog->titleFormat());
}

bool FilesSelectionWizardPage::isComplete() const
{
    return m_finished;
}

QStringList FilesSelectionWizardPage::selectedPaths() const
{
    return m_model ? m_model->selectedPaths() : QStringList();
}

QStringList FilesSelectionWizardPage::selectedFiles() const
{
    return m_model ? m_model->selectedFiles() : QStringList();
}

void FilesSelectionWizardPage::applyFilter()
{
    m_model->applyFilter(m_filterLineEdit->text());
}
