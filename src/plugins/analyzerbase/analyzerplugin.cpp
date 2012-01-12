/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#include "analyzerplugin.h"
#include "analyzerconstants.h"
#include "analyzermanager.h"
#include "ianalyzertool.h"
#include "analyzerruncontrolfactory.h"

#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

#include <QtCore/QtPlugin>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>
#include <QtGui/QApplication>

using namespace Analyzer;
using namespace Analyzer::Internal;

static AnalyzerPlugin *m_instance = 0;

////////////////////////////////////////////////////////////////////////
//
// AnalyzerPlugin
//
////////////////////////////////////////////////////////////////////////

AnalyzerPlugin::AnalyzerPlugin()
{
    m_instance = this;
}

AnalyzerPlugin::~AnalyzerPlugin()
{
    m_instance = 0;
}

bool AnalyzerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    (void) new AnalyzerManager(this);

    addAutoReleasedObject(new AnalyzerRunControlFactory());

    // Task integration.
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    ProjectExplorer::TaskHub *hub = pm->getObject<ProjectExplorer::TaskHub>();
    //: Category under which Analyzer tasks are listed in Issues view
    hub->addCategory(QLatin1String(Constants::ANALYZERTASK_ID), tr("Analyzer"));

    return true;
}

void AnalyzerPlugin::extensionsInitialized()
{
    AnalyzerManager::extensionsInitialized();
}

ExtensionSystem::IPlugin::ShutdownFlag AnalyzerPlugin::aboutToShutdown()
{
    AnalyzerManager::shutdown();
    return SynchronousShutdown;
}

AnalyzerPlugin *AnalyzerPlugin::instance()
{
    return m_instance;
}

Q_EXPORT_PLUGIN(AnalyzerPlugin)
