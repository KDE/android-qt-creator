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

#include "formeditorw.h"
#include "formwindoweditor.h"
#include "designerconstants.h"
#include "settingsmanager.h"
#include "settingspage.h"
#include "editorwidget.h"
#include "editordata.h"
#include "qtcreatorintegration.h"
#include "designerxmleditor.h"
#include "designercontext.h"
#include "editorwidget.h"
#include "resourcehandler.h"
#include <widgethost.h>

#include <coreplugin/editortoolbar.h>
#include <coreplugin/designmode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/outputpane.h>
#include <texteditor/texteditorsettings.h>
#include <extensionsystem/pluginmanager.h>
#include <cpptools/cpptoolsconstants.h>
#include <utils/qtcassert.h>

#include <QtDesigner/QDesignerFormEditorPluginInterface>
#include "qt_private/pluginmanager_p.h"

#include "qt_private/iconloader_p.h"  // createIconSet
#include "qt_private/qdesigner_formwindowmanager_p.h"
#include "qt_private/formwindowbase_p.h"
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerComponents>

#include <QtDesigner/QDesignerWidgetBoxInterface>
#include <QtDesigner/abstractobjectinspector.h>
#include <QtDesigner/QDesignerPropertyEditorInterface>
#include <QtDesigner/QDesignerActionEditorInterface>
#include <QtDesigner/QDesignerFormEditorInterface>

#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QDockWidget>
#include <QtGui/QMenu>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QKeySequence>
#include <QtGui/QPrintDialog>
#include <QtGui/QPrinter>
#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtGui/QToolBar>
#include <QtGui/QVBoxLayout>

#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QSignalMapper>
#include <QtCore/QPluginLoader>
#include <QtCore/QTime>

static const char settingsGroup[] = "Designer";

#ifdef Q_OS_MAC
    enum { osMac = 1 };
#else
    enum { osMac = 0 };
#endif

/* Actions of the designer plugin:
 * Designer provides a toolbar which is subject to a context change (to
 * "edit mode" context) when it is focused.
 * In order to prevent its actions from being disabled/hidden by that context
 * change, the actions are registered on the global context. In currentEditorChanged(),
 * the ones that are present in the global edit menu are set visible/invisible manually.
 * The designer context is currently used for Cut/Copy/Paste, etc. */

static inline QIcon designerIcon(const QString &iconName)
{
    const QIcon icon = qdesigner_internal::createIconSet(iconName);
    if (icon.isNull())
        qWarning() << "Unable to locate " << iconName;
    return icon;
}

// Create a menu separator
static inline QAction *createSeparator(QObject *parent,
                                 Core::ActionManager *am,
                                 const Core::Context &context,
                                 Core::ActionContainer *container,
                                 const QString &name = QString(),
                                 const QString &group = QString())
{
    QAction *actSeparator = new QAction(parent);
    actSeparator->setSeparator(true);
    Core::Command *command = am->registerAction(actSeparator, name, context);
    container->addAction(command, group);
    return actSeparator;
}

using namespace Designer::Constants;

namespace Designer {
namespace Internal {

// --------- FormEditorW

FormEditorW *FormEditorW::m_self = 0;

FormEditorW::FormEditorW() :
    m_formeditor(QDesignerComponents::createFormEditor(0)),
    m_integration(0),
    m_fwm(0),
    m_core(Core::ICore::instance()),
    m_initStage(RegisterPlugins),
    m_actionGroupEditMode(0),
    m_actionPrint(0),
    m_actionPreview(0),
    m_actionGroupPreviewInStyle(0),
    m_previewInStyleMenu(0),
    m_actionAboutPlugins(0),
    m_actionSwitchSource(0),
    m_shortcutMapper(new QSignalMapper(this)),
    m_context(0),
    m_modeWidget(0),
    m_editorWidget(0),
    m_designMode(0),
    m_editorToolBar(0),
    m_toolBar(0)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO;
    QTC_ASSERT(!m_self, return);
    m_self = this;
    QTC_ASSERT(m_core, return);

    qFill(m_designerSubWindows, m_designerSubWindows + Designer::Constants::DesignerSubWindowCount,
          static_cast<QWidget *>(0));

    m_formeditor->setTopLevel(qobject_cast<QWidget *>(m_core->editorManager()));
    m_formeditor->setSettingsManager(new SettingsManager());

    m_fwm = qobject_cast<qdesigner_internal::QDesignerFormWindowManager*>(m_formeditor->formWindowManager());
    QTC_ASSERT(m_fwm, return);

    m_contexts.add(Designer::Constants::C_FORMEDITOR);

    setupActions();

