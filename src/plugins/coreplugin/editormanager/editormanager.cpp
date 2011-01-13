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

#include "editormanager.h"
#include "editorview.h"
#include "openeditorswindow.h"
#include "openeditorsview.h"
#include "openeditorsmodel.h"
#include "openwithdialog.h"
#include "filemanager.h"
#include "icore.h"
#include "ieditor.h"
#include "iversioncontrol.h"
#include "mimedatabase.h"
#include "tabpositionindicator.h"
#include "vcsmanager.h"

#include <coreplugin/editortoolbar.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/editormanager/iexternaleditor.h>
#include <coreplugin/icorelistener.h>
#include <coreplugin/imode.h>
#include <coreplugin/settingsdatabase.h>
#include <coreplugin/variablemanager.h>
#include <coreplugin/uniqueidmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/consoleprocess.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QMap>
#include <QtCore/QProcess>
#include <QtCore/QSet>
#include <QtCore/QSettings>
#include <QtCore/QTextCodec>

#include <QtGui/QAction>
#include <QtGui/QShortcut>
#include <QtGui/QApplication>
#include <QtGui/QFileDialog>
#include <QtGui/QLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QSplitter>
#include <QtGui/QStackedLayout>

enum { debugEditorManager=0 };

static inline ExtensionSystem::PluginManager *pluginManager()
{
    return ExtensionSystem::PluginManager::instance();
}

//===================EditorClosingCoreListener======================

namespace Core {
namespace Internal {

class EditorClosingCoreListener : public ICoreListener
{
public:
    EditorClosingCoreListener(EditorManager *em);
    bool editorAboutToClose(IEditor *editor);
    bool coreAboutToClose();

private:
    EditorManager *m_em;
};

EditorClosingCoreListener::EditorClosingCoreListener(EditorManager *em)
        : m_em(em)
{
}

bool EditorClosingCoreListener::editorAboutToClose(IEditor *)
{
    return true;
}

bool EditorClosingCoreListener::coreAboutToClose()
{
    // Do not ask for files to save.
    // MainWindow::closeEvent has already done that.
    return m_em->closeAllEditors(false);
}

} // namespace Internal
} // namespace Core

using namespace Core;
using namespace Core::Internal;
using namespace Utils;

//===================EditorManager=====================

EditorManagerPlaceHolder *EditorManagerPlaceHolder::m_current = 0;

EditorManagerPlaceHolder::EditorManagerPlaceHolder(Core::IMode *mode, QWidget *parent)
    : QWidget(parent), m_mode(mode)
{
    setLayout(new QVBoxLayout);
    layout()->setMargin(0);
    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode *)),
            this, SLOT(currentModeChanged(Core::IMode *)));

    currentModeChanged(Core::ModeManager::instance()->currentMode());
}

EditorManagerPlaceHolder::~EditorManagerPlaceHolder()
{
    if (m_current == this) {
        EditorManager::instance()->setParent(0);
        EditorManager::instance()->hide();
    }
}

void EditorManagerPlaceHolder::currentModeChanged(Core::IMode *mode)
{
    if (m_current == this) {
        m_current = 0;
        EditorManager::instance()->setParent(0);
        EditorManager::instance()->hide();
    }
    if (m_mode == mode) {
        m_current = this;
        layout()->addWidget(EditorManager::instance());
        EditorManager::instance()->show();
    }
}

EditorManagerPlaceHolder* EditorManagerPlaceHolder::current()
{
    return m_current;
}

// ---------------- EditorManager

namespace Core {


struct EditorManagerPrivate {
    explicit EditorManagerPrivate(ICore *core, QWidget *parent);
    ~EditorManagerPrivate();
    Internal::EditorView *m_view;
    Internal::SplitterOrView *m_splitter;
    QPointer<IEditor> m_currentEditor;
    QPointer<SplitterOrView> m_currentView;

    ICore *m_core;


    // actions
    QAction *m_revertToSavedAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_closeCurrentEditorAction;
    QAction *m_closeAllEditorsAction;
    QAction *m_closeOtherEditorsAction;
    QAction *m_gotoNextDocHistoryAction;
    QAction *m_gotoPreviousDocHistoryAction;
    QAction *m_goBackAction;
    QAction *m_goForwardAction;
    QAction *m_openInExternalEditorAction;
    QAction *m_splitAction;
    QAction *m_splitSideBySideAction;
    QAction *m_removeCurrentSplitAction;
    QAction *m_removeAllSplitsAction;
    QAction *m_gotoOtherSplitAction;

    Internal::OpenEditorsWindow *m_windowPopup;
    Internal::EditorClosingCoreListener *m_coreListener;

    QMap<QString, QVariant> m_editorStates;
    Internal::OpenEditorsViewFactory *m_openEditorsFactory;

    OpenEditorsModel *m_editorModel;
    QString m_externalEditor;

    IFile::ReloadSetting m_reloadSetting;
    IFile::Utf8BomSetting m_utf8BomSetting;

    QString m_titleAddition;
};
}

EditorManagerPrivate::EditorManagerPrivate(ICore *core, QWidget *parent) :
    m_view(0),
    m_splitter(0),
    m_core(core),
    m_revertToSavedAction(new QAction(EditorManager::tr("Revert to Saved"), parent)),
    m_saveAction(new QAction(parent)),
    m_saveAsAction(new QAction(parent)),
    m_closeCurrentEditorAction(new QAction(EditorManager::tr("Close"), parent)),
    m_closeAllEditorsAction(new QAction(EditorManager::tr("Close All"), parent)),
    m_closeOtherEditorsAction(new QAction(EditorManager::tr("Close Others"), parent)),
    m_gotoNextDocHistoryAction(new QAction(EditorManager::tr("Next Open Document in History"), parent)),
    m_gotoPreviousDocHistoryAction(new QAction(EditorManager::tr("Previous Open Document in History"), parent)),
    m_goBackAction(new QAction(QIcon(QLatin1String(Constants::ICON_PREV)), EditorManager::tr("Go Back"), parent)),
    m_goForwardAction(new QAction(QIcon(QLatin1String(Constants::ICON_NEXT)), EditorManager::tr("Go Forward"), parent)),
    m_openInExternalEditorAction(new QAction(EditorManager::tr("Open in External Editor"), parent)),
    m_windowPopup(0),
    m_coreListener(0),
    m_reloadSetting(IFile::AlwaysAsk),
    m_utf8BomSetting(IFile::OnlyKeep)
{
    m_editorModel = new OpenEditorsModel(parent);
}

EditorManagerPrivate::~EditorManagerPrivate()
{
//    clearNavigationHistory();
}

EditorManager *EditorManager::m_instance = 0;

static Command *createSeparator(ActionManager *am, QObject *parent,
                                const QString &name,
                                const Context &context)
{
    QAction *tmpaction = new QAction(parent);
    tmpaction->setSeparator(true);
    Command *cmd = am->registerAction(tmpaction, name, context);
    return cmd;
}

