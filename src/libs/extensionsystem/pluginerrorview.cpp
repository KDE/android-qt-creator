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

#include "pluginerrorview.h"
#include "ui_pluginerrorview.h"
#include "pluginspec.h"

#include <QtCore/QString>

/*!
    \class ExtensionSystem::PluginErrorView
    \brief Widget that displays the state and error message of a PluginSpec.

    Can be used for integration in the application that
    uses the plugin manager.

    \sa ExtensionSystem::PluginView
*/

using namespace ExtensionSystem;

/*!
    \fn PluginErrorView::PluginErrorView(QWidget *parent)
    Constructs a new error view with given \a parent widget.
*/
PluginErrorView::PluginErrorView(QWidget *parent)
    : QWidget(parent),
      m_ui(new Internal::Ui::PluginErrorView())
{
    m_ui->setupUi(this);
}

/*!
    \fn PluginErrorView::~PluginErrorView()
    \internal
*/
PluginErrorView::~PluginErrorView()
{
    delete m_ui;
}

/*!
    \fn void PluginErrorView::update(PluginSpec *spec)
    Reads the given \a spec and displays its state and
    error information in this PluginErrorView.
*/
void PluginErrorView::update(PluginSpec *spec)
{
    QString text;
    QString tooltip;
    switch (spec->state()) {
    case PluginSpec::Invalid:
        text = tr("Invalid");
        tooltip = tr("Description file found, but error on read");
        break;
    case PluginSpec::Read:
        text = tr("Read");
        tooltip = tr("Description successfully read");
        break;
    case PluginSpec::Resolved:
        text = tr("Resolved");
        tooltip = tr("Dependencies are successfully resolved");
        break;
    case PluginSpec::Loaded:
        text = tr("Loaded");
        tooltip = tr("Library is loaded");
        break;
    case PluginSpec::Initialized:
        text = tr("Initialized");
        tooltip = tr("Plugin's initialization method succeeded");
        break;
    case PluginSpec::Running:
        text = tr("Running");
        tooltip = tr("Plugin successfully loaded and running");
        break;
    case PluginSpec::Stopped:
        text = tr("Stopped");
        tooltip = tr("Plugin was shut down");
        break;
    case PluginSpec::Deleted:
        text = tr("Deleted");
        tooltip = tr("Plugin ended its life cycle and was deleted");
        break;
    }

    m_ui->state->setText(text);
    m_ui->state->setToolTip(tooltip);
    m_ui->errorString->setText(spec->errorString());
}
