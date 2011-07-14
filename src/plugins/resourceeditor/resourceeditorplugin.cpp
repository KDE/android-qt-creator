/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "resourceeditorplugin.h"

#include "resourceeditorw.h"
#include "resourceeditorconstants.h"
#include "resourcewizard.h"
#include "resourceeditorfactory.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QtCore/QtPlugin>
#include <QtCore/QCoreApplication>
#include <QtGui/QAction>

using namespace ResourceEditor::Internal;

ResourceEditorPlugin::ResourceEditorPlugin() :
    m_wizard(0),
    m_editor(0),
    m_redoAction(0),
    m_undoAction(0)
{
}

ResourceEditorPlugin::~ResourceEditorPlugin()
{
    removeObject(m_editor);
    removeObject(m_wizard);
}

bool ResourceEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/resourceeditor/ResourceEditor.mimetypes.xml"), errorMessage))
        return false;

    m_editor = new ResourceEditorFactory(this);
    addObject(m_editor);

    Core::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);
    wizardParameters.setDescription(tr("Creates a Qt Resource file (.qrc) that you can add to a Qt Widget Project."));
    wizardParameters.setDisplayName(tr("Qt Resource file"));
    wizardParameters.setId(QLatin1String("F.Resource"));
    wizardParameters.setCategory(QLatin1String(Core::Constants::WIZARD_CATEGORY_QT));
    wizardParameters.setDisplayCategory(QCoreApplication::translate("Core", Core::Constants::WIZARD_TR_CATEGORY_QT));

    m_wizard = new ResourceWizard(wizardParameters, this);
    addObject(m_wizard);

    errorMessage->clear();

    // Register undo and redo
    const Core::Context context(Constants::C_RESOURCEEDITOR);
    m_undoAction = new QAction(tr("&Undo"), this);
    m_redoAction = new QAction(tr("&Redo"), this);
    Core::ActionManager * const actionManager = core->actionManager();
    actionManager->registerAction(m_undoAction, Core::Constants::UNDO, context);
    actionManager->registerAction(m_redoAction, Core::Constants::REDO, context);
    connect(m_undoAction, SIGNAL(triggered()), this, SLOT(onUndo()));
    connect(m_redoAction, SIGNAL(triggered()), this, SLOT(onRedo()));

    return true;
}

void ResourceEditorPlugin::extensionsInitialized()
{
}

void ResourceEditorPlugin::onUndo()
{
    currentEditor()->onUndo();
}

void ResourceEditorPlugin::onRedo()
{
    currentEditor()->onRedo();
}

void ResourceEditorPlugin::onUndoStackChanged(ResourceEditorW const *editor,
        bool canUndo, bool canRedo)
{
    if (editor == currentEditor()) {
        m_undoAction->setEnabled(canUndo);
        m_redoAction->setEnabled(canRedo);
    }
}

ResourceEditorW * ResourceEditorPlugin::currentEditor() const
{
    ResourceEditorW * const focusEditor = qobject_cast<ResourceEditorW *>(
            Core::EditorManager::instance()->currentEditor());
    QTC_ASSERT(focusEditor, return 0);
    return focusEditor;
}

Q_EXPORT_PLUGIN(ResourceEditorPlugin)