    foreach (QDesignerOptionsPageInterface *designerPage, m_formeditor->optionsPages()) {
        SettingsPage *settingsPage = new SettingsPage(designerPage);
        m_settingsPages.append(settingsPage);
    }

    connect(m_core->editorManager(), SIGNAL(currentEditorChanged(Core::IEditor *)),
            this, SLOT(currentEditorChanged(Core::IEditor *)));
    connect(m_shortcutMapper, SIGNAL(mapped(QObject *)),
            this, SLOT(updateShortcut(QObject *)));
}

FormEditorW::~FormEditorW()
{
    if (m_context)
        m_core->removeContextObject(m_context);
    if (m_initStage == FullyInitialized) {
        if (QSettings *s = m_core->settings()) {
            m_core->settings()->beginGroup(settingsGroup);
            m_editorWidget->saveSettings(s);
            s->endGroup();
        }

        m_designMode->unregisterDesignWidget(m_modeWidget);
        delete m_modeWidget;
        m_modeWidget = 0;
    }

    delete m_formeditor;
    qDeleteAll(m_settingsPages);
    m_settingsPages.clear();
    delete m_integration;

    m_self = 0;
}

// Add an actioon to toggle the view state of a dock window
void FormEditorW::addDockViewAction(Core::ActionManager *am,
                                    Core::ActionContainer *viewMenu,
                                    int index, const Core::Context &context,
                                    const QString &title, const QString &id)
{
    if (const QDockWidget *dw = m_editorWidget->designerDockWidgets()[index]) {
        QAction *action = dw->toggleViewAction();
        action->setText(title);
        Core::Command *cmd = addToolAction(action, am, context, id, viewMenu, QString());
        cmd->setAttribute(Core::Command::CA_Hide);
    }
}

void FormEditorW::setupViewActions()
{
    // Populate "View" menu of form editor menu
    Core::ActionManager *am = m_core->actionManager();
    Core::ActionContainer *viewMenu = am->actionContainer(Core::Id(Core::Constants::M_WINDOW_VIEWS));
    QTC_ASSERT(viewMenu, return)

    addDockViewAction(am, viewMenu, WidgetBoxSubWindow, m_contexts,
                      tr("Widget box"), QLatin1String("FormEditor.WidgetBox"));

    addDockViewAction(am, viewMenu, ObjectInspectorSubWindow, m_contexts,
                      tr("Object Inspector"), QLatin1String("FormEditor.ObjectInspector"));

    addDockViewAction(am, viewMenu, PropertyEditorSubWindow, m_contexts,
                      tr("Property Editor"), QLatin1String("FormEditor.PropertyEditor"));

    addDockViewAction(am, viewMenu, SignalSlotEditorSubWindow, m_contexts,
                      tr("Signals && Slots Editor"), QLatin1String("FormEditor.SignalsAndSlotsEditor"));

    addDockViewAction(am, viewMenu, ActionEditorSubWindow, m_contexts,
                      tr("Action Editor"), QLatin1String("FormEditor.ActionEditor"));
    // Lock/Reset
    Core::Command *cmd = addToolAction(m_editorWidget->menuSeparator1(), am, m_contexts, QLatin1String("FormEditor.SeparatorLock"), viewMenu, QString());
    cmd->setAttribute(Core::Command::CA_Hide);

    cmd = addToolAction(m_editorWidget->toggleLockedAction(), am, m_contexts, QLatin1String("FormEditor.Locked"), viewMenu, QString());
    cmd->setAttribute(Core::Command::CA_Hide);

    cmd = addToolAction(m_editorWidget->menuSeparator2(), am, m_contexts, QLatin1String("FormEditor.SeparatorReset"), viewMenu, QString());
    cmd->setAttribute(Core::Command::CA_Hide);

    cmd = addToolAction(m_editorWidget->resetLayoutAction(), am, m_contexts, QLatin1String("FormEditor.ResetToDefaultLayout"), viewMenu, QString());
    connect(m_editorWidget, SIGNAL(resetLayout()), m_editorWidget, SLOT(resetToDefaultLayout()));
    cmd->setAttribute(Core::Command::CA_Hide);
}

void FormEditorW::fullInit()
{
    QTC_ASSERT(m_initStage == RegisterPlugins, return);
    QTime *initTime = 0;
    if (Designer::Constants::Internal::debug) {
        initTime = new QTime;
        initTime->start();
    }

    QDesignerComponents::createTaskMenu(m_formeditor, parent());
    QDesignerComponents::initializePlugins(designerEditor());
    QDesignerComponents::initializeResources();
    initDesignerSubWindows();
    m_integration = new QtCreatorIntegration(m_formeditor, this);
    m_formeditor->setIntegration(m_integration);
    // Connect Qt Designer help request to HelpManager.
    connect(m_integration, SIGNAL(creatorHelpRequested(QUrl)),
        Core::HelpManager::instance(), SIGNAL(helpRequested(QUrl)));

    /**
     * This will initialize our TabOrder, Signals and slots and Buddy editors.
     */
    QList<QObject*> plugins = QPluginLoader::staticInstances();
    plugins += m_formeditor->pluginManager()->instances();
    foreach (QObject *plugin, plugins) {
        if (QDesignerFormEditorPluginInterface *formEditorPlugin = qobject_cast<QDesignerFormEditorPluginInterface*>(plugin)) {
            if (!formEditorPlugin->isInitialized())
                formEditorPlugin->initialize(m_formeditor);
        }
    }

    if (m_actionAboutPlugins)
        m_actionAboutPlugins->setEnabled(true);

    if (Designer::Constants::Internal::debug) {
        qDebug() << Q_FUNC_INFO << initTime->elapsed() << "ms";
        delete initTime;
    }

    connect(m_core->editorManager()->instance(), SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            SLOT(closeFormEditorsForXmlEditors(QList<Core::IEditor*>)));
    // Nest toolbar and editor widget
    m_editorWidget = new EditorWidget(this);
    QSettings *settings = m_core->settings();
    settings->beginGroup(settingsGroup);
    m_editorWidget->restoreSettings(settings);
    settings->endGroup();

    m_editorToolBar = createEditorToolBar();
    m_toolBar = Core::EditorManager::createToolBar();
    m_toolBar->setToolbarCreationFlags(Core::EditorToolBar::FlagsStandalone);
    m_toolBar->setNavigationVisible(false);
    m_toolBar->addCenterToolBar(m_editorToolBar);

    m_designMode = ExtensionSystem::PluginManager::instance()->getObject<Core::DesignMode>();
    m_modeWidget = new QWidget;
    m_modeWidget->setObjectName(QLatin1String("DesignerModeWidget"));
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_toolBar);
    // Avoid mode switch to 'Edit' mode when the application started by
    // 'Run' in 'Design' mode emits output.
    Core::MiniSplitter *splitter = new Core::MiniSplitter(Qt::Vertical);
    splitter->addWidget(m_editorWidget);
    QWidget *outputPane = new Core::OutputPanePlaceHolder(m_designMode, splitter);
    outputPane->setObjectName(QLatin1String("DesignerOutputPanePlaceHolder"));
    splitter->addWidget(outputPane);
    layout->addWidget(splitter);
    m_modeWidget->setLayout(layout);