EditorManager::EditorManager(ICore *core, QWidget *parent) :
    QWidget(parent),
    m_d(new EditorManagerPrivate(core, parent))
{
    m_instance = this;

    connect(m_d->m_core, SIGNAL(contextAboutToChange(Core::IContext *)),
            this, SLOT(handleContextChange(Core::IContext *)));

    const Context editManagerContext(Constants::C_EDITORMANAGER);
    // combined context for edit & design modes
    const Context editDesignContext(Constants::C_EDITORMANAGER, Constants::C_DESIGN_MODE);

    ActionManager *am = m_d->m_core->actionManager();
    ActionContainer *mfile = am->actionContainer(Constants::M_FILE);

    // Revert to saved
    m_d->m_revertToSavedAction->setIcon(QIcon::fromTheme(QLatin1String("document-revert")));
    Command *cmd = am->registerAction(m_d->m_revertToSavedAction,
                                       Constants::REVERTTOSAVED, editManagerContext);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultText(tr("Revert File to Saved"));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);
    connect(m_d->m_revertToSavedAction, SIGNAL(triggered()), this, SLOT(revertToSaved()));

    // Save Action
    am->registerAction(m_d->m_saveAction, Constants::SAVE, editManagerContext);
    connect(m_d->m_saveAction, SIGNAL(triggered()), this, SLOT(saveFile()));

    // Save As Action
    am->registerAction(m_d->m_saveAsAction, Constants::SAVEAS, editManagerContext);
    connect(m_d->m_saveAsAction, SIGNAL(triggered()), this, SLOT(saveFileAs()));

    // Window Menu
    ActionContainer *mwindow = am->actionContainer(Constants::M_WINDOW);

    // Window menu separators
    QAction *tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    cmd = am->registerAction(tmpaction, "QtCreator.Window.Sep.Split", editManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    cmd = am->registerAction(tmpaction, "QtCreator.Window.Sep.Navigate", editManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);

    // Close Action
    cmd = am->registerAction(m_d->m_closeCurrentEditorAction, Constants::CLOSE, editManagerContext, true);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+W")));
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDefaultText(m_d->m_closeCurrentEditorAction->text());
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    connect(m_d->m_closeCurrentEditorAction, SIGNAL(triggered()), this, SLOT(closeEditor()));

#ifdef Q_WS_WIN
    // workaround for QTCREATORBUG-72
    QShortcut *sc = new QShortcut(parent);
    cmd = am->registerShortcut(sc, Constants::CLOSE_ALTERNATIVE, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+F4")));
    cmd->setDefaultText(EditorManager::tr("Close"));
    connect(sc, SIGNAL(activated()), this, SLOT(closeEditor()));
#endif

    // Close All Action
    cmd = am->registerAction(m_d->m_closeAllEditorsAction, Constants::CLOSEALL, editManagerContext, true);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+W")));
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    connect(m_d->m_closeAllEditorsAction, SIGNAL(triggered()), this, SLOT(closeAllEditors()));

    // Close All Others Action
    cmd = am->registerAction(m_d->m_closeOtherEditorsAction, Constants::CLOSEOTHERS, editManagerContext, true);
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    connect(m_d->m_closeOtherEditorsAction, SIGNAL(triggered()), this, SLOT(closeOtherEditors()));

    // Goto Previous In History Action
    cmd = am->registerAction(m_d->m_gotoPreviousDocHistoryAction, Constants::GOTOPREVINHISTORY, editDesignContext);
#ifdef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+Tab")));
#else
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Tab")));
#endif
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_d->m_gotoPreviousDocHistoryAction, SIGNAL(triggered()), this, SLOT(gotoPreviousDocHistory()));

    // Goto Next In History Action
    cmd = am->registerAction(m_d->m_gotoNextDocHistoryAction, Constants::GOTONEXTINHISTORY, editDesignContext);
#ifdef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+Shift+Tab")));
#else
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+Tab")));
#endif
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_d->m_gotoNextDocHistoryAction, SIGNAL(triggered()), this, SLOT(gotoNextDocHistory()));

    // Go back in navigation history
    cmd = am->registerAction(m_d->m_goBackAction, Constants::GO_BACK, editDesignContext);
#ifdef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Left")));
#else
    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+Left")));
#endif
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_d->m_goBackAction, SIGNAL(triggered()), this, SLOT(goBackInNavigationHistory()));

    // Go forward in navigation history
    cmd = am->registerAction(m_d->m_goForwardAction, Constants::GO_FORWARD, editDesignContext);
#ifdef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Right")));
#else
    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+Right")));
#endif
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_d->m_goForwardAction, SIGNAL(triggered()), this, SLOT(goForwardInNavigationHistory()));

#ifdef Q_WS_MAC
    QString prefix = tr("Meta+E");
#else
    QString prefix = tr("Ctrl+E");
#endif

    m_d->m_splitAction = new QAction(tr("Split"), this);
    cmd = am->registerAction(m_d->m_splitAction, Constants::SPLIT, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("%1,2").arg(prefix)));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_d->m_splitAction, SIGNAL(triggered()), this, SLOT(split()));

    m_d->m_splitSideBySideAction = new QAction(tr("Split Side by Side"), this);
    cmd = am->registerAction(m_d->m_splitSideBySideAction, Constants::SPLIT_SIDE_BY_SIDE, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("%1,3").arg(prefix)));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_d->m_splitSideBySideAction, SIGNAL(triggered()), this, SLOT(splitSideBySide()));

    m_d->m_removeCurrentSplitAction = new QAction(tr("Remove Current Split"), this);
    cmd = am->registerAction(m_d->m_removeCurrentSplitAction, Constants::REMOVE_CURRENT_SPLIT, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("%1,0").arg(prefix)));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_d->m_removeCurrentSplitAction, SIGNAL(triggered()), this, SLOT(removeCurrentSplit()));

    m_d->m_removeAllSplitsAction = new QAction(tr("Remove All Splits"), this);
    cmd = am->registerAction(m_d->m_removeAllSplitsAction, Constants::REMOVE_ALL_SPLITS, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("%1,1").arg(prefix)));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_d->m_removeAllSplitsAction, SIGNAL(triggered()), this, SLOT(removeAllSplits()));

    m_d->m_gotoOtherSplitAction = new QAction(tr("Go to Next Split"), this);
    cmd = am->registerAction(m_d->m_gotoOtherSplitAction, Constants::GOTO_OTHER_SPLIT, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("%1,o").arg(prefix)));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_d->m_gotoOtherSplitAction, SIGNAL(triggered()), this, SLOT(gotoOtherSplit()));

    ActionContainer *medit = am->actionContainer(Constants::M_EDIT);
    ActionContainer *advancedMenu = am->createMenu(Constants::M_EDIT_ADVANCED);
    medit->addMenu(advancedMenu, Constants::G_EDIT_ADVANCED);
    advancedMenu->menu()->setTitle(tr("&Advanced"));
    advancedMenu->appendGroup(Constants::G_EDIT_FORMAT);
    advancedMenu->appendGroup(Constants::G_EDIT_COLLAPSING);
    advancedMenu->appendGroup(Constants::G_EDIT_BLOCKS);
    advancedMenu->appendGroup(Constants::G_EDIT_FONT);
    advancedMenu->appendGroup(Constants::G_EDIT_EDITOR);

    // Advanced menu separators
    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.Collapsing"), editManagerContext);
    advancedMenu->addAction(cmd, Constants::G_EDIT_COLLAPSING);
    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.Blocks"), editManagerContext);
    advancedMenu->addAction(cmd, Constants::G_EDIT_BLOCKS);
    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.Font"), editManagerContext);
    advancedMenu->addAction(cmd, Constants::G_EDIT_FONT);
    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.Editor"), editManagerContext);
    advancedMenu->addAction(cmd, Constants::G_EDIT_EDITOR);

    cmd = am->registerAction(m_d->m_openInExternalEditorAction, Constants::OPEN_IN_EXTERNAL_EDITOR, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+V,Alt+I")));
    advancedMenu->addAction(cmd, Constants::G_EDIT_EDITOR);
    connect(m_d->m_openInExternalEditorAction, SIGNAL(triggered()), this, SLOT(openInExternalEditor()));

    // Connect to VariableManager for CURRENT_DOCUMENT variable setting
    VariableManager::initEditorManagerConnections();
    // other setup
    m_d->m_splitter = new SplitterOrView(m_d->m_editorModel);
    m_d->m_view = m_d->m_splitter->view();


    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_d->m_splitter);

    updateActions();

    m_d->m_windowPopup = new OpenEditorsWindow(this);
}

EditorManager::~EditorManager()
{
    m_instance = 0;
    if (m_d->m_core) {
        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
        if (m_d->m_coreListener) {
            pm->removeObject(m_d->m_coreListener);
            delete m_d->m_coreListener;
        }
        pm->removeObject(m_d->m_openEditorsFactory);
        delete m_d->m_openEditorsFactory;
    }
    delete m_d;
}

void EditorManager::init()
{
    m_d->m_coreListener = new EditorClosingCoreListener(this);
    pluginManager()->addObject(m_d->m_coreListener);

    m_d->m_openEditorsFactory = new OpenEditorsViewFactory();
    pluginManager()->addObject(m_d->m_openEditorsFactory);
}


EditorToolBar *EditorManager::createToolBar(QWidget *parent)
{
    return new EditorToolBar(parent);
}

QString EditorManager::defaultExternalEditor() const
{
#ifdef Q_OS_UNIX
    return ConsoleProcess::defaultTerminalEmulator() + QLatin1String(
# ifdef Q_OS_MAC
            " -async"
# endif
            " -geom %Wx%H+%x+%y -e vi %f +%l +\"normal %c|\"");
#else
    return QLatin1String("notepad %f");
#endif
}

void EditorManager::removeEditor(IEditor *editor)
{
    bool isDuplicate = m_d->m_editorModel->isDuplicate(editor);
    m_d->m_editorModel->removeEditor(editor);
    if (!isDuplicate) {
        m_d->m_core->fileManager()->removeFile(editor->file());
    }
    m_d->m_core->removeContextObject(editor);
}

void EditorManager::handleContextChange(Core::IContext *context)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO;
    IEditor *editor = context ? qobject_cast<IEditor*>(context) : 0;
    if (editor) {
        setCurrentEditor(editor);
    } else {
        updateActions();
    }
}

