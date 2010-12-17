/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "codepastersettings.h"
#include "cpasterconstants.h"

#include <coreplugin/icore.h>

#include <QtCore/QSettings>
#include <QtCore/QCoreApplication>
#include <QtGui/QLineEdit>
#include <QtGui/QFileDialog>
#include <QtGui/QGroupBox>
#include <QtGui/QFormLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtCore/QDebug>
#include <QtCore/QVariant>

static const char settingsGroupC[] = "CodePasterSettings";
static const char serverKeyC[] = "Server";

namespace CodePaster {

CodePasterSettingsPage::CodePasterSettingsPage()
{
    m_settings = Core::ICore::instance()->settings();
    if (m_settings) {
        const QString keyRoot = QLatin1String(settingsGroupC) + QLatin1Char('/');
        m_host = m_settings->value(keyRoot + QLatin1String(serverKeyC), QString()).toString();
    }
}

QString CodePasterSettingsPage::id() const
{
    return QLatin1String("C.CodePaster");
}

QString CodePasterSettingsPage::displayName() const
{
    return tr("CodePaster");
}

QString CodePasterSettingsPage::category() const
{
    return QLatin1String(Constants::CPASTER_SETTINGS_CATEGORY);
}

QString CodePasterSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("CodePaster", Constants::CPASTER_SETTINGS_TR_CATEGORY);
}

QIcon CodePasterSettingsPage::categoryIcon() const
{
    return QIcon();
}

QWidget *CodePasterSettingsPage::createPage(QWidget *parent)
{
    QWidget *outerWidget = new QWidget(parent);
    QVBoxLayout *outerLayout = new QVBoxLayout(outerWidget);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    QLineEdit *lineEdit = new QLineEdit(m_host);
    connect(lineEdit, SIGNAL(textChanged(QString)), this, SLOT(serverChanged(QString)));
    formLayout->addRow(tr("Server:"), lineEdit);
    outerLayout->addLayout(formLayout);
    outerLayout->addSpacerItem(new QSpacerItem(0, 30, QSizePolicy::Ignored, QSizePolicy::Fixed));

    QLabel *noteLabel = new QLabel(tr("Note: Specify the host name for the CodePaster service "
                                      "without any protocol prepended (e.g. codepaster.mycompany.com)."));
    noteLabel->setWordWrap(true);
    outerLayout->addWidget(noteLabel);

    outerLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    return outerWidget;
}

void CodePasterSettingsPage::apply()
{
    if (!m_settings)
        return;

    m_settings->beginGroup(QLatin1String(settingsGroupC));
    m_settings->setValue(QLatin1String(serverKeyC), m_host);
    m_settings->endGroup();
}

void CodePasterSettingsPage::serverChanged(const QString &host)
{
    m_host = host;
}

QString CodePasterSettingsPage::hostName() const
{
    return m_host;
}
} // namespace CodePaster
