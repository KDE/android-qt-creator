/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "plugindetailsview.h"
#include "ui_plugindetailsview.h"
#include "pluginspec.h"

#include <QtCore/QDir>

/*!
    \class ExtensionSystem::PluginDetailsView
    \brief Widget that displays the contents of a PluginSpec.

    Can be used for integration in the application that
    uses the plugin manager.

    \sa ExtensionSystem::PluginView
*/

using namespace ExtensionSystem;

/*!
    \fn PluginDetailsView::PluginDetailsView(QWidget *parent)
    Constructs a new view with given \a parent widget.
*/
PluginDetailsView::PluginDetailsView(QWidget *parent)
	: QWidget(parent),
          m_ui(new Internal::Ui::PluginDetailsView())
{
    m_ui->setupUi(this);
}

/*!
    \fn PluginDetailsView::~PluginDetailsView()
    \internal
*/
PluginDetailsView::~PluginDetailsView()
{
    delete m_ui;
}

/*!
    \fn void PluginDetailsView::update(PluginSpec *spec)
    Reads the given \a spec and displays its values
    in this PluginDetailsView.
*/
void PluginDetailsView::update(PluginSpec *spec)
{
    m_ui->name->setText(spec->name());
    m_ui->version->setText(spec->version());
    m_ui->compatVersion->setText(spec->compatVersion());
    m_ui->vendor->setText(spec->vendor());
    const QString link = QString::fromLatin1("<a href=\"%1\">%1</a>").arg(spec->url());
    m_ui->url->setText(link);
    QString component = tr("None");
    if (!spec->category().isEmpty())
        component = spec->category();
    m_ui->component->setText(component);
    m_ui->location->setText(QDir::toNativeSeparators(spec->filePath()));
    m_ui->description->setText(spec->description());
    m_ui->copyright->setText(spec->copyright());
    m_ui->license->setText(spec->license());
    QStringList depStrings;
    foreach (const PluginDependency &dep, spec->dependencies()) {
        QString depString = dep.name;
        depString += QLatin1String(" (");
        depString += dep.version;
        if (dep.type == PluginDependency::Optional)
            depString += QLatin1String(", optional");
        depString += QLatin1Char(')');
        depStrings.append(depString);
    }
    m_ui->dependencies->addItems(depStrings);
}