void EditorManager::setCurrentEditor(IEditor *editor, bool ignoreNavigationHistory)
{
    if (editor)
        setCurrentView(0);

    if (m_d->m_currentEditor == editor)
        return;
    if (m_d->m_currentEditor && !ignoreNavigationHistory)
        addCurrentPositionToNavigationHistory();

    m_d->m_currentEditor = editor;
    if (editor) {
        if (SplitterOrView *splitterOrView = m_d->m_splitter->findView(editor))
            splitterOrView->view()->setCurrentEditor(editor);
        m_d->m_view->updateEditorHistory(editor); // the global view should have a complete history
    }
    updateActions();
    updateWindowTitle();
    emit currentEditorChanged(editor);
}


void EditorManager::setCurrentView(Core::Internal::SplitterOrView *view)
{
    if (view == m_d->m_currentView)
        return;

    SplitterOrView *old = m_d->m_currentView;
    m_d->m_currentView = view;

    if (old)
        old->update();
    if (view)
        view->update();

    if (view && !view->editor())
        view->setFocus();
}

Core::Internal::SplitterOrView *EditorManager::currentSplitterOrView() const
{
    SplitterOrView *view = m_d->m_currentView;
    if (!view)
        view = m_d->m_currentEditor?
               m_d->m_splitter->findView(m_d->m_currentEditor):
               m_d->m_splitter->findFirstView();
    if (!view)
        return m_d->m_splitter;
    return view;
}

Core::Internal::EditorView *EditorManager::currentEditorView() const
{
    return currentSplitterOrView()->view();
}

QList<IEditor *> EditorManager::editorsForFileName(const QString &filename) const
{
    QList<IEditor *> found;
    QString fixedname = FileManager::fixFileName(filename, FileManager::KeepLinks);
    foreach (IEditor *editor, openedEditors()) {
        if (fixedname == FileManager::fixFileName(editor->file()->fileName(), FileManager::KeepLinks))
            found << editor;
    }
    return found;
}

QList<IEditor *> EditorManager::editorsForFile(IFile *file) const
{
    QList<IEditor *> found;
    foreach (IEditor *editor, openedEditors()) {
        if (editor->file() == file)
            found << editor;
    }
    return found;
}

IEditor *EditorManager::currentEditor() const
{
    return m_d->m_currentEditor;
}

void EditorManager::emptyView(Core::Internal::EditorView *view)
{
    if (!view)
        return;

    QList<IEditor *> editors = view->editors();
    foreach (IEditor *editor, editors) {
        if (!m_d->m_editorModel->isDuplicate(editor)) {
            editors.removeAll(editor);
            view->removeEditor(editor);
            continue;
        }
        emit editorAboutToClose(editor);
        removeEditor(editor);
        view->removeEditor(editor);
    }
    emit editorsClosed(editors);
    foreach (IEditor *editor, editors) {
        delete editor;
    }
}

void EditorManager::closeView(Core::Internal::EditorView *view)
{
    if (!view)
        return;

    if (view == m_d->m_view) {
        if (IEditor *e = view->currentEditor())
            closeEditors(QList<IEditor *>() << e);
        return;
    }

    if (IEditor *e = view->currentEditor()) {
        /*
           when we are closing a view with an original editor which has
           duplicates, then make one of the duplicates the original.
           Otherwise the original would be kept around and the user might
           experience jumping to a missleading position within the file when
           visiting the file again. With the code below, the position within
           the file will be the position of the first duplicate which is still
           around.
        */
        if (!m_d->m_editorModel->isDuplicate(e)) {
            QList<IEditor *> duplicates = m_d->m_editorModel->duplicatesFor(e);
            if (!duplicates.isEmpty()) {
                m_d->m_editorModel->makeOriginal(duplicates.first());
            }
        }
    }

    emptyView(view);

    SplitterOrView *splitterOrView = m_d->m_splitter->findView(view);
    Q_ASSERT(splitterOrView);
    Q_ASSERT(splitterOrView->view() == view);
    SplitterOrView *splitter = m_d->m_splitter->findSplitter(splitterOrView);
    Q_ASSERT(splitterOrView->hasEditors() == false);
    splitterOrView->hide();
    delete splitterOrView;

    splitter->unsplit();

    SplitterOrView *newCurrent = splitter->findFirstView();
    if (newCurrent) {
        if (IEditor *e = newCurrent->editor()) {
            activateEditor(newCurrent->view(), e);
        } else {
            setCurrentView(newCurrent);
        }
    }
}

QList<IEditor*>
    EditorManager::editorsForFiles(QList<IFile*> files) const
{
    const QList<IEditor *> editors = openedEditors();
    QSet<IEditor *> found;
    foreach (IFile *file, files) {
        foreach (IEditor *editor, editors) {
            if (editor->file() == file && !found.contains(editor)) {
                    found << editor;
            }
        }
    }
    return found.toList();
}

QList<IFile *> EditorManager::filesForEditors(QList<IEditor *> editors) const
{
    QSet<IEditor *> handledEditors;
    QList<IFile *> files;
    foreach (IEditor *editor, editors) {
        if (!handledEditors.contains(editor)) {
            files << editor->file();
            handledEditors.insert(editor);
        }
    }
    return files;
}

bool EditorManager::closeAllEditors(bool askAboutModifiedEditors)
{
    m_d->m_editorModel->removeAllRestoredEditors();
    if (closeEditors(openedEditors(), askAboutModifiedEditors)) {
//        m_d->clearNavigationHistory();
        return true;
    }
    return false;
}

void EditorManager::closeOtherEditors(IEditor *editor)
{
    m_d->m_editorModel->removeAllRestoredEditors();
    QList<IEditor*> editors = openedEditors();
    editors.removeAll(editor);
    closeEditors(editors, true);
}

void EditorManager::closeOtherEditors()
{
    IEditor *current = currentEditor();
    QTC_ASSERT(current, return);
    closeOtherEditors(current);
}

// SLOT connected to action
void EditorManager::closeEditor()
{
    if (!m_d->m_currentEditor)
        return;
    addCurrentPositionToNavigationHistory();
    closeEditor(m_d->m_currentEditor);
}

void EditorManager::closeEditor(Core::IEditor *editor)
{
    if (!editor)
        return;
    closeEditors(QList<IEditor *>() << editor);
}

void EditorManager::closeEditor(const QModelIndex &index)
{
    IEditor *editor = index.data(Qt::UserRole).value<Core::IEditor*>();
    if (editor)
        closeEditor(editor);
    else
        m_d->m_editorModel->removeEditor(index);
}

bool EditorManager::closeEditors(const QList<IEditor*> &editorsToClose, bool askAboutModifiedEditors)
{
    if (editorsToClose.isEmpty())
        return true;

    SplitterOrView *currentSplitterOrView = this->currentSplitterOrView();

    bool closingFailed = false;
    QList<IEditor*> acceptedEditors;
    //ask all core listeners to check whether the editor can be closed
    const QList<ICoreListener *> listeners =
        pluginManager()->getObjects<ICoreListener>();
    foreach (IEditor *editor, editorsToClose) {
        bool editorAccepted = true;
        if (m_d->m_editorModel->isDuplicate(editor))
            editor = m_d->m_editorModel->originalForDuplicate(editor);
        foreach (ICoreListener *listener, listeners) {
            if (!listener->editorAboutToClose(editor)) {
                editorAccepted = false;
                closingFailed = true;
                break;
            }
        }
        if (editorAccepted)
            acceptedEditors.append(editor);
    }
    if (acceptedEditors.isEmpty())
        return false;
    //ask whether to save modified files
    if (askAboutModifiedEditors) {
        bool cancelled = false;
        QList<IFile*> list = m_d->m_core->fileManager()->
            saveModifiedFiles(filesForEditors(acceptedEditors), &cancelled);
        if (cancelled)
            return false;
        if (!list.isEmpty()) {
            closingFailed = true;
            QSet<IEditor*> skipSet = editorsForFiles(list).toSet();
            acceptedEditors = acceptedEditors.toSet().subtract(skipSet).toList();
        }
    }
    if (acceptedEditors.isEmpty())
        return false;

    // add duplicates
    foreach(IEditor *editor, acceptedEditors)
        acceptedEditors += m_d->m_editorModel->duplicatesFor(editor);

    QList<EditorView*> closedViews;

    // remove the editors
    foreach (IEditor *editor, acceptedEditors) {
        emit editorAboutToClose(editor);
        if (!editor->file()->fileName().isEmpty()
                && !editor->isTemporary()) {
            QByteArray state = editor->saveState();
            if (!state.isEmpty())
                m_d->m_editorStates.insert(editor->file()->fileName(), QVariant(state));
        }

        removeEditor(editor);
        if (SplitterOrView *view = m_d->m_splitter->findView(editor)) {
            if (editor == view->view()->currentEditor())
                closedViews += view->view();
            view->view()->removeEditor(editor);
        }
    }

    foreach (EditorView *view, closedViews) {
        IEditor *newCurrent = view->currentEditor();
        if (!newCurrent)
            newCurrent = pickUnusedEditor();
        if (newCurrent) {
            activateEditor(view, newCurrent, NoActivate);
        } else {
            QModelIndex idx = m_d->m_editorModel->firstRestoredEditor();
            if (idx.isValid())
                activateEditorForIndex(view, idx, NoActivate);
        }
    }

    emit editorsClosed(acceptedEditors);

    foreach (IEditor *editor, acceptedEditors) {
        delete editor;
    }

    if (currentSplitterOrView) {
        if (IEditor *editor = currentSplitterOrView->editor())
            activateEditor(currentSplitterOrView->view(), editor);
    }

    if (!currentEditor()) {
        emit currentEditorChanged(0);
        updateActions();
        updateWindowTitle();
    }

    return !closingFailed;
}

