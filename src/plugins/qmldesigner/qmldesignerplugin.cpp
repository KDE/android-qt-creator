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

#include "exception.h"
#include "qmldesignerplugin.h"
#include "qmldesignerconstants.h"
#include "pluginmanager.h"
#include "designmodewidget.h"
#include "settingspage.h"
#include "designmodecontext.h"

#include <qmljseditor/qmljseditorconstants.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <coreplugin/dialogs/iwizard.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/modemanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <integrationcore.h>

#include <QtGui/QAction>

#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>
#include <QtCore/qplugin.h>
#include <QtCore/QDebug>
#include <QtCore/QProcessEnvironment>

namespace QmlDesigner {
namespace Internal {

BauhausPlugin *BauhausPlugin::m_pluginInstance = 0;

bool shouldAssertInException()
{
    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    return !processEnvironment.value("QMLDESIGNER_ASSERT_ON_EXCEPTION").isEmpty();
}

BauhausPlugin::BauhausPlugin() :
    m_designerCore(0),
    m_designMode(0),
    m_isActive(false),
    m_revertToSavedAction(new QAction(this)),
    m_saveAction(new QAction(this)),
    m_saveAsAction(new QAction(this)),
    m_closeCurrentEditorAction(new QAction(this)),
    m_closeAllEditorsAction(new QAction(this)),
    m_closeOtherEditorsAction(new QAction(this))
{

    // Exceptions should never ever assert: they are handled in a number of
    // places where it is actually VALID AND EXPECTED BEHAVIOUR to get an
    // exception.
    // If you still want to see exactly where the exception originally
    // occurred, then you have various ways to do this:
    //  1. set a breakpoint on the constructor of the exception
    //  2. in gdb: "catch throw" or "catch throw Exception"
    //  3. set a breakpoint on __raise_exception()
    // And with gdb, you can even do this from your ~/.gdbinit file.
    // DnD is not working with gdb so this is still needed to get a good stacktrace

    Exception::setShouldAssert(shouldAssertInException());
}

BauhausPlugin::~BauhausPlugin()
{
    delete m_designerCore;
    Core::ICore::instance()->removeContextObject(m_context);
}

////////////////////////////////////////////////////
//
// INHERITED FROM ExtensionSystem::Plugin
//
////////////////////////////////////////////////////
bool BauhausPlugin::initialize(const QStringList & /*arguments*/, QString *error_message/* = 0*/) // =0;
{
    Core::ICore *core = Core::ICore::instance();

    const Core::Context switchContext(QmlDesigner::Constants::C_QMLDESIGNER,
        QmlJSEditor::Constants::C_QMLJSEDITOR_ID);

    Core::ActionManager *am = core->actionManager();

    QAction *switchAction = new QAction(tr("Switch Text/Design"), this);
    Core::Command *command = am->registerAction(switchAction, QmlDesigner::Constants::SWITCH_TEXT_DESIGN, switchContext);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_F4));

    m_designerCore = new QmlDesigner::IntegrationCore;
    m_pluginInstance = this;

#ifdef Q_OS_MAC
    const QString pluginPath = QCoreApplication::applicationDirPath() + "/../PlugIns/QmlDesigner";
#else
    const QString pluginPath = QCoreApplication::applicationDirPath() + "/../"
                               + QLatin1String(IDE_LIBRARY_BASENAME) + "/qmldesigner";
#endif

    m_designerCore->pluginManager()->setPluginPaths(QStringList() << pluginPath);

    createDesignModeWidget();
    connect(switchAction, SIGNAL(triggered()), this, SLOT(switchTextDesign()));

    addAutoReleasedObject(new SettingsPage);


    m_settings.fromSettings(core->settings());

    error_message->clear();

    return true;
}