    Core::Context designerContexts = m_contexts;
    designerContexts.add(Core::Constants::C_EDITORMANAGER);
    m_context = new DesignerContext(designerContexts, m_modeWidget, this);
    m_core->addContextObject(m_context);

    m_designMode->registerDesignWidget(m_modeWidget, QStringList(QLatin1String(FORM_MIMETYPE)), m_contexts);

    setupViewActions();

    m_initStage = FullyInitialized;
}

void FormEditorW::initDesignerSubWindows()
{
    qFill(m_designerSubWindows, m_designerSubWindows + Designer::Constants::DesignerSubWindowCount, static_cast<QWidget*>(0));

    QDesignerWidgetBoxInterface *wb = QDesignerComponents::createWidgetBox(m_formeditor, 0);
    wb->setWindowTitle(tr("Widget Box"));
    wb->setObjectName(QLatin1String("WidgetBox"));
    m_formeditor->setWidgetBox(wb);
    m_designerSubWindows[WidgetBoxSubWindow] = wb;

    QDesignerObjectInspectorInterface *oi = QDesignerComponents::createObjectInspector(m_formeditor, 0);
    oi->setWindowTitle(tr("Object Inspector"));
    oi->setObjectName(QLatin1String("ObjectInspector"));
    m_formeditor->setObjectInspector(oi);
    m_designerSubWindows[ObjectInspectorSubWindow] = oi;

    QDesignerPropertyEditorInterface *pe = QDesignerComponents::createPropertyEditor(m_formeditor, 0);
    pe->setWindowTitle(tr("Property Editor"));
    pe->setObjectName(QLatin1String("PropertyEditor"));
    m_formeditor->setPropertyEditor(pe);
    m_designerSubWindows[PropertyEditorSubWindow] = pe;

    QWidget *se = QDesignerComponents::createSignalSlotEditor(m_formeditor, 0);
    se->setWindowTitle(tr("Signals && Slots Editor"));
    se->setObjectName(QLatin1String("SignalsAndSlotsEditor"));
    m_designerSubWindows[SignalSlotEditorSubWindow] = se;

    QDesignerActionEditorInterface *ae = QDesignerComponents::createActionEditor(m_formeditor, 0);
    ae->setWindowTitle(tr("Action Editor"));
    ae->setObjectName(QLatin1String("ActionEditor"));
    m_formeditor->setActionEditor(ae);
    m_designerSubWindows[ActionEditorSubWindow] = ae;
}

