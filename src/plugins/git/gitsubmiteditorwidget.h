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

#ifndef GITSUBMITEDITORWIDGET_H
#define GITSUBMITEDITORWIDGET_H

#include "ui_gitsubmitpanel.h"

#include <utils/submiteditorwidget.h>

QT_BEGIN_NAMESPACE
class QValidator;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

struct GitSubmitEditorPanelInfo;
struct GitSubmitEditorPanelData;

/* Submit editor widget with 2 additional panes:
 * 1) Info with branch, description, etc
 * 2) Data, with author and email to edit.
 * The file contents is the submit message.
 * The previously added files will be added 'checked' to the file list, the
 * remaining un-added and untracked files will be added 'unchecked' for the
 * user to click. */

class GitSubmitEditorWidget : public Utils::SubmitEditorWidget
{
    Q_OBJECT

public:
    explicit GitSubmitEditorWidget(QWidget *parent = 0);


    GitSubmitEditorPanelData panelData() const;
    void setPanelData(const GitSubmitEditorPanelData &data);

    void setPanelInfo(const GitSubmitEditorPanelInfo &info);

protected:
    bool canSubmit() const;
    QString cleanupDescription(const QString &) const;

private slots:
    void authorInformationChanged();

private:
    bool emailIsValid() const;

    QWidget *m_gitSubmitPanel;
    Ui::GitSubmitPanel m_gitSubmitPanelUi;
    QValidator *m_emailValidator;
};

} // namespace Internal
} // namespace Perforce

#endif // GITSUBMITEDITORWIDGET_H