void BauhausPlugin::createDesignModeWidget()
{
    Core::ICore *creatorCore = Core::ICore::instance();
    Core::ActionManager *actionManager = creatorCore->actionManager();
    m_editorManager = creatorCore->editorManager();
    Core::ActionContainer *editMenu = actionManager->actionContainer(Core::Constants::M_EDIT);

    m_mainWidget = new DesignModeWidget;

    m_context = new DesignModeContext(m_mainWidget);
    creatorCore->addContextObject(m_context);
    Core::Context qmlDesignerMainContext(Constants::C_QMLDESIGNER);
    Core::Context qmlDesignerFormEditorContext(Constants::C_QMLFORMEDITOR);

    // Revert to saved
    actionManager->registerAction(m_revertToSavedAction,
                                      Core::Constants::REVERTTOSAVED, qmlDesignerMainContext);
    connect(m_revertToSavedAction, SIGNAL(triggered()), m_editorManager, SLOT(revertToSaved()));

    //Save
    actionManager->registerAction(m_saveAction, Core::Constants::SAVE, qmlDesignerMainContext);
    connect(m_saveAction, SIGNAL(triggered()), m_editorManager, SLOT(saveFile()));

    //Save As
    actionManager->registerAction(m_saveAsAction, Core::Constants::SAVEAS, qmlDesignerMainContext);
    connect(m_saveAsAction, SIGNAL(triggered()), m_editorManager, SLOT(saveFileAs()));

    //Close Editor
    actionManager->registerAction(m_closeCurrentEditorAction, Core::Constants::CLOSE, qmlDesignerMainContext);
    connect(m_closeCurrentEditorAction, SIGNAL(triggered()), m_editorManager, SLOT(closeEditor()));

    //Close All
    actionManager->registerAction(m_closeAllEditorsAction, Core::Constants::CLOSEALL, qmlDesignerMainContext);
    connect(m_closeAllEditorsAction, SIGNAL(triggered()), m_editorManager, SLOT(closeAllEditors()));

    //Close All Others Action
    actionManager->registerAction(m_closeOtherEditorsAction, Core::Constants::CLOSEOTHERS, qmlDesignerMainContext);
    connect(m_closeOtherEditorsAction, SIGNAL(triggered()), m_editorManager, SLOT(closeOtherEditors()));

    // Undo / Redo
    actionManager->registerAction(m_mainWidget->undoAction(), Core::Constants::UNDO, qmlDesignerMainContext);
    actionManager->registerAction(m_mainWidget->redoAction(), Core::Constants::REDO, qmlDesignerMainContext);

    Core::Command *command;
    command = actionManager->registerAction(m_mainWidget->deleteAction(),
                                            QmlDesigner::Constants::DELETE, qmlDesignerFormEditorContext);
    command->setDefaultKeySequence(QKeySequence::Delete);
    command->setAttribute(Core::Command::CA_Hide); // don't show delete in other modes
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = actionManager->registerAction(m_mainWidget->cutAction(),
                                            Core::Constants::CUT, qmlDesignerFormEditorContext);
    command->setDefaultKeySequence(QKeySequence::Cut);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = actionManager->registerAction(m_mainWidget->copyAction(),
                                            Core::Constants::COPY, qmlDesignerFormEditorContext);
    command->setDefaultKeySequence(QKeySequence::Copy);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = actionManager->registerAction(m_mainWidget->pasteAction(),
                                            Core::Constants::PASTE, qmlDesignerFormEditorContext);
    command->setDefaultKeySequence(QKeySequence::Paste);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = actionManager->registerAction(m_mainWidget->selectAllAction(),
                                            Core::Constants::SELECTALL, qmlDesignerFormEditorContext);
    command->setDefaultKeySequence(QKeySequence::SelectAll);
    editMenu->addAction(command, Core::Constants::G_EDIT_SELECTALL);

    Core::ActionContainer *viewsMenu = actionManager->actionContainer(Core::Constants::M_WINDOW_VIEWS);

    command = actionManager->registerAction(m_mainWidget->toggleLeftSidebarAction(),
                                            Constants::TOGGLE_LEFT_SIDEBAR, qmlDesignerMainContext);
    command->setAttribute(Core::Command::CA_Hide);
    command->setDefaultKeySequence(QKeySequence("Ctrl+Alt+0"));
    viewsMenu->addAction(command);

    command = actionManager->registerAction(m_mainWidget->toggleRightSidebarAction(),
                                            Constants::TOGGLE_RIGHT_SIDEBAR, qmlDesignerMainContext);
    command->setAttribute(Core::Command::CA_Hide);
    command->setDefaultKeySequence(QKeySequence("Ctrl+Alt+Shift+0"));
    viewsMenu->addAction(command);

    command = actionManager->registerAction(m_mainWidget->restoreDefaultViewAction(),
                                            Constants::RESTORE_DEFAULT_VIEW, qmlDesignerMainContext);
    command->setAttribute(Core::Command::CA_Hide);
    viewsMenu->addAction(command);

    command = actionManager->registerAction(m_mainWidget->hideSidebarsAction(),
                                            Core::Constants::TOGGLE_SIDEBAR, qmlDesignerMainContext);

#ifdef Q_OS_MACX
    // add second shortcut to trigger delete
    QAction *deleteAction = new QAction(m_mainWidget);
    deleteAction->setShortcut(QKeySequence(QLatin1String("Backspace")));
    connect(deleteAction, SIGNAL(triggered()), m_mainWidget->deleteAction(),
            SIGNAL(triggered()));

    m_mainWidget->addAction(deleteAction);
#endif // Q_OS_MACX

    connect(m_editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(updateEditor(Core::IEditor*)));

    connect(m_editorManager, SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this, SLOT(textEditorsClosed(QList<Core::IEditor*>)));

    connect(creatorCore, SIGNAL(contextChanged(Core::IContext*,Core::Context)),
            this, SLOT(contextChanged(Core::IContext*,Core::Context)));

}

