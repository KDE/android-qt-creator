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

#include "buildenvironmentwidget.h"

#include "buildconfiguration.h"
#include "environmentwidget.h"

#include <utils/qtcassert.h>

#include <QtGui/QVBoxLayout>
#include <QtGui/QCheckBox>

using namespace ProjectExplorer;

BuildEnvironmentWidget::BuildEnvironmentWidget()
    : m_buildConfiguration(0)
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    m_clearSystemEnvironmentCheckBox = new QCheckBox(this);
    m_clearSystemEnvironmentCheckBox->setText(tr("Clear system environment"));

    m_buildEnvironmentWidget = new EnvironmentWidget(this, m_clearSystemEnvironmentCheckBox);
    vbox->addWidget(m_buildEnvironmentWidget);

    connect(m_buildEnvironmentWidget, SIGNAL(userChangesChanged()),
            this, SLOT(environmentModelUserChangesChanged()));
    connect(m_clearSystemEnvironmentCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(clearSystemEnvironmentCheckBoxClicked(bool)));
}

QString BuildEnvironmentWidget::displayName() const
{
    return tr("Build Environment");
}

void BuildEnvironmentWidget::init(BuildConfiguration *bc)
{
    Q_ASSERT(bc);

    if (m_buildConfiguration) {
        disconnect(m_buildConfiguration, SIGNAL(environmentChanged()),
                   this, SLOT(environmentChanged()));
        disconnect(m_buildConfiguration, SIGNAL(toolChainChanged()),
                   this, SLOT(environmentChanged()));
    }

    m_buildConfiguration = bc;

    connect(m_buildConfiguration, SIGNAL(environmentChanged()),
            this, SLOT(environmentChanged()));
    connect(m_buildConfiguration, SIGNAL(toolChainChanged()),
            this, SLOT(environmentChanged()));

    m_clearSystemEnvironmentCheckBox->setChecked(!m_buildConfiguration->useSystemEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironment(m_buildConfiguration->baseEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironmentText(m_buildConfiguration->baseEnvironmentText());
    m_buildEnvironmentWidget->setUserChanges(m_buildConfiguration->userEnvironmentChanges());
}

void BuildEnvironmentWidget::environmentModelUserChangesChanged()
{
    m_buildConfiguration->setUserEnvironmentChanges(m_buildEnvironmentWidget->userChanges());
}

void BuildEnvironmentWidget::clearSystemEnvironmentCheckBoxClicked(bool checked)
{
    m_buildConfiguration->setUseSystemEnvironment(!checked);
    m_buildEnvironmentWidget->setBaseEnvironment(m_buildConfiguration->baseEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironmentText(m_buildConfiguration->baseEnvironmentText());
}

void BuildEnvironmentWidget::environmentChanged()
{
    m_buildEnvironmentWidget->setBaseEnvironment(m_buildConfiguration->baseEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironmentText(m_buildConfiguration->baseEnvironmentText());
}