QList<Core::IOptionsPage *> FormEditorW::optionsPages() const
{
    return m_settingsPages;
}

void FormEditorW::ensureInitStage(InitializationStage s)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << s;
    if (!m_self)
        m_self = new FormEditorW;
    if (m_self->m_initStage >= s)
        return;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_self->fullInit();
    QApplication::restoreOverrideCursor();
}

FormEditorW *FormEditorW::instance()
{
    ensureInitStage(FullyInitialized);
    return m_self;
}

void FormEditorW::deleteInstance()
{
    delete m_self;
}

void FormEditorW::setupActions()
{
    Core::ActionManager *am = m_core->actionManager();
    Core::Command *command;

    //menus
    Core::ActionContainer *medit =
        am->actionContainer(Core::Constants::M_EDIT);
    Core::ActionContainer *mtools =
        am->actionContainer(Core::Constants::M_TOOLS);

    Core::ActionContainer *mformtools =
        am->createMenu(M_FORMEDITOR);
    mformtools->menu()->setTitle(tr("For&m Editor"));
    mtools->addMenu(mformtools);

    //overridden actions
    bindShortcut(am->registerAction(m_fwm->actionUndo(), Core::Constants::UNDO, m_contexts), m_fwm->actionUndo());
    bindShortcut(am->registerAction(m_fwm->actionRedo(), Core::Constants::REDO, m_contexts), m_fwm->actionRedo());
    bindShortcut(am->registerAction(m_fwm->actionCut(), Core::Constants::CUT, m_contexts), m_fwm->actionCut());
    bindShortcut(am->registerAction(m_fwm->actionCopy(), Core::Constants::COPY, m_contexts), m_fwm->actionCopy());
    bindShortcut(am->registerAction(m_fwm->actionPaste(), Core::Constants::PASTE, m_contexts), m_fwm->actionPaste());
    bindShortcut(am->registerAction(m_fwm->actionSelectAll(), Core::Constants::SELECTALL, m_contexts), m_fwm->actionSelectAll());

    m_actionPrint = new QAction(this);
    bindShortcut(am->registerAction(m_actionPrint, Core::Constants::PRINT, m_contexts), m_actionPrint);
    connect(m_actionPrint, SIGNAL(triggered()), this, SLOT(print()));

    //'delete' action. Do not set a shortcut as Designer handles
    // the 'Delete' key by event filter. Setting a shortcut triggers
    // buggy behaviour on Mac (Pressing Delete in QLineEdit removing the widget).
    command = am->registerAction(m_fwm->actionDelete(), Core::Id("FormEditor.Edit.Delete"), m_contexts);
    bindShortcut(command, m_fwm->actionDelete());
    command->setAttribute(Core::Command::CA_Hide);
    medit->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    m_actionGroupEditMode = new QActionGroup(this);
    m_actionGroupEditMode->setExclusive(true);
    connect(m_actionGroupEditMode, SIGNAL(triggered(QAction*)), this, SLOT(activateEditMode(QAction*)));

    m_modeActionSeparator = new QAction(this);
    m_modeActionSeparator->setSeparator(true);
    command = am->registerAction(m_modeActionSeparator, Core::Id("FormEditor.Sep.ModeActions"), m_contexts);
    medit->addAction(command, Core::Constants::G_EDIT_OTHER);

    m_toolActionIds.push_back(QLatin1String("FormEditor.WidgetEditor"));
    createEditModeAction(m_actionGroupEditMode, m_contexts, am, medit,
                         tr("Edit Widgets"), m_toolActionIds.back(),
                         EditModeWidgetEditor, QLatin1String("widgettool.png"), tr("F3"));

    m_toolActionIds.push_back(QLatin1String("FormEditor.SignalsSlotsEditor"));
    createEditModeAction(m_actionGroupEditMode, m_contexts, am, medit,
                         tr("Edit Signals/Slots"), m_toolActionIds.back(),
                         EditModeSignalsSlotEditor, QLatin1String("signalslottool.png"), tr("F4"));

    m_toolActionIds.push_back(QLatin1String("FormEditor.BuddyEditor"));
    createEditModeAction(m_actionGroupEditMode, m_contexts, am, medit,
                         tr("Edit Buddies"), m_toolActionIds.back(),
                         EditModeBuddyEditor, QLatin1String("buddytool.png"));

    m_toolActionIds.push_back(QLatin1String("FormEditor.TabOrderEditor"));
    createEditModeAction(m_actionGroupEditMode, m_contexts, am, medit,
                         tr("Edit Tab Order"),  m_toolActionIds.back(),
                         EditModeTabOrderEditor, QLatin1String("tabordertool.png"));

    //tool actions
    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutHorizontally"));
    const QString horizLayoutShortcut = osMac ? tr("Meta+H") : tr("Ctrl+H");
    addToolAction(m_fwm->actionHorizontalLayout(), am, m_contexts,
                  m_toolActionIds.back(), mformtools, horizLayoutShortcut);

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutVertically"));
    const QString vertLayoutShortcut = osMac ? tr("Meta+L") : tr("Ctrl+L");
    addToolAction(m_fwm->actionVerticalLayout(), am, m_contexts,
                  m_toolActionIds.back(),  mformtools, vertLayoutShortcut);

    m_toolActionIds.push_back(QLatin1String("FormEditor.SplitHorizontal"));
    addToolAction(m_fwm->actionSplitHorizontal(), am, m_contexts,
                  m_toolActionIds.back(), mformtools);

    m_toolActionIds.push_back(QLatin1String("FormEditor.SplitVertical"));
    addToolAction(m_fwm->actionSplitVertical(), am, m_contexts,
                  m_toolActionIds.back(), mformtools);

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutForm"));
    addToolAction(m_fwm->actionFormLayout(), am, m_contexts,
                  m_toolActionIds.back(),  mformtools);

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutGrid"));
    const QString gridShortcut = osMac ? tr("Meta+G") : tr("Ctrl+G");
    addToolAction(m_fwm->actionGridLayout(), am, m_contexts,
                  m_toolActionIds.back(),  mformtools, gridShortcut);

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutBreak"));
    addToolAction(m_fwm->actionBreakLayout(), am, m_contexts,
                  m_toolActionIds.back(), mformtools);

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutAdjustSize"));
    const QString adjustShortcut = osMac ? tr("Meta+J") : tr("Ctrl+J");
    addToolAction(m_fwm->actionAdjustSize(), am, m_contexts,
                  m_toolActionIds.back(),  mformtools, adjustShortcut);

    m_toolActionIds.push_back(QLatin1String("FormEditor.SimplifyLayout"));
    addToolAction(m_fwm->actionSimplifyLayout(), am, m_contexts,
                  m_toolActionIds.back(),  mformtools);

    createSeparator(this, am, m_contexts, mformtools, QLatin1String("FormEditor.Menu.Tools.Separator1"));

    addToolAction(m_fwm->actionLower(), am, m_contexts,
                  QLatin1String("FormEditor.Lower"), mformtools);

    addToolAction(m_fwm->actionRaise(), am, m_contexts,
                  QLatin1String("FormEditor.Raise"), mformtools);

    // Commands that do not go into the editor toolbar
    createSeparator(this, am, m_contexts, mformtools, QLatin1String("FormEditor.Menu.Tools.Separator2"));

    m_actionPreview = m_fwm->actionDefaultPreview();
    QTC_ASSERT(m_actionPreview, return);
    addToolAction(m_actionPreview,  am,  m_contexts,
                   QLatin1String("FormEditor.Preview"), mformtools, tr("Alt+Shift+R"));

    // Preview in style...
    m_actionGroupPreviewInStyle = m_fwm->actionGroupPreviewInStyle();
    Core::ActionContainer *previewAC = createPreviewStyleMenu(am, m_actionGroupPreviewInStyle);
    m_previewInStyleMenu = previewAC->menu();
    mformtools->addMenu(previewAC);
    setPreviewMenuEnabled(false);

    // Form settings
    createSeparator(this, am, m_contexts,  medit, QLatin1String("FormEditor.Edit.Separator2"), Core::Constants::G_EDIT_OTHER);

    createSeparator(this, am, m_contexts, mformtools, QLatin1String("FormEditor.Menu.Tools.Separator3"));

    m_actionSwitchSource = new QAction(tr("Switch Source/Form"), this);
    connect(m_actionSwitchSource, SIGNAL(triggered()), this, SLOT(switchSourceForm()));

    // Switch form/source in editor/design contexts.
    Core::Context switchContexts = m_contexts;
    switchContexts.add(Core::Constants::C_EDITORMANAGER);
    addToolAction(m_actionSwitchSource, am, switchContexts, QLatin1String("FormEditor.FormSwitchSource"), mformtools,
                  tr("Shift+F4"));

    createSeparator(this, am, m_contexts, mformtools, QLatin1String("FormEditor.Menu.Tools.Separator4"));
    QAction *actionFormSettings = m_fwm->actionShowFormWindowSettingsDialog();
    addToolAction(actionFormSettings, am, m_contexts, QLatin1String("FormEditor.FormSettings"), mformtools);

    createSeparator(this, am, m_contexts, mformtools, QLatin1String("FormEditor.Menu.Tools.Separator5"));
    m_actionAboutPlugins = new QAction(tr("About Qt Designer plugins...."), this);
    addToolAction(m_actionAboutPlugins,  am,  m_contexts,
                   QLatin1String("FormEditor.AboutPlugins"), mformtools);
    connect(m_actionAboutPlugins,  SIGNAL(triggered()), m_fwm, SLOT(aboutPlugins()));
    m_actionAboutPlugins->setEnabled(false);

    // FWM
    connect(m_fwm, SIGNAL(activeFormWindowChanged(QDesignerFormWindowInterface *)), this, SLOT(activeFormWindowChanged(QDesignerFormWindowInterface *)));
}