void EditorManager::closeDuplicate(Core::IEditor *editor)
{

    IEditor *original = editor;
    if (m_d->m_editorModel->isDuplicate(editor))
        original= m_d->m_editorModel->originalForDuplicate(editor);
    QList<IEditor *> duplicates = m_d->m_editorModel->duplicatesFor(original);

    if (duplicates.isEmpty()) {
        closeEditor(editor);
        return;
    }

    if (original== editor)
        m_d->m_editorModel->makeOriginal(duplicates.first());

    SplitterOrView *currentSplitterOrView = this->currentSplitterOrView();

    emit editorAboutToClose(editor);

    if(m_d->m_splitter->findView(editor)) {
        EditorView *view = m_d->m_splitter->findView(editor)->view();
        removeEditor(editor);
        view->removeEditor(editor);

        IEditor *newCurrent = view->currentEditor();
        if (!newCurrent)
            newCurrent = pickUnusedEditor();
        if (newCurrent) {
            activateEditor(view, newCurrent, NoActivate);
        } else {
            QModelIndex idx = m_d->m_editorModel->firstRestoredEditor();
            if (idx.isValid())
                activateEditorForIndex(view, idx, NoActivate);
        }
    }

    emit editorsClosed(QList<IEditor*>() << editor);
    delete editor;
    if (currentSplitterOrView) {
        if (IEditor *currentEditor = currentSplitterOrView->editor())
            activateEditor(currentSplitterOrView->view(), currentEditor);
    }
}

Core::IEditor *EditorManager::pickUnusedEditor() const
{
    foreach (IEditor *editor, openedEditors()) {
        SplitterOrView *view = m_d->m_splitter->findView(editor);
        if (!view || view->editor() != editor)
            return editor;
    }
    return 0;
}

void EditorManager::activateEditorForIndex(const QModelIndex &index, OpenEditorFlags flags)
{
    activateEditorForIndex(currentEditorView(), index, flags);
}

void EditorManager::activateEditorForIndex(Internal::EditorView *view, const QModelIndex &index, OpenEditorFlags flags)
{
    Q_ASSERT(view);
    IEditor *editor = index.data(Qt::UserRole).value<IEditor*>();
    if (editor)  {
        activateEditor(view, editor, flags);
        return;
    }

    QString fileName = index.data(Qt::UserRole + 1).toString();
    QString id = index.data(Qt::UserRole + 2).toString();
    openEditor(view, fileName, id, flags);
}

Core::IEditor *EditorManager::placeEditor(Core::Internal::EditorView *view, Core::IEditor *editor)
{
    Q_ASSERT(view && editor);

    if (view->currentEditor() && view->currentEditor()->file() == editor->file())
        editor = view->currentEditor();

    if (!view->hasEditor(editor)) {
        bool duplicateSupported = editor->duplicateSupported();
        if (SplitterOrView *sourceView = m_d->m_splitter->findView(editor)) {
            if (editor != sourceView->editor() || !duplicateSupported) {
                sourceView->view()->removeEditor(editor);
                view->addEditor(editor);
                view->setCurrentEditor(editor);
                if (!sourceView->editor()) {
                    if (IEditor *replacement = pickUnusedEditor()) {
                        sourceView->view()->addEditor(replacement);
                    }
                }
                return editor;
            } else if (duplicateSupported) {
                editor = duplicateEditor(editor);
                Q_ASSERT(editor);
                m_d->m_editorModel->makeOriginal(editor);
            }
        }
        view->addEditor(editor);
    }
    return editor;
}

void EditorManager::activateEditor(Core::IEditor *editor, OpenEditorFlags flags)
{
    SplitterOrView *splitterOrView = m_d->m_splitter->findView(editor);
    EditorView *view = (splitterOrView ? splitterOrView->view() : 0);
    // TODO an IEditor doesn't have to belong to a view, which makes this method a bit funny
    if (!view)
        view = currentEditorView();
    activateEditor(view, editor, flags);
}

Core::IEditor *EditorManager::activateEditor(Core::Internal::EditorView *view, Core::IEditor *editor, OpenEditorFlags flags)
{
    Q_ASSERT(view);

    if (!editor) {
        if (!m_d->m_currentEditor)
            setCurrentEditor(0, (flags & IgnoreNavigationHistory));
        return 0;
    }

    editor = placeEditor(view, editor);

    if (!(flags & NoActivate)) {
        setCurrentEditor(editor, (flags & IgnoreNavigationHistory));
        if (flags & ModeSwitch) {
            switchToPreferedMode();
        }
        if (isVisible())
            editor->widget()->setFocus();
    }
    return editor;
}

Core::IEditor *EditorManager::activateEditorForFile(Core::Internal::EditorView *view, Core::IFile *file, OpenEditorFlags flags)
{
    Q_ASSERT(view);
    const QList<IEditor*> editors = editorsForFile(file);
    if (editors.isEmpty())
        return 0;

    activateEditor(view, editors.first(), flags);
    return editors.first();
}

/* For something that has a 'QStringList mimeTypes' (IEditorFactory
 * or IExternalEditor), find the one best matching the mimetype passed in.
 *  Recurse over the parent classes of the mimetype to find them. */
template <class EditorFactoryLike>
static void mimeTypeFactoryRecursion(const MimeDatabase *db,
                                     const MimeType &mimeType,
                                     const QList<EditorFactoryLike*> &allFactories,
                                     bool firstMatchOnly,
                                     QList<EditorFactoryLike*> *list)
{
    typedef typename QList<EditorFactoryLike*>::const_iterator EditorFactoryLikeListConstIterator;
    // Loop factories to find type
    const QString type = mimeType.type();
    const EditorFactoryLikeListConstIterator fcend = allFactories.constEnd();
    for (EditorFactoryLikeListConstIterator fit = allFactories.constBegin(); fit != fcend; ++fit) {
        // Exclude duplicates when recursing over xml or C++ -> C -> text.
        EditorFactoryLike *factory = *fit;
        if (!list->contains(factory) && factory->mimeTypes().contains(type)) {
            list->push_back(*fit);
            if (firstMatchOnly)
                return;
            break;
        }
    }
    // Any parent mime type classes? -> recurse
    QStringList parentTypes = mimeType.subClassesOf();
    if (parentTypes.empty())
        return;
    const QStringList::const_iterator pcend = parentTypes .constEnd();
    for (QStringList::const_iterator pit = parentTypes .constBegin(); pit != pcend; ++pit) {
        if (const MimeType parent = db->findByType(*pit))
            mimeTypeFactoryRecursion(db, parent, allFactories, firstMatchOnly, list);
    }
}

EditorManager::EditorFactoryList
    EditorManager::editorFactories(const MimeType &mimeType, bool bestMatchOnly) const
{
    EditorFactoryList rc;
    const EditorFactoryList allFactories = pluginManager()->getObjects<IEditorFactory>();
    mimeTypeFactoryRecursion(m_d->m_core->mimeDatabase(), mimeType, allFactories, bestMatchOnly, &rc);
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << mimeType.type() << " returns " << rc;
    return rc;
}

EditorManager::ExternalEditorList
        EditorManager::externalEditors(const MimeType &mimeType, bool bestMatchOnly) const
{
    ExternalEditorList rc;
    const ExternalEditorList allEditors = pluginManager()->getObjects<IExternalEditor>();
    mimeTypeFactoryRecursion(m_d->m_core->mimeDatabase(), mimeType, allEditors, bestMatchOnly, &rc);
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << mimeType.type() << " returns " << rc;
    return rc;
}

/* For something that has a 'QString id' (IEditorFactory
 * or IExternalEditor), find the one matching a id. */
template <class EditorFactoryLike>
        inline EditorFactoryLike *findById(ExtensionSystem::PluginManager *pm,
                                             const QString &id)
{
    const QList<EditorFactoryLike *> factories = pm->template getObjects<EditorFactoryLike>();
    foreach(EditorFactoryLike *efl, factories)
        if (id == efl->id())
            return efl;
    return 0;
}