void BauhausPlugin::updateEditor(Core::IEditor *editor)
{
    Core::ICore *creatorCore = Core::ICore::instance();
    if (editor && editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID
        && creatorCore->modeManager()->currentMode() == m_designMode)
    {
        m_mainWidget->showEditor(editor);
    }
}

void BauhausPlugin::contextChanged(Core::IContext *context, const Core::Context &additionalContexts)
{
    Q_UNUSED(context)

    foreach (int additionalContext, additionalContexts) {
        if (m_context->context().contains(additionalContext)) {
            m_isActive = true;
            m_mainWidget->showEditor(m_editorManager->currentEditor());
            return;
        }
    }

    if (m_isActive) {
        m_isActive = false;
        m_mainWidget->showEditor(0);
    }
}

void BauhausPlugin::textEditorsClosed(QList<Core::IEditor*> editors)
{
    m_mainWidget->closeEditors(editors);
}

// copied from EditorManager::updateActions
void BauhausPlugin::updateActions(Core::IEditor* editor)
{
    Core::IEditor *curEditor = editor;
    int openedCount = m_editorManager->openedEditors().count()
                      + m_editorManager->openedEditorsModel()->restoredEditors().count();

    QString fName;
    if (curEditor) {
        if (!curEditor->file()->fileName().isEmpty()) {
            QFileInfo fi(curEditor->file()->fileName());
            fName = fi.fileName();
        } else {
            fName = curEditor->displayName();
        }
    }

    m_saveAction->setEnabled(curEditor != 0 && curEditor->file()->isModified());
    m_saveAsAction->setEnabled(curEditor != 0 && curEditor->file()->isSaveAsAllowed());
    m_revertToSavedAction->setEnabled(curEditor != 0
                                      && !curEditor->file()->fileName().isEmpty()
                                      && curEditor->file()->isModified());

    QString quotedName;
    if (!fName.isEmpty())
        quotedName = '"' + fName + '"';
    m_saveAsAction->setText(tr("Save %1 As...").arg(quotedName));
    m_saveAction->setText(tr("&Save %1").arg(quotedName));
    m_revertToSavedAction->setText(tr("Revert %1 to Saved").arg(quotedName));

    m_closeCurrentEditorAction->setEnabled(curEditor != 0);
    m_closeCurrentEditorAction->setText(tr("Close %1").arg(quotedName));
    m_closeAllEditorsAction->setEnabled(openedCount > 0);
    m_closeOtherEditorsAction->setEnabled(openedCount > 1);
    m_closeOtherEditorsAction->setText((openedCount > 1 ? tr("Close All Except %1").arg(quotedName) : tr("Close Others")));
}

void BauhausPlugin::extensionsInitialized()
{
    m_designMode = ExtensionSystem::PluginManager::instance()->getObject<Core::DesignMode>();

    m_mimeTypes << "application/x-qml" << "application/javascript"
                << "application/x-javascript" << "text/javascript";

    m_designMode->registerDesignWidget(m_mainWidget, m_mimeTypes, m_context->context());
    connect(m_designMode, SIGNAL(actionsUpdated(Core::IEditor*)), SLOT(updateActions(Core::IEditor*)));
}

BauhausPlugin *BauhausPlugin::pluginInstance()
{
    return m_pluginInstance;
}

void BauhausPlugin::switchTextDesign()
{
    Core::ModeManager *modeManager = Core::ModeManager::instance();
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->currentEditor();


    if (modeManager->currentMode()->id() == Core::Constants::MODE_EDIT) {
        if (editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
            modeManager->activateMode(Core::Constants::MODE_DESIGN);
            m_mainWidget->setFocus();
        }
    } else if (modeManager->currentMode()->id() == Core::Constants::MODE_DESIGN) {
        modeManager->activateMode(Core::Constants::MODE_EDIT);
    }
}

DesignerSettings BauhausPlugin::settings() const
{
    return m_settings;
}

void BauhausPlugin::setSettings(const DesignerSettings &s)
{
    if (s != m_settings) {
        m_settings = s;
        if (QSettings *settings = Core::ICore::instance()->settings())
            m_settings.toSettings(settings);
    }
}

}
}

Q_EXPORT_PLUGIN(QmlDesigner::Internal::BauhausPlugin)