QToolBar *FormEditorW::createEditorToolBar() const
{
    QToolBar *editorToolBar = new QToolBar;
    Core::ActionManager *am = m_core->actionManager();
    const QStringList::const_iterator cend = m_toolActionIds.constEnd();
    for (QStringList::const_iterator it = m_toolActionIds.constBegin(); it != cend; ++it) {
        Core::Command *cmd = am->command(*it);
        QTC_ASSERT(cmd, continue);
        QAction *action = cmd->action();
        if (!action->icon().isNull()) // Simplify grid has no action yet
            editorToolBar->addAction(action);
    }
    const int size = editorToolBar->style()->pixelMetric(QStyle::PM_SmallIconSize);
    editorToolBar->setIconSize(QSize(size, size));
    editorToolBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    return editorToolBar;
}

Core::ActionContainer *FormEditorW::createPreviewStyleMenu(Core::ActionManager *am,
                                                            QActionGroup *actionGroup)
{
    const QString menuId = QLatin1String(M_FORMEDITOR_PREVIEW);
    Core::ActionContainer *menuPreviewStyle = am->createMenu(menuId);
    menuPreviewStyle->menu()->setTitle(tr("Preview in"));

    // The preview menu is a list of invisible actions for the embedded design
    // device profiles (integer data) followed by a separator and the styles
    // (string data). Make device profiles update their text and hide them
    // in the configuration dialog.
    const QList<QAction*> actions = actionGroup->actions();

    const QString deviceProfilePrefix = QLatin1String("DeviceProfile");
    const QChar dot = QLatin1Char('.');

    foreach (QAction* a, actions) {
        QString name = menuId;
        name += dot;
        const QVariant data = a->data();
        const bool isDeviceProfile = data.type() == QVariant::Int;
        if (isDeviceProfile) {
            name += deviceProfilePrefix;
            name += dot;
        }
        name += data.toString();
        Core::Command *command = am->registerAction(a, name, m_contexts);
        bindShortcut(command, a);
        if (isDeviceProfile) {
            command->setAttribute(Core::Command::CA_UpdateText);
            command->setAttribute(Core::Command::CA_NonConfigureable);
        }
        menuPreviewStyle->addAction(command);
    }
    return menuPreviewStyle;
}