IEditor *EditorManager::createEditor(const QString &editorId,
                                     const QString &fileName)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << editorId << fileName;

    EditorFactoryList factories;
    if (editorId.isEmpty()) {
        const QFileInfo fileInfo(fileName);
        // Find by mime type
        MimeType mimeType = m_d->m_core->mimeDatabase()->findByFile(fileInfo);
        if (!mimeType) {
            qWarning("%s unable to determine mime type of %s/%s. Falling back to text/plain",
                     Q_FUNC_INFO, fileName.toUtf8().constData(), editorId.toUtf8().constData());
            mimeType = m_d->m_core->mimeDatabase()->findByType(QLatin1String("text/plain"));
        }
        // open text files > 48 MB in binary editor
        if (fileInfo.size() >  maxTextFileSize() && mimeType.type().startsWith(QLatin1String("text")))
            mimeType = m_d->m_core->mimeDatabase()->findByType(QLatin1String("application/octet-stream"));
        factories = editorFactories(mimeType, true);
    } else {
        // Find by editor id
        if (IEditorFactory *factory = findById<IEditorFactory>(pluginManager(), editorId))
            factories.push_back(factory);
    }
    if (factories.empty()) {
        qWarning("%s: unable to find an editor factory for the file '%s', editor Id '%s'.",
                 Q_FUNC_INFO, fileName.toUtf8().constData(), editorId.toUtf8().constData());
        return 0;
    }

    IEditor *editor = factories.front()->createEditor(this);
    if (editor)
        connect(editor, SIGNAL(changed()), this, SLOT(handleEditorStateChange()));
    if (editor)
        emit editorCreated(editor, fileName);
    return editor;
}

void EditorManager::addEditor(IEditor *editor, bool isDuplicate)
{
    if (!editor)
        return;
    m_d->m_core->addContextObject(editor);

    m_d->m_editorModel->addEditor(editor, isDuplicate);
    if (!isDuplicate) {
        const bool isTemporary = editor->isTemporary();
        const bool addWatcher = !isTemporary;
        m_d->m_core->fileManager()->addFile(editor->file(), addWatcher);
        if (!isTemporary)
            m_d->m_core->fileManager()->addToRecentFiles(editor->file()->fileName(),
                                                         editor->id());
    }
    emit editorOpened(editor);
}

// Run the OpenWithDialog and return the editor id
// selected by the user.
QString EditorManager::getOpenWithEditorId(const QString &fileName,
                                           bool *isExternalEditor) const
{
    // Collect editors that can open the file
    const MimeType mt = m_d->m_core->mimeDatabase()->findByFile(fileName);
    if (!mt)
        return QString();
    QStringList allEditorIds;
    QStringList externalEditorIds;
    // Built-in
    const EditorFactoryList editors = editorFactories(mt, false);
    const int size = editors.size();
    for (int i = 0; i < size; i++) {
        allEditorIds.push_back(editors.at(i)->id());
    }
    // External editors
    const ExternalEditorList exEditors = externalEditors(mt, false);
    const int esize = exEditors.size();
    for (int i = 0; i < esize; i++) {
        externalEditorIds.push_back(exEditors.at(i)->id());
        allEditorIds.push_back(exEditors.at(i)->id());
    }
    if (allEditorIds.empty())
        return QString();
    // Run dialog.
    OpenWithDialog dialog(fileName, m_d->m_core->mainWindow());
    dialog.setEditors(allEditorIds);
    dialog.setCurrentEditor(0);
    if (dialog.exec() != QDialog::Accepted)
        return QString();
    const QString selectedId = dialog.editor();
    if (isExternalEditor)
        *isExternalEditor = externalEditorIds.contains(selectedId);
    return selectedId;
}

IEditor *EditorManager::openEditor(const QString &fileName, const QString &editorId,
                                   OpenEditorFlags flags, bool *newEditor)
{
    return openEditor(currentEditorView(), fileName, editorId, flags, newEditor);
}

int extractLineNumber(QString *fileName)
{
    int i = fileName->length() - 1;
    for (; i >= 0; --i) {
        if (!fileName->at(i).isNumber())
            break;
    }
    if (i == -1)
        return -1;
    if (fileName->at(i) == ':' || fileName->at(i) == '+') {
        int result = fileName->mid(i+1).toInt();
        if (result) {
            *fileName = fileName->left(i);
            return result;
        }
    }
    return -1;
}

IEditor *EditorManager::openEditor(Core::Internal::EditorView *view, const QString &fileName,
                        const QString &editorId, OpenEditorFlags flags, bool *newEditor)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << fileName << editorId;

    QString fn = fileName;
    int lineNumber = -1;
    if (flags && EditorManager::CanContainLineNumber)
        lineNumber = extractLineNumber(&fn);

    if (fn.isEmpty())
        return 0;

    if (newEditor)
        *newEditor = false;

    const QList<IEditor *> editors = editorsForFileName(fn);
    if (!editors.isEmpty()) {
        IEditor *editor = editors.first();
        if (flags && EditorManager::CanContainLineNumber)
            editor->gotoLine(lineNumber, -1);
        return activateEditor(view, editor, flags);
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    IEditor *editor = createEditor(editorId, fn);
    // If we could not open the file in the requested editor, fall
    // back to the default editor:
    if (!editor)
        editor = createEditor(QString(), fn);
    if (!editor || !editor->open(fn)) {
        QApplication::restoreOverrideCursor();
        QMessageBox::critical(m_d->m_core->mainWindow(), tr("Opening File"), tr("Cannot open file %1!").arg(QDir::toNativeSeparators(fn)));
        delete editor;
        editor = 0;
        return 0;
    }
    addEditor(editor);

    if (newEditor)
        *newEditor = true;

    IEditor *result = activateEditor(view, editor, flags);
    if (editor == result)
        restoreEditorState(editor);

    if (flags && EditorManager::CanContainLineNumber)
        editor->gotoLine(lineNumber, -1);

    QApplication::restoreOverrideCursor();
    return result;
}

bool EditorManager::openExternalEditor(const QString &fileName, const QString &editorId)
{
    IExternalEditor *ee = findById<IExternalEditor>(pluginManager(), editorId);
    if (!ee)
        return false;
    QString errorMessage;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    const bool ok = ee->startEditor(fileName, &errorMessage);
    QApplication::restoreOverrideCursor();
    if (!ok)
        QMessageBox::critical(m_d->m_core->mainWindow(), tr("Opening File"), errorMessage);
    return ok;
}

QStringList EditorManager::getOpenFileNames() const
{
    QString selectedFilter;
    const QString &fileFilters = m_d->m_core->mimeDatabase()->allFiltersString(&selectedFilter);
    return ICore::instance()->fileManager()->getOpenFileNames(fileFilters,
                                                              QString(), &selectedFilter);
}


/// Empty mode == figure out the correct mode from the editor
/// forcePrefered = true, switch to the mode even if the editor is visible in another mode
/// forcePrefered = false, only switch if it is not visible
void EditorManager::switchToPreferedMode()
{
    QString preferedMode;
    // Figure out preferred mode for editor
    if (m_d->m_currentEditor)
        preferedMode = m_d->m_currentEditor->preferredModeType();

    if (preferedMode.isEmpty())
        preferedMode = Constants::MODE_EDIT_TYPE;

    m_d->m_core->modeManager()->activateModeType(preferedMode);
}

IEditor *EditorManager::openEditorWithContents(const QString &editorId,
                                        QString *titlePattern,
                                        const QString &contents)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << editorId << titlePattern << contents;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QString title;
    if (titlePattern) {
        const QChar dollar = QLatin1Char('$');

        QString base = *titlePattern;
        if (base.isEmpty())
            base = QLatin1String("unnamed$");
        if (base.contains(dollar)) {
            int i = 1;
            QSet<QString> docnames;
            foreach (IEditor *editor, openedEditors()) {
                QString name = editor->file()->fileName();
                if (name.isEmpty()) {
                    name = editor->displayName();
                } else {
                    name = QFileInfo(name).completeBaseName();
                }
                docnames << name;
            }

            do {
                title = base;
                title.replace(QString(dollar), QString::number(i++));
            } while (docnames.contains(title));
        } else {
            title = *titlePattern;
        }
        *titlePattern = title;
    }

    IEditor *edt = createEditor(editorId, title);
    if (!edt) {
        QApplication::restoreOverrideCursor();
        return 0;
    }

    if (!edt->createNew(contents)) {
        QApplication::restoreOverrideCursor();
        delete edt;
        edt = 0;
        return 0;
    }

    if (title.isEmpty())
        title = edt->displayName();

    edt->setDisplayName(title);
    addEditor(edt);
    QApplication::restoreOverrideCursor();
    return edt;
}

bool EditorManager::hasEditor(const QString &fileName) const
{
    return !editorsForFileName(fileName).isEmpty();
}

