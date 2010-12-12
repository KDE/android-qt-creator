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
#ifndef ANDROIDPROFILESUPDATEDIALOG_H
#define ANDROIDPROFILESUPDATEDIALOG_H

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
    class AndroidProFilesUpdateDialog;
}
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class AndroidProFilesUpdateDialog : public QDialog
{
    Q_OBJECT

public:

    explicit AndroidProFilesUpdateDialog(QWidget *parent = 0);
    ~AndroidProFilesUpdateDialog();
    //QList<UpdateSetting> getUpdateSettings() const;

private:
    Q_SLOT void checkAll();
    Q_SLOT void uncheckAll();
    void setCheckStateForAll(Qt::CheckState checkState);

    Ui::AndroidProFilesUpdateDialog *ui;
};

} // namespace Qt4ProjectManager
} // namespace Internal

#endif // ANDROIDPROFILESUPDATEDIALOG_H