void FormEditorW::setPreviewMenuEnabled(bool e)
{
    m_actionPreview->setEnabled(e);
    m_previewInStyleMenu->setEnabled(e);
}

void FormEditorW::saveSettings(QSettings *s)
{
    s->beginGroup(settingsGroup);
    m_editorWidget->saveSettings(s);
    s->endGroup();
}

void FormEditorW::critical(const QString &errorMessage)
{
    QMessageBox::critical(m_core->mainWindow(), tr("Designer"),  errorMessage);
}

// Apply the command shortcut to the action and connects to the command's keySequenceChanged signal
void FormEditorW::bindShortcut(Core::Command *command, QAction *action)
{
    m_commandToDesignerAction.insert(command, action);
    connect(command, SIGNAL(keySequenceChanged()),
            m_shortcutMapper, SLOT(map()));
    m_shortcutMapper->setMapping(command, command);
    updateShortcut(command);
}

// Create an action to activate a designer tool
QAction *FormEditorW::createEditModeAction(QActionGroup *ag,
                                     const Core::Context &context,
                                     Core::ActionManager *am,
                                     Core::ActionContainer *medit,
                                     const QString &actionName,
                                     const QString &name,
                                     int toolNumber,
                                     const QString &iconName,
                                     const QString &keySequence)
{
    QAction *rc = new QAction(actionName, ag);
    rc->setCheckable(true);
    if (!iconName.isEmpty())
         rc->setIcon(designerIcon(iconName));
    Core::Command *command = am->registerAction(rc, name, context);
    command->setAttribute(Core::Command::CA_Hide);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    bindShortcut(command, rc);
    medit->addAction(command, Core::Constants::G_EDIT_OTHER);
    rc->setData(toolNumber);
    ag->addAction(rc);
    return rc;
}