void EditorManager::restoreEditorState(IEditor *editor)
{
    QTC_ASSERT(editor, return);
    QString fileName = editor->file()->fileName();
    editor->restoreState(m_d->m_editorStates.value(fileName).toByteArray());
}

bool EditorManager::saveEditor(IEditor *editor)
{
    return saveFile(editor->file());
}

bool EditorManager::saveFile(IFile *fileParam)
{
    IFile *file = fileParam;
    if (!file && currentEditor())
        file = currentEditor()->file();
    if (!file)
        return false;

    file->checkPermissions();

    const QString &fileName = file->fileName();

    if (fileName.isEmpty())
        return saveFileAs(file);

    bool success = false;

    // try saving, no matter what isReadOnly tells us
    m_d->m_core->fileManager()->blockFileChange(file);
    success = file->save(fileName);
    m_d->m_core->fileManager()->unblockFileChange(file);

    if (!success) {
        MakeWritableResult answer =
                makeFileWritable(file);
        if (answer == Failed)
            return false;
        if (answer == SavedAs)
            return true;

        file->checkPermissions();

        m_d->m_core->fileManager()->blockFileChange(file);
        success = file->save(fileName);
        m_d->m_core->fileManager()->unblockFileChange(file);
    }

    if (success) {
        addFileToRecentFiles(file);
    }

    return success;
}

MakeWritableResult
EditorManager::makeFileWritable(IFile *file)
{
    if (!file)
        return Failed;
    QString directory = QFileInfo(file->fileName()).absolutePath();
    IVersionControl *versionControl = m_d->m_core->vcsManager()->findVersionControlForDirectory(directory);
    const QString &fileName = file->fileName();

    switch (FileManager::promptReadOnlyFile(fileName, versionControl, m_d->m_core->mainWindow(), file->isSaveAsAllowed())) {
    case FileManager::RO_OpenVCS:
        if (!versionControl->vcsOpen(fileName)) {
            QMessageBox::warning(m_d->m_core->mainWindow(), tr("Failed!"), tr("Could not open the file for editing with SCC."));
            return Failed;
        }
        file->checkPermissions();
        return OpenedWithVersionControl;
    case FileManager::RO_MakeWriteable: {
        const bool permsOk = QFile::setPermissions(fileName, QFile::permissions(fileName) | QFile::WriteUser);
        if (!permsOk) {
            QMessageBox::warning(m_d->m_core->mainWindow(), tr("Failed!"),  tr("Could not set permissions to writable."));
            return Failed;
        }
    }
        file->checkPermissions();
        return MadeWritable;
    case FileManager::RO_SaveAs :
        return saveFileAs(file) ? SavedAs : Failed;
    case FileManager::RO_Cancel:
        break;
    }
    return Failed;
}

bool EditorManager::saveFileAs(IFile *fileParam)
{
    IFile *file = fileParam;
    if (!file && currentEditor())
        file = currentEditor()->file();
    if (!file)
        return false;

    const QString &filter = m_d->m_core->mimeDatabase()->allFiltersString();
    QString selectedFilter =
        m_d->m_core->mimeDatabase()->findByFile(QFileInfo(file->fileName())).filterString();
    const QString &absoluteFilePath =
        m_d->m_core->fileManager()->getSaveAsFileName(file, filter, &selectedFilter);

    if (absoluteFilePath.isEmpty())
        return false;

    if (absoluteFilePath != file->fileName()) {
        // close existing editors for the new file name
        const QList<IEditor *> existList = editorsForFileName(absoluteFilePath);
        if (!existList.isEmpty()) {
            closeEditors(existList, false);
        }
    }

    m_d->m_core->fileManager()->blockFileChange(file);
    const bool success = file->save(absoluteFilePath);
    m_d->m_core->fileManager()->unblockFileChange(file);
    file->checkPermissions();

    // @todo: There is an issue to be treated here. The new file might be of a different mime
    // type than the original and thus require a different editor. An alternative strategy
    // would be to close the current editor and open a new appropriate one, but this is not
    // a good way out either (also the undo stack would be lost). Perhaps the best is to
    // re-think part of the editors design.

    if (success) {
        addFileToRecentFiles(file);
    }
    updateActions();
    return success;
}

/* Adds the file name to the recent files if there is at least one non-temporary editor for it */
void EditorManager::addFileToRecentFiles(IFile *file)
{
    bool isTemporary = true;
    QString editorId;
    QList<IEditor *> editors = editorsForFile(file);
    foreach (IEditor *editor, editors) {
        if (!editor->isTemporary()) {
            editorId = editor->id();
            isTemporary = false;
            break;
        }
    }
    if (!isTemporary)
        m_d->m_core->fileManager()->addToRecentFiles(file->fileName(), editorId);
}

void EditorManager::gotoNextDocHistory()
{
    OpenEditorsWindow *dialog = windowPopup();
    if (dialog->isVisible()) {
        dialog->selectNextEditor();
    } else {
        EditorView *view = currentEditorView();
        dialog->setEditors(m_d->m_view, view, m_d->m_editorModel);
        dialog->selectNextEditor();
        showPopupOrSelectDocument();
    }
}

void EditorManager::gotoPreviousDocHistory()
{
    OpenEditorsWindow *dialog = windowPopup();
    if (dialog->isVisible()) {
        dialog->selectPreviousEditor();
    } else {
        EditorView *view = currentEditorView();
        dialog->setEditors(m_d->m_view, view, m_d->m_editorModel);
        dialog->selectPreviousEditor();
        showPopupOrSelectDocument();
    }
}

void EditorManager::makeCurrentEditorWritable()
{
    if (IEditor* curEditor = currentEditor())
        makeFileWritable(curEditor->file());
}

void EditorManager::updateWindowTitle()
{
    QString windowTitle = tr("Qt Creator");
    if (!m_d->m_titleAddition.isEmpty()) {
        windowTitle.prepend(m_d->m_titleAddition + " - ");
    }
    IEditor *curEditor = currentEditor();
    if (curEditor) {
        QString editorName = curEditor->displayName();
        if (!editorName.isEmpty())
            windowTitle.prepend(editorName + " - ");
        QString filePath = QFileInfo(curEditor->file()->fileName()).absoluteFilePath();
        if (!filePath.isEmpty())
            m_d->m_core->mainWindow()->setWindowFilePath(filePath);
    } else {
        m_d->m_core->mainWindow()->setWindowFilePath(QString());
    }
    m_d->m_core->mainWindow()->setWindowTitle(windowTitle);
}

void EditorManager::handleEditorStateChange()
{
    updateActions();
    IEditor *currEditor = currentEditor();
    if (qobject_cast<IEditor *>(sender()) == currEditor) {
        updateWindowTitle();
        emit currentEditorStateChanged(currEditor);
    }
}

void EditorManager::updateActions()
{
    QString fName;
    IEditor *curEditor = currentEditor();
    int openedCount = openedEditors().count() + m_d->m_editorModel->restoredEditors().count();

    if (curEditor) {

        if (!curEditor->file()->fileName().isEmpty()) {
            QFileInfo fi(curEditor->file()->fileName());
            fName = fi.fileName();
        } else {
            fName = curEditor->displayName();
        }

#ifdef Q_WS_MAC
        window()->setWindowModified(curEditor->file()->isModified());
#endif
        if (curEditor->file()->isModified() && curEditor->file()->isReadOnly()) {
            // we are about to change a read-only file, warn user
            showEditorInfoBar(QLatin1String("Core.EditorManager.MakeWritable"),
                tr("<b>Warning:</b> You are changing a read-only file."),
                tr("Make writable"), this, SLOT(makeCurrentEditorWritable()));
        } else {
            hideEditorInfoBar(QLatin1String("Core.EditorManager.MakeWritable"));
        }
#ifdef Q_WS_MAC
    } else { // curEditor
        window()->setWindowModified(false);
#endif
    }

    m_d->m_saveAction->setEnabled(curEditor != 0 && curEditor->file()->isModified());
    m_d->m_saveAsAction->setEnabled(curEditor != 0 && curEditor->file()->isSaveAsAllowed());
    m_d->m_revertToSavedAction->setEnabled(curEditor != 0
        && !curEditor->file()->fileName().isEmpty() && curEditor->file()->isModified());

    QString quotedName;
    if (!fName.isEmpty())
        quotedName = '"' + fName + '"';

    m_d->m_saveAsAction->setText(tr("Save %1 &As...").arg(quotedName));
    m_d->m_saveAction->setText(tr("&Save %1").arg(quotedName));
    m_d->m_revertToSavedAction->setText(tr("Revert %1 to Saved").arg(quotedName));

    m_d->m_closeCurrentEditorAction->setEnabled(curEditor != 0);
    m_d->m_closeCurrentEditorAction->setText(tr("Close %1").arg(quotedName));
    m_d->m_closeAllEditorsAction->setEnabled(openedCount > 0);
    m_d->m_closeOtherEditorsAction->setEnabled(openedCount > 1);
    m_d->m_closeOtherEditorsAction->setText((openedCount > 1 ? tr("Close All Except %1").arg(quotedName) : tr("Close Others")));

    m_d->m_gotoNextDocHistoryAction->setEnabled(m_d->m_editorModel->rowCount() != 0);
    m_d->m_gotoPreviousDocHistoryAction->setEnabled(m_d->m_editorModel->rowCount() != 0);
    EditorView *view  = currentEditorView();
    m_d->m_goBackAction->setEnabled(view ? view->canGoBack() : false);
    m_d->m_goForwardAction->setEnabled(view ? view->canGoForward() : false);

    bool hasSplitter = m_d->m_splitter->isSplitter();
    m_d->m_removeCurrentSplitAction->setEnabled(hasSplitter);
    m_d->m_removeAllSplitsAction->setEnabled(hasSplitter);
    m_d->m_gotoOtherSplitAction->setEnabled(hasSplitter);

    m_d->m_openInExternalEditorAction->setEnabled(curEditor != 0);
}

