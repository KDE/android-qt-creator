/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#include "srcdestdialog.h"
#include "ui_srcdestdialog.h"


using namespace Mercurial::Internal;

SrcDestDialog::SrcDestDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::SrcDestDialog)
{
    m_ui->setupUi(this);
    m_ui->localPathChooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
}

SrcDestDialog::~SrcDestDialog()
{
    delete m_ui;
}

void SrcDestDialog::setPathChooserKind(Utils::PathChooser::Kind kind)
{
    m_ui->localPathChooser->setExpectedKind(kind);
}

QString SrcDestDialog::getRepositoryString() const
{
    if (m_ui->defaultButton->isChecked())
        return QString();
    if (m_ui->localButton->isChecked())
        return m_ui->localPathChooser->path();
    return m_ui->urlLineEdit->text();
}

void SrcDestDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