// Create a tool action
Core::Command *FormEditorW::addToolAction(QAction *a, Core::ActionManager *am,
                                          const Core::Context &context, const QString &name,
                                          Core::ActionContainer *c1, const QString &keySequence)
{
    Core::Command *command = am->registerAction(a, name, context);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    if (!a->isSeparator())
        bindShortcut(command, a);
    c1->addAction(command);
    return command;
}

EditorData FormEditorW::createEditor(QWidget *parent)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorW::createEditor";
    // Create and associate form and text editor.
    EditorData data;
    m_fwm->closeAllPreviews();
    qdesigner_internal::FormWindowBase *form = qobject_cast<qdesigner_internal::FormWindowBase *>(m_fwm->createFormWindow(0));
    QTC_ASSERT(form, return data);
    connect(form, SIGNAL(toolChanged(int)), this, SLOT(toolChanged(int)));
    ResourceHandler *resourceHandler = new ResourceHandler(form);
    form->setDesignerGrid(qdesigner_internal::FormWindowBase::defaultDesignerGrid());
    qdesigner_internal::FormWindowBase::setupDefaultAction(form);
    data.widgetHost = new SharedTools::WidgetHost( /* parent */ 0, form);
    DesignerXmlEditor *xmlEditor = new DesignerXmlEditor(form, parent);
    TextEditor::TextEditorSettings::instance()->initializeEditor(xmlEditor);
    data.formWindowEditor = xmlEditor->designerEditor();
    connect(data.widgetHost, SIGNAL(formWindowSizeChanged(int,int)),
            xmlEditor, SIGNAL(changed()));
    connect(data.formWindowEditor->file(), SIGNAL(changed()),
            resourceHandler, SLOT(updateResources()));
    m_editorWidget->add(data);

    m_toolBar->addEditor(xmlEditor->editableInterface());

    return data;
}

void FormEditorW::updateShortcut(QObject *command)
{
    Core::Command *c = qobject_cast<Core::Command *>(command);
    if (!c)
        return;
    QAction *a = m_commandToDesignerAction.value(c);
    if (!a)
        return;
    a->setShortcut(c->action()->shortcut());
}

void FormEditorW::currentEditorChanged(Core::IEditor *editor)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << editor << " of " << m_fwm->formWindowCount();

    if (editor && editor->id() == QLatin1String(Constants::K_DESIGNER_XML_EDITOR_ID)) {
        FormWindowEditor *xmlEditor = qobject_cast<FormWindowEditor *>(editor);
        QTC_ASSERT(xmlEditor, return);
        ensureInitStage(FullyInitialized);
        SharedTools::WidgetHost *fw = m_editorWidget->formWindowEditorForXmlEditor(xmlEditor);
        QTC_ASSERT(fw, return)
        m_editorWidget->setVisibleEditor(xmlEditor);
        m_fwm->setActiveFormWindow(fw->formWindow());
    }
}

void FormEditorW::activeFormWindowChanged(QDesignerFormWindowInterface *afw)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << afw << " of " << m_fwm->formWindowCount();

    m_fwm->closeAllPreviews();
    setPreviewMenuEnabled(afw != 0);
}

EditorData FormEditorW::activeEditor() const
{
    if (m_editorWidget)
        return m_editorWidget->activeEditor();
    return EditorData();
}

void FormEditorW::activateEditMode(int id)
{
    if (const int count = m_fwm->formWindowCount())
        for (int i = 0; i <  count; i++)
             m_fwm->formWindow(i)->setCurrentTool(id);
}

void FormEditorW::activateEditMode(QAction* a)
{
    activateEditMode(a->data().toInt());
}

void FormEditorW::toolChanged(int t)
{
    typedef QList<QAction *> ActionList;
    if (const QAction *currentAction = m_actionGroupEditMode->checkedAction())
        if (currentAction->data().toInt() == t)
            return;
    const ActionList actions = m_actionGroupEditMode->actions();
    const ActionList::const_iterator cend = actions.constEnd();
    for (ActionList::const_iterator it = actions.constBegin(); it != cend; ++it)
        if ( (*it)->data().toInt() == t) {
            (*it)->setChecked(true);
            break;
        }
}