bool EditorManager::hasSplitter() const
{
    return m_d->m_splitter->isSplitter();
}

QList<IEditor*> EditorManager::visibleEditors() const
{
    QList<IEditor *> editors;
    if (m_d->m_splitter->isSplitter()) {
        SplitterOrView *firstView = m_d->m_splitter->findFirstView();
        SplitterOrView *view = firstView;
        if (view) {
            do {
                if (view->editor())
                    editors.append(view->editor());
                view = m_d->m_splitter->findNextView(view);
            } while (view && view != firstView);
        }
    } else {
        if (m_d->m_splitter->editor()) {
            editors.append(m_d->m_splitter->editor());
        }
    }
    return editors;
}

QList<IEditor*> EditorManager::openedEditors() const
{
    return m_d->m_editorModel->editors();
}

OpenEditorsModel *EditorManager::openedEditorsModel() const
{
    return m_d->m_editorModel;
}

void EditorManager::addCurrentPositionToNavigationHistory(IEditor *editor, const QByteArray &saveState)
{
    currentEditorView()->addCurrentPositionToNavigationHistory(editor, saveState);
    updateActions();
}

void EditorManager::cutForwardNavigationHistory()
{
    currentEditorView()->cutForwardNavigationHistory();
    updateActions();
}

void EditorManager::goBackInNavigationHistory()
{
    currentEditorView()->goBackInNavigationHistory();
    updateActions();
    return;
}

void EditorManager::goForwardInNavigationHistory()
{
    currentEditorView()->goForwardInNavigationHistory();
    updateActions();
}

OpenEditorsWindow *EditorManager::windowPopup() const
{
    return m_d->m_windowPopup;
}

void EditorManager::showPopupOrSelectDocument() const
{
    if (QApplication::keyboardModifiers() == Qt::NoModifier) {
        windowPopup()->selectAndHide();
    } else {
        // EditorManager is invisible when invoked from Design Mode.
        const QPoint p = isVisible() ?
                         mapToGlobal(QPoint(0, 0)) :
                         m_d->m_core->mainWindow()->mapToGlobal(QPoint(0, 0));
        windowPopup()->move((width()-m_d->m_windowPopup->width())/2 + p.x(),
                            (height()-m_d->m_windowPopup->height())/2 + p.y());
        windowPopup()->setVisible(true);
    }
}

// Save state of all non-teporary editors.
QByteArray EditorManager::saveState() const
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);

    stream << QByteArray("EditorManagerV4");

    QList<IEditor *> editors = openedEditors();
    foreach (IEditor *editor, editors) {
        if (!editor->file()->fileName().isEmpty()
                && !editor->isTemporary()) {
            QByteArray state = editor->saveState();
            if (!state.isEmpty())
                m_d->m_editorStates.insert(editor->file()->fileName(), QVariant(state));
        }
    }

    stream << m_d->m_editorStates;

    QList<OpenEditorsModel::Entry> entries = m_d->m_editorModel->entries();
    int entriesCount = 0;
    foreach (const OpenEditorsModel::Entry &entry, entries) {
        // The editor may be 0 if it was not loaded yet: In that case it is not temporary
        if (!entry.editor || !entry.editor->isTemporary())
            ++entriesCount;
    }

    stream << entriesCount;

    foreach (const OpenEditorsModel::Entry &entry, entries) {
        if (!entry.editor || !entry.editor->isTemporary())
            stream << entry.fileName() << entry.displayName() << entry.id().toUtf8();
    }

    stream << m_d->m_splitter->saveState();

    return bytes;
}

bool EditorManager::restoreState(const QByteArray &state)
{
    closeAllEditors(true);
    removeAllSplits();
    QDataStream stream(state);

    QByteArray version;
    stream >> version;

    if (version != "EditorManagerV4")
        return false;

    QMap<QString, QVariant> editorstates;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    stream >> editorstates;

    QMapIterator<QString, QVariant> i(editorstates);
    while (i.hasNext()) {
        i.next();
        m_d->m_editorStates.insert(i.key(), i.value());
    }

    int editorCount = 0;
    stream >> editorCount;
    while (--editorCount >= 0) {
        QString fileName;
        stream >> fileName;
        QString displayName;
        stream >> displayName;
        QByteArray id;
        stream >> id;

        if (!fileName.isEmpty() && !displayName.isEmpty())
            m_d->m_editorModel->addRestoredEditor(fileName, displayName, QString::fromUtf8(id));
    }

    QByteArray splitterstates;
    stream >> splitterstates;
    m_d->m_splitter->restoreState(splitterstates);

    // splitting and stuff results in focus trouble, that's why we set the focus again after restoration
    if (m_d->m_currentEditor) {
        m_d->m_currentEditor->widget()->setFocus();
    } else if (Core::Internal::SplitterOrView *view = currentSplitterOrView()) {
        if (IEditor *e = view->editor())
            e->widget()->setFocus();
        else if (view->view())
            view->view()->setFocus();
    }

    QApplication::restoreOverrideCursor();

    return true;
}

static const char * const documentStatesKey = "EditorManager/DocumentStates";
static const char * const externalEditorKey = "EditorManager/ExternalEditorCommand";
static const char * const reloadBehaviorKey = "EditorManager/ReloadBehavior";
static const char * const utf8BomBehaviorKey = "EditorManager/Utf8BomBehavior";

void EditorManager::saveSettings()
{
    SettingsDatabase *settings = m_d->m_core->settingsDatabase();
    settings->setValue(QLatin1String(documentStatesKey), m_d->m_editorStates);
    settings->setValue(QLatin1String(externalEditorKey), m_d->m_externalEditor);
    settings->setValue(QLatin1String(reloadBehaviorKey), m_d->m_reloadSetting);
    settings->setValue(QLatin1String(utf8BomBehaviorKey), m_d->m_utf8BomSetting);
}

void EditorManager::readSettings()
{
    // Backward compatibility to old locations for these settings
    QSettings *qs = m_d->m_core->settings();
    if (qs->contains(QLatin1String(documentStatesKey))) {
        m_d->m_editorStates = qs->value(QLatin1String(documentStatesKey))
            .value<QMap<QString, QVariant> >();
        qs->remove(QLatin1String(documentStatesKey));
    }
    if (qs->contains(QLatin1String(externalEditorKey))) {
        m_d->m_externalEditor = qs->value(QLatin1String(externalEditorKey)).toString();
        qs->remove(QLatin1String(externalEditorKey));
    }

    SettingsDatabase *settings = m_d->m_core->settingsDatabase();
    if (settings->contains(QLatin1String(documentStatesKey)))
        m_d->m_editorStates = settings->value(QLatin1String(documentStatesKey))
            .value<QMap<QString, QVariant> >();
    if (settings->contains(QLatin1String(externalEditorKey)))
        m_d->m_externalEditor = settings->value(QLatin1String(externalEditorKey)).toString();

    if (settings->contains(QLatin1String(reloadBehaviorKey)))
        m_d->m_reloadSetting = (IFile::ReloadSetting)settings->value(QLatin1String(reloadBehaviorKey)).toInt();

    if (settings->contains(QLatin1String(utf8BomBehaviorKey)))
        m_d->m_utf8BomSetting = (IFile::Utf8BomSetting)settings->value(QLatin1String(utf8BomBehaviorKey)).toInt();
}


