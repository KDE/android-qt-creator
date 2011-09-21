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

#include "filewizardpage.h"
#include "ui_filewizardpage.h"

/*!
  \class Utils::FileWizardPage

  \brief Standard wizard page for a single file letting the user choose name
  and path.

  The name and path labels can be changed. By default they are simply "Name:"
  and "Path:".
*/

namespace Utils {

struct FileWizardPagePrivate
{
    FileWizardPagePrivate();
    Ui::WizardPage m_ui;
    bool m_complete;
};

FileWizardPagePrivate::FileWizardPagePrivate() :
    m_complete(false)
{
}

FileWizardPage::FileWizardPage(QWidget *parent) :
    QWizardPage(parent),
    d(new FileWizardPagePrivate)
{
    d->m_ui.setupUi(this);
    connect(d->m_ui.pathChooser, SIGNAL(validChanged()), this, SLOT(slotValidChanged()));
    connect(d->m_ui.nameLineEdit, SIGNAL(validChanged()), this, SLOT(slotValidChanged()));

    connect(d->m_ui.pathChooser, SIGNAL(returnPressed()), this, SLOT(slotActivated()));
    connect(d->m_ui.nameLineEdit, SIGNAL(validReturnPressed()), this, SLOT(slotActivated()));
}

FileWizardPage::~FileWizardPage()
{
    delete d;
}

QString FileWizardPage::fileName() const
{
    return d->m_ui.nameLineEdit->text();
}

QString FileWizardPage::path() const
{
    return d->m_ui.pathChooser->path();
}

void FileWizardPage::setPath(const QString &path)
{
    d->m_ui.pathChooser->setPath(path);
}

void FileWizardPage::setFileName(const QString &name)
{
    d->m_ui.nameLineEdit->setText(name);
}

void FileWizardPage::changeEvent(QEvent *e)
{
    QWizardPage::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        d->m_ui.retranslateUi(this);
        break;
    default:
        break;
    }
}

bool FileWizardPage::isComplete() const
{
    return d->m_complete;
}

void FileWizardPage::setFileNameLabel(const QString &label)
{
    d->m_ui.nameLabel->setText(label);
}

void FileWizardPage::setPathLabel(const QString &label)
{
    d->m_ui.pathLabel->setText(label);
}

void FileWizardPage::slotValidChanged()
{
    const bool newComplete = d->m_ui.pathChooser->isValid() && d->m_ui.nameLineEdit->isValid();
    if (newComplete != d->m_complete) {
        d->m_complete = newComplete;
        emit completeChanged();
    }
}

void FileWizardPage::slotActivated()
{
    if (d->m_complete)
        emit activated();
}

bool FileWizardPage::validateBaseName(const QString &name, QString *errorMessage /* = 0*/)
{
    return FileNameValidatingLineEdit::validateFileName(name, false, errorMessage);
}

} // namespace Utils