void FormEditorW::closeFormEditorsForXmlEditors(QList<Core::IEditor*> editors)
{
    foreach(Core::IEditor *editor, editors) {
        m_editorWidget->removeFormWindowEditor(editor);
    }
}

void FormEditorW::print()
{
    // Printing code courtesy of designer_actions.cpp
    QDesignerFormWindowInterface *fw = m_fwm->activeFormWindow();
    if (!fw)
        return;

    const bool oldFullPage =  m_core->printer()->fullPage();
    const QPrinter::Orientation oldOrientation =  m_core->printer()->orientation ();
    m_core->printer()->setFullPage(false);
    do {

        // Grab the image to be able to a suggest suitable orientation
        QString errorMessage;
        const QPixmap pixmap = m_fwm->createPreviewPixmap(&errorMessage);
        if (pixmap.isNull()) {
            critical(tr("The image could not be created: %1").arg(errorMessage));
            break;
        }

        const QSizeF pixmapSize = pixmap.size();
        m_core->printer()->setOrientation( pixmapSize.width() > pixmapSize.height() ?  QPrinter::Landscape :  QPrinter::Portrait);

        // Printer parameters
        QPrintDialog dialog(m_core->printer(), fw);
        if (!dialog.exec())
           break;

        const QCursor oldCursor = m_core->mainWindow()->cursor();
        m_core->mainWindow()->setCursor(Qt::WaitCursor);
        // Estimate of required scaling to make form look the same on screen and printer.
        const double suggestedScaling = static_cast<double>(m_core->printer()->physicalDpiX()) /  static_cast<double>(fw->physicalDpiX());

        QPainter painter(m_core->printer());
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        // Clamp to page
        const QRectF page =  painter.viewport();
        const double maxScaling = qMin(page.size().width() / pixmapSize.width(), page.size().height() / pixmapSize.height());
        const double scaling = qMin(suggestedScaling, maxScaling);

        const double xOffset = page.left() + qMax(0.0, (page.size().width()  - scaling * pixmapSize.width())  / 2.0);
        const double yOffset = page.top()  + qMax(0.0, (page.size().height() - scaling * pixmapSize.height()) / 2.0);

        // Draw.
        painter.translate(xOffset, yOffset);
        painter.scale(scaling, scaling);
        painter.drawPixmap(0, 0, pixmap);
        m_core->mainWindow()->setCursor(oldCursor);

    } while (false);
    m_core->printer()->setFullPage(oldFullPage);
    m_core->printer()->setOrientation(oldOrientation);
}

// Find out current existing editor file
static QString currentFile(const Core::EditorManager *em)
{
    if (Core::IEditor *editor = em->currentEditor())
        if (const Core::IFile *file = editor->file()) {
            const QString fileName = file->fileName();
            if (!fileName.isEmpty() && QFileInfo(fileName).isFile())
                return fileName;
        }
    return QString();
}

// Switch between form ('ui') and source file ('cpp'):
// Find corresponding 'other' file, simply assuming it is in the same directory.
static QString otherFile(const Core::EditorManager *em)
{
    // Determine mime type of current file.
    const QString current = currentFile(em);
    if (current.isEmpty())
        return QString();
    const Core::MimeDatabase *mdb = Core::ICore::instance()->mimeDatabase();
    const Core::MimeType currentMimeType = mdb->findByFile(current);
    if (!currentMimeType)
        return QString();
    // Determine potential suffixes of candidate files
    // 'ui' -> 'cpp', 'cpp/h' -> 'ui'.
    QStringList candidateSuffixes;
    if (currentMimeType.type() == QLatin1String(FORM_MIMETYPE)) {
        candidateSuffixes += mdb->findByType(QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)).suffixes();
    } else if (currentMimeType.type() == QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)
               || currentMimeType.type() == QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE)) {
        candidateSuffixes += mdb->findByType(QLatin1String(FORM_MIMETYPE)).suffixes();
    } else {
        return QString();
    }
    // Try to find existing file with desired suffix
    const QFileInfo currentFI(current);
    const QString currentBaseName = currentFI.path() + QLatin1Char('/')
            + currentFI.baseName() + QLatin1Char('.');
    foreach (const QString &candidateSuffix, candidateSuffixes) {
        const QFileInfo fi(currentBaseName + candidateSuffix);
        if (fi.isFile())
            return fi.absoluteFilePath();
    }
    return QString();
}

void FormEditorW::switchSourceForm()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    const QString fileToOpen = otherFile(em);
    if (!fileToOpen.isEmpty())
        em->openEditor(fileToOpen, QString(), Core::EditorManager::ModeSwitch);
}

} // namespace Internal
} // namespace Designer
