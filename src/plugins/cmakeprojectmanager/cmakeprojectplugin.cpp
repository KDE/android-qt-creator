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

#include "cmakeprojectplugin.h"
#include "cmakeprojectmanager.h"
#include "cmakerunconfiguration.h"
#include "cmakeeditorfactory.h"
#include "makestep.h"
#include "cmakeprojectconstants.h"
#include "cmaketarget.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <texteditor/texteditoractionhandler.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>


using namespace CMakeProjectManager::Internal;

CMakeProjectPlugin::CMakeProjectPlugin()
{
}

CMakeProjectPlugin::~CMakeProjectPlugin()
{
}

bool CMakeProjectPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":cmakeproject/CMakeProject.mimetypes.xml"), errorMessage))
        return false;
    CMakeSettingsPage *cmp = new CMakeSettingsPage();
    addAutoReleasedObject(cmp);
    CMakeManager *manager = new CMakeManager(cmp);
    addAutoReleasedObject(manager);
    addAutoReleasedObject(new MakeStepFactory);
    addAutoReleasedObject(new CMakeRunConfigurationFactory);
    TextEditor::TextEditorActionHandler *editorHandler
           = new TextEditor::TextEditorActionHandler(CMakeProjectManager::Constants::C_CMAKEEDITOR);

    addAutoReleasedObject(new CMakeEditorFactory(manager, editorHandler));
    addAutoReleasedObject(new CMakeTargetFactory);

    return true;
}

void CMakeProjectPlugin::extensionsInitialized()
{
}

Q_EXPORT_PLUGIN(CMakeProjectPlugin)
