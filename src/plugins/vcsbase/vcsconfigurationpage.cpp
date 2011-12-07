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

#include "vcsconfigurationpage.h"

#include "vcsbaseconstants.h"

#include "ui_vcsconfigurationpage.h"

#include <coreplugin/dialogs/iwizard.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>

namespace VCSBase {
namespace Internal {

class VcsConfigurationPagePrivate
{
public:
    VcsConfigurationPagePrivate() :
        m_ui(new Ui::VcsConfigurationPage)
    { }

    ~VcsConfigurationPagePrivate()
    {
        delete m_ui;
    }

    Ui::VcsConfigurationPage *m_ui;
    const Core::IVersionControl *m_versionControl;
};

} // namespace Internal

VcsConfigurationPage::VcsConfigurationPage(const Core::IVersionControl *vc, QWidget *parent) :
    QWizardPage(parent),
    d(new Internal::VcsConfigurationPagePrivate)
{
    Q_ASSERT(vc);
    setTitle(tr("Configuration"));
    setSubTitle(tr("Please configure <b>%1</b> now.").arg(vc->displayName()));

    d->m_versionControl = vc;

    connect(d->m_versionControl, SIGNAL(configurationChanged()),
            this, SIGNAL(completeChanged()));

    d->m_ui->setupUi(this);

    connect(d->m_ui->configureButton, SIGNAL(clicked()),
            this, SLOT(openConfiguration()));
}

VcsConfigurationPage::~VcsConfigurationPage()
{
    delete d->m_ui;
}

bool VcsConfigurationPage::isComplete() const
{
    return d->m_versionControl->isConfigured();
}

void VcsConfigurationPage::openConfiguration()
{
    Core::ICore *core = Core::ICore::instance();
    core->showOptionsDialog(VCSBase::Constants::VCS_SETTINGS_CATEGORY, d->m_versionControl->id());
}

} // namespace VCSBase
