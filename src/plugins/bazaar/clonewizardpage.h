/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Hugues Delorme
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

#ifndef CLONEWIZARDPAGE_H
#define CLONEWIZARDPAGE_H

#include <vcsbase/basecheckoutwizardpage.h>

namespace Bazaar {
namespace Internal {
class CloneOptionsPanel;

class CloneWizardPage : public VcsBase::BaseCheckoutWizardPage
{
    Q_OBJECT

public:
    CloneWizardPage(QWidget *parent = 0);
    const CloneOptionsPanel *cloneOptionsPanel() const;

protected:
    QString directoryFromRepository(const QString &repository) const;

private:
    CloneOptionsPanel *m_optionsPanel;
};

} // namespace Internal
} // namespace Bazaar

#endif // CLONEWIZARDPAGE_H