void EditorManager::revertToSaved()
{
    IEditor *currEditor = currentEditor();
    if (!currEditor)
        return;
    const QString fileName =  currEditor->file()->fileName();
    if (fileName.isEmpty())
        return;
    if (currEditor->file()->isModified()) {
        QMessageBox msgBox(QMessageBox::Question, tr("Revert to Saved"),
                           tr("You will lose your current changes if you proceed reverting %1.").arg(QDir::toNativeSeparators(fileName)),
                           QMessageBox::Yes|QMessageBox::No, m_d->m_core->mainWindow());
        msgBox.button(QMessageBox::Yes)->setText(tr("Proceed"));
        msgBox.button(QMessageBox::No)->setText(tr("Cancel"));
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setEscapeButton(QMessageBox::No);
        if (msgBox.exec() == QMessageBox::No)
            return;

    }
    currEditor->file()->reload(IFile::FlagReload, IFile::TypeContents);
}

void EditorManager::showEditorInfoBar(const QString &id,
                                      const QString &infoText,
                                      const QString &buttonText,
                                      QObject *object, const char *buttonPressMember,
                                      const char *cancelButtonPressMember)
{
    currentEditorView()->showEditorInfoBar(id, infoText, buttonText, object, buttonPressMember, cancelButtonPressMember);
}


void EditorManager::hideEditorInfoBar(const QString &id)
{
    Core::Internal::EditorView *cev = currentEditorView();
    if (cev)
        cev->hideEditorInfoBar(id);
}

void EditorManager::showEditorStatusBar(const QString &id,
                                      const QString &infoText,
                                      const QString &buttonText,
                                      QObject *object, const char *member)
{

    currentEditorView()->showEditorStatusBar(id, infoText, buttonText, object, member);
}

void EditorManager::hideEditorStatusBar(const QString &id)
{
    currentEditorView()->hideEditorStatusBar(id);
}

QString EditorManager::externalEditorHelpText() const
{
    QString help = tr(
            "<table border=1 cellspacing=0 cellpadding=3>"
            "<tr><th>Variable</th><th>Expands to</th></tr>"
            "<tr><td>%f</td><td>file name</td></tr>"
            "<tr><td>%l</td><td>current line number</td></tr>"
            "<tr><td>%c</td><td>current column number</td></tr>"
            "<tr><td>%x</td><td>editor's x position on screen</td></tr>"
            "<tr><td>%y</td><td>editor's y position on screen</td></tr>"
            "<tr><td>%w</td><td>editor's width in pixels</td></tr>"
            "<tr><td>%h</td><td>editor's height in pixels</td></tr>"
            "<tr><td>%W</td><td>editor's width in characters</td></tr>"
            "<tr><td>%H</td><td>editor's height in characters</td></tr>"
            "<tr><td>%%</td><td>%</td></tr>"
            "</table>");
    return help;
}

void EditorManager::openInExternalEditor()
{
    QString command = m_d->m_externalEditor;
    if (command.isEmpty())
        command = defaultExternalEditor();

    if (command.isEmpty())
        return;

    IEditor *editor = currentEditor();
    if (!editor)
        return;
    if (editor->file()->isModified()) {
        bool cancelled = false;
        QList<IFile*> list = m_d->m_core->fileManager()->
                             saveModifiedFiles(QList<IFile*>() << editor->file(), &cancelled);
        if (cancelled)
            return;
    }

    QRect rect = editor->widget()->rect();
    QFont font = editor->widget()->font();
    QFontMetrics fm(font);
    rect.moveTo(editor->widget()->mapToGlobal(QPoint(0,0)));

    QString pre = command;
    QString cmd;
    for (int i = 0; i < pre.size(); ++i) {
        QChar c = pre.at(i);
        if (c == QLatin1Char('%') && i < pre.size()-1) {
            c = pre.at(++i);
            QString s;
            if (c == QLatin1Char('f'))
                s = editor->file()->fileName();
            else if (c == QLatin1Char('l'))
                s = QString::number(editor->currentLine());
            else if (c == QLatin1Char('c'))
                s = QString::number(editor->currentColumn());
            else if (c == QLatin1Char('x'))
                s = QString::number(rect.x());
            else if (c == QLatin1Char('y'))
                s = QString::number(rect.y());
            else if (c == QLatin1Char('w'))
                s = QString::number(rect.width());
            else if (c == QLatin1Char('h'))
                s = QString::number(rect.height());
            else if (c == QLatin1Char('W'))
                s = QString::number(rect.width() / fm.width(QLatin1Char('x')));
            else if (c == QLatin1Char('H'))
                s = QString::number(rect.height() / fm.lineSpacing());
            else if (c == QLatin1Char('%'))
                s = c;
            else {
                s = QLatin1Char('%');
                s += c;
            }
            cmd += s;
            continue;

        }
        cmd += c;
    }

    QProcess::startDetached(cmd);
}

void EditorManager::setExternalEditor(const QString &editor)
{
    if (editor.isEmpty() || editor == defaultExternalEditor())
        m_d->m_externalEditor = defaultExternalEditor();
    else
        m_d->m_externalEditor = editor;
}

QString EditorManager::externalEditor() const
{
    if (m_d->m_externalEditor.isEmpty())
        return defaultExternalEditor();
    return m_d->m_externalEditor;
}

void EditorManager::setReloadSetting(IFile::ReloadSetting behavior)
{
    m_d->m_reloadSetting = behavior;
}

IFile::ReloadSetting EditorManager::reloadSetting() const
{
    return m_d->m_reloadSetting;
}

void EditorManager::setUtf8BomSetting(IFile::Utf8BomSetting behavior)
{
    m_d->m_utf8BomSetting = behavior;
}

IFile::Utf8BomSetting EditorManager::utf8BomSetting() const
{
    return m_d->m_utf8BomSetting;
}

QTextCodec *EditorManager::defaultTextEncoding() const
{
    QSettings *settings = Core::ICore::instance()->settings();
    if (QTextCodec *candidate = QTextCodec::codecForName(
            settings->value(QLatin1String(Constants::SETTINGS_DEFAULTTEXTENCODING)).toByteArray()))
        return candidate;
    return QTextCodec::codecForLocale();
}

Core::IEditor *EditorManager::duplicateEditor(Core::IEditor *editor)
{
    if (!editor->duplicateSupported())
        return 0;

    IEditor *duplicate = editor->duplicate(0);
    duplicate->restoreState(editor->saveState());
    connect(duplicate, SIGNAL(changed()), this, SLOT(handleEditorStateChange()));
    emit editorCreated(duplicate, duplicate->file()->fileName());
    addEditor(duplicate, true);
    return duplicate;
}

void EditorManager::split(Qt::Orientation orientation)
{
    SplitterOrView *view = m_d->m_currentView;
    if (!view)
            view = m_d->m_currentEditor ? m_d->m_splitter->findView(m_d->m_currentEditor)
                       : m_d->m_splitter->findFirstView();
    if (view && !view->splitter()) {
        view->split(orientation);
    }
    updateActions();
}

void EditorManager::split()
{
    split(Qt::Vertical);
}

void EditorManager::splitSideBySide()
{
    split(Qt::Horizontal);
}

void EditorManager::removeCurrentSplit()
{
    SplitterOrView *viewToClose = m_d->m_currentView;
    if (!viewToClose && m_d->m_currentEditor)
        viewToClose = m_d->m_splitter->findView(m_d->m_currentEditor);

    if (!viewToClose || viewToClose->isSplitter() || viewToClose == m_d->m_splitter)
        return;

    closeView(viewToClose->view());
    updateActions();
}

void EditorManager::removeAllSplits()
{
    if (!m_d->m_splitter->isSplitter())
        return;
    IEditor *editor = m_d->m_currentEditor;
    // trigger update below
    m_d->m_currentEditor = 0;
    if (editor && m_d->m_editorModel->isDuplicate(editor))
        m_d->m_editorModel->makeOriginal(editor);
    m_d->m_splitter->unsplitAll();
    if (!editor)
        editor = pickUnusedEditor();
    activateEditor(editor);
}

void EditorManager::gotoOtherSplit()
{
    if (m_d->m_splitter->isSplitter()) {
        SplitterOrView *currentView = m_d->m_currentView;
        if (!currentView && m_d->m_currentEditor)
            currentView = m_d->m_splitter->findView(m_d->m_currentEditor);
        if (!currentView)
            currentView = m_d->m_splitter->findFirstView();
        SplitterOrView *view = m_d->m_splitter->findNextView(currentView);
        if (!view)
            view = m_d->m_splitter->findFirstView();
        if (view) {
            if (IEditor *editor = view->editor()) {
                setCurrentEditor(editor, true);
                editor->widget()->setFocus();
            } else {
                setCurrentView(view);
            }
        }
    }
}

qint64 EditorManager::maxTextFileSize()
{
    return (qint64(3) << 24);
}

void EditorManager::setWindowTitleAddition(const QString &addition)
{
    m_d->m_titleAddition = addition;
    updateWindowTitle();
}

QString EditorManager::windowTitleAddition() const
{
    return m_d->m_titleAddition;
}

