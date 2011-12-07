/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
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

#ifndef BAZAARCOMMITWIDGET_H
#define BAZAARCOMMITWIDGET_H

#include "ui_bazaarcommitpanel.h"

#include <utils/submiteditorwidget.h>

namespace Bazaar {
namespace Internal {

class BranchInfo;

/*submit editor widget based on git SubmitEditor
  Some extra fields have been added to the standard SubmitEditorWidget,
  to help to conform to the commit style that is used by both git and Bazaar*/

class BazaarCommitWidget : public Utils::SubmitEditorWidget
{

public:
    explicit BazaarCommitWidget(QWidget *parent = 0);

    void setFields(const BranchInfo &branch,
                   const QString &userName, const QString &email);

    QString committer() const;
    QStringList fixedBugs() const;
    bool isLocalOptionEnabled() const;

private:
    QWidget *m_bazaarCommitPanel;
    Ui::BazaarCommitPanel m_bazaarCommitPanelUi;
};

} // namespace Internal
} // namespace Bazaar

#endif // BAZAARCOMMITWIDGET_H
