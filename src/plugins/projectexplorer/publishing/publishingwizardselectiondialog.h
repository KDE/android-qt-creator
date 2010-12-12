/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#ifndef PUBLISHINGWIZARDSELECTIONDIALOG_H
#define PUBLISHINGWIZARDSELECTIONDIALOG_H

#include <QtCore/QList>
#include <QtGui/QDialog>

QT_FORWARD_DECLARE_CLASS(QWizard);

namespace ProjectExplorer {
class IPublishingWizardFactory;
class Project;
namespace Internal {
namespace Ui {
class PublishingWizardSelectionDialog;
}

class PublishingWizardSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PublishingWizardSelectionDialog(const Project *project,
        QWidget *parent = 0);
    ~PublishingWizardSelectionDialog();
    QWizard *createSelectedWizard() const;

private slots:
    void handleWizardIndexChanged(int index);

private:
    Ui::PublishingWizardSelectionDialog *ui;

    const Project * const m_project;
    QList<const IPublishingWizardFactory *> m_factories;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PUBLISHINGWIZARDSELECTIONDIALOG_H
