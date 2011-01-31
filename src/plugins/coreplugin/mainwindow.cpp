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

#include "mainwindow.h"
#include "actioncontainer.h"
#include "command.h"
#include "actionmanager_p.h"
#include "coreimpl.h"
#include "coreconstants.h"
#include "editormanager.h"
#include "fancytabwidget.h"
#include "filemanager.h"
#include "generalsettings.h"
#include "helpmanager.h"
#include "ifilefactory.h"
#include "messagemanager.h"
#include "modemanager.h"
#include "mimedatabase.h"
#include "newdialog.h"
#include "outputpanemanager.h"
#include "outputpane.h"
#include "plugindialog.h"
#include "progressmanager_p.h"
#include "progressview.h"
#include "shortcutsettings.h"
#include "vcsmanager.h"
#include "ieditor.h"

#include "scriptmanager_p.h"
#include "settingsdialog.h"
#include "variablemanager.h"
#include "versiondialog.h"
#include "statusbarmanager.h"
#include "uniqueidmanager.h"
#include "manhattanstyle.h"
#include "navigationwidget.h"
#include "rightpane.h"
#include "editormanager/ieditorfactory.h"
#include "statusbarwidget.h"
#include "basefilewizard.h"
#include "ioutputpane.h"
#include "editormanager/systemeditor.h"

#include <coreplugin/findplaceholder.h>
#include <coreplugin/icorelistener.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <coreplugin/settingsdatabase.h>
#include <utils/pathchooser.h>
#include <utils/stylehelper.h>
#include <utils/stringutils.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QtPlugin>
#include <QtCore/QUrl>
#include <QtCore/QDir>
#include <QtCore/QFile>

#include <QtGui/QApplication>
#include <QtGui/QCloseEvent>
#include <QtGui/QMenu>
#include <QtGui/QPixmap>
#include <QtGui/QPrinter>
#include <QtGui/QShortcut>
#include <QtGui/QStatusBar>
#include <QtGui/QWizard>
#include <QtGui/QToolButton>
#include <QtGui/QMessageBox>
#include <QtGui/QMenuBar>

/*
#ifdef Q_OS_UNIX
#include <signal.h>
extern "C" void handleSigInt(int sig)
{
    Q_UNUSED(sig)
    Core::ICore::instance()->exit();
    qDebug() << "SIGINT caught. Shutting down.";
}
#endif
*/

using namespace Core;
using namespace Core::Internal;

enum { debugMainWindow = 0 };

MainWindow::MainWindow() :
    EventFilteringMainWindow(),
    m_coreImpl(new CoreImpl(this)),
    m_uniqueIDManager(new UniqueIDManager()),
    m_additionalContexts(Constants::C_GLOBAL),
    m_settings(ExtensionSystem::PluginManager::instance()->settings()),
    m_globalSettings(new QSettings(QSettings::IniFormat, QSettings::SystemScope,
                             QLatin1String("Nokia"), QLatin1String("QtCreator"), this)),
    m_settingsDatabase(new SettingsDatabase(QFileInfo(m_settings->fileName()).path(),
                                            QLatin1String("QtCreator"),
                                            this)),
    m_printer(0),
    m_actionManager(new ActionManagerPrivate(this)),
    m_editorManager(0),
    m_fileManager(new FileManager(this)),
    m_progressManager(new ProgressManagerPrivate()),
    m_scriptManager(new ScriptManagerPrivate(this)),
    m_variableManager(new VariableManager),
    m_vcsManager(new VcsManager),
    m_statusBarManager(0),
    m_modeManager(0),
    m_mimeDatabase(new MimeDatabase),
    m_helpManager(new HelpManager),
    m_navigationWidget(0),
    m_rightPaneWidget(0),
    m_versionDialog(0),
    m_activeContext(0),
    m_generalSettings(new GeneralSettings),
    m_shortcutSettings(new ShortcutSettings),
    m_systemEditor(new SystemEditor),
    m_focusToEditor(0),
    m_newAction(0),
    m_openAction(0),
    m_openWithAction(0),
    m_saveAllAction(0),
    m_exitAction(0),
    m_optionsAction(0),
    m_toggleSideBarAction(0),
    m_toggleFullScreenAction(0),
#ifdef Q_WS_MAC
    m_minimizeAction(0),
    m_zoomAction(0),
#endif
    m_toggleSideBarButton(new QToolButton)
{
    OutputPaneManager::create();

    setWindowTitle(tr("Qt Creator"));
#ifndef Q_WS_MAC
    QApplication::setWindowIcon(QIcon(QLatin1String(Constants::ICON_QTLOGO_128)));
#endif
    QCoreApplication::setApplicationName(QLatin1String("QtCreator"));
    QCoreApplication::setApplicationVersion(QLatin1String(Core::Constants::IDE_VERSION_LONG));
    QCoreApplication::setOrganizationName(QLatin1String("Nokia"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QString baseName = QApplication::style()->objectName();
#ifdef Q_WS_X11
    if (baseName == QLatin1String("windows")) {
        // Sometimes we get the standard windows 95 style as a fallback
        // e.g. if we are running on a KDE4 desktop
        QByteArray desktopEnvironment = qgetenv("DESKTOP_SESSION");
        if (desktopEnvironment == "kde")
            baseName = QLatin1String("plastique");
        else
            baseName = QLatin1String("cleanlooks");
    }
#endif
    qApp->setStyle(new ManhattanStyle(baseName));

    setDockNestingEnabled(true);

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    registerDefaultContainers();
    registerDefaultActions();

    m_navigationWidget = new NavigationWidget(m_toggleSideBarAction);
    m_rightPaneWidget = new RightPaneWidget();

    m_modeStack = new FancyTabWidget(this);
    m_modeManager = new ModeManager(this, m_modeStack);
    m_modeManager->addWidget(m_progressManager->progressView());
    m_statusBarManager = new StatusBarManager(this);
    m_messageManager = new MessageManager;
    m_editorManager = new EditorManager(m_coreImpl, this);
    m_editorManager->hide();
    setCentralWidget(m_modeStack);

    connect(QApplication::instance(), SIGNAL(focusChanged(QWidget*,QWidget*)),
            this, SLOT(updateFocusWidget(QWidget*,QWidget*)));
    // Add a small Toolbutton for toggling the navigation widget
    statusBar()->insertPermanentWidget(0, m_toggleSideBarButton);

//    setUnifiedTitleAndToolBarOnMac(true);
#ifdef Q_OS_UNIX
     //signal(SIGINT, handleSigInt);
#endif

    statusBar()->setProperty("p_styled", true);
    setAcceptDrops(true);
}

void MainWindow::setSidebarVisible(bool visible)
{
    if (NavigationWidgetPlaceHolder::current()) {
        if (m_navigationWidget->isSuppressed() && visible) {
            m_navigationWidget->setShown(true);
            m_navigationWidget->setSuppressed(false);
        } else {
            m_navigationWidget->setShown(visible);
        }
    }
}

void MainWindow::setSuppressNavigationWidget(bool suppress)
{
    if (NavigationWidgetPlaceHolder::current())
        m_navigationWidget->setSuppressed(suppress);
}

void MainWindow::setOverrideColor(const QColor &color)
{
    m_overrideColor = color;
}

MainWindow::~MainWindow()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    pm->removeObject(m_shortcutSettings);
    pm->removeObject(m_generalSettings);
    pm->removeObject(m_systemEditor);
    delete m_messageManager;
    m_messageManager = 0;
    delete m_shortcutSettings;
    m_shortcutSettings = 0;
    delete m_generalSettings;
    m_generalSettings = 0;
    delete m_systemEditor;
    m_systemEditor = 0;
    delete m_settings;
    m_settings = 0;
    delete m_printer;
    m_printer = 0;
    delete m_uniqueIDManager;
    m_uniqueIDManager = 0;
    delete m_vcsManager;
    m_vcsManager = 0;
    //we need to delete editormanager and statusbarmanager explicitly before the end of the destructor,
    //because they might trigger stuff that tries to access data from editorwindow, like removeContextWidget

    // All modes are now gone
    OutputPaneManager::destroy();

    // Now that the OutputPaneManager is gone, is a good time to delete the view
    pm->removeObject(m_outputView);
    delete m_outputView;

    delete m_editorManager;
    m_editorManager = 0;
    delete m_statusBarManager;
    m_statusBarManager = 0;
    delete m_progressManager;
    m_progressManager = 0;
    pm->removeObject(m_coreImpl);
    delete m_coreImpl;
    m_coreImpl = 0;

    delete m_rightPaneWidget;
    m_rightPaneWidget = 0;

    delete m_modeManager;
    m_modeManager = 0;
    delete m_mimeDatabase;
    m_mimeDatabase = 0;

    delete m_helpManager;
    m_helpManager = 0;
}

bool MainWindow::init(QString *errorMessage)
{
    Q_UNUSED(errorMessage)

    if (!mimeDatabase()->addMimeTypes(QLatin1String(":/core/editormanager/BinFiles.mimetypes.xml"), errorMessage))
        return false;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    pm->addObject(m_coreImpl);
    m_statusBarManager->init();
    m_modeManager->init();
    m_progressManager->init();

    pm->addObject(m_generalSettings);
    pm->addObject(m_shortcutSettings);
    pm->addObject(m_systemEditor);


    // Add widget to the bottom, we create the view here instead of inside the
    // OutputPaneManager, since the StatusBarManager needs to be initialized before
    m_outputView = new Core::StatusBarWidget;
    m_outputView->setWidget(OutputPaneManager::instance()->buttonsWidget());
    m_outputView->setPosition(Core::StatusBarWidget::Second);
    pm->addObject(m_outputView);
    m_messageManager->init();
    return true;
}

void MainWindow::extensionsInitialized()
{
    m_editorManager->init();
    m_statusBarManager->extensionsInitalized();
    OutputPaneManager::instance()->init();
    m_vcsManager->extensionsInitialized();
    m_navigationWidget->setFactories(ExtensionSystem::PluginManager::instance()->getObjects<INavigationWidgetFactory>());

    // reading the shortcut settings must be done after all shortcuts have been registered
    m_actionManager->initialize();

    readSettings();
    updateContext();

    registerUserMimeTypes();

    emit m_coreImpl->coreAboutToOpen();
    show();
    emit m_coreImpl->coreOpened();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    emit m_coreImpl->saveSettingsRequested();

    // Save opened files
    bool cancelled;
    QList<IFile*> notSaved = fileManager()->saveModifiedFiles(fileManager()->modifiedFiles(), &cancelled);
    if (cancelled || !notSaved.isEmpty()) {
        event->ignore();
        return;
    }

    const QList<ICoreListener *> listeners =
        ExtensionSystem::PluginManager::instance()->getObjects<ICoreListener>();
    foreach (ICoreListener *listener, listeners) {
        if (!listener->coreAboutToClose()) {
            event->ignore();
            return;
        }
    }

    emit m_coreImpl->coreAboutToClose();

    writeSettings();

    m_navigationWidget->closeSubWidgets();

    event->accept();
}

// Check for desktop file manager file drop events

static bool isDesktopFileManagerDrop(const QMimeData *d, QStringList *files = 0)
{
    if (files)
        files->clear();
    // Extract dropped files from Mime data.
    if (!d->hasUrls())
        return false;
    const QList<QUrl> urls = d->urls();
    if (urls.empty())
        return false;
    // Try to find local files
    bool hasFiles = false;
    const QList<QUrl>::const_iterator cend = urls.constEnd();
    for (QList<QUrl>::const_iterator it = urls.constBegin(); it != cend; ++it) {
        const QString fileName = it->toLocalFile();
        if (!fileName.isEmpty()) {
            hasFiles = true;
            if (files) {
                files->push_back(fileName);
            } else {
                break; // No result list, sufficient for checking
            }
        }
    }
    return hasFiles;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (isDesktopFileManagerDrop(event->mimeData()) && m_filesToOpenDelayed.isEmpty()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QStringList files;
    if (isDesktopFileManagerDrop(event->mimeData(), &files)) {
        event->accept();
        m_filesToOpenDelayed.append(files);
        QTimer::singleShot(50, this, SLOT(openDelayedFiles()));
    } else {
        event->ignore();
    }
}

void MainWindow::openDelayedFiles()
{
    if (m_filesToOpenDelayed.isEmpty())
        return;
    activateWindow();
    raise();
    openFiles(m_filesToOpenDelayed, ICore::SwitchMode);
    m_filesToOpenDelayed.clear();
}

IContext *MainWindow::currentContextObject() const
{
    return m_activeContext;
}

QStatusBar *MainWindow::statusBar() const
{
    return m_modeStack->statusBar();
}

void MainWindow::registerDefaultContainers()
{
    ActionManagerPrivate *am = m_actionManager;

    ActionContainer *menubar = am->createMenuBar(Constants::MENU_BAR);

#ifndef Q_WS_MAC // System menu bar on Mac
    setMenuBar(menubar->menuBar());
#endif
    menubar->appendGroup(Constants::G_FILE);
    menubar->appendGroup(Constants::G_EDIT);
    menubar->appendGroup(Constants::G_VIEW);
    menubar->appendGroup(Constants::G_TOOLS);
    menubar->appendGroup(Constants::G_WINDOW);
    menubar->appendGroup(Constants::G_HELP);

    // File Menu
    ActionContainer *filemenu = am->createMenu(Constants::M_FILE);
    menubar->addMenu(filemenu, Constants::G_FILE);
    filemenu->menu()->setTitle(tr("&File"));
    filemenu->appendGroup(Constants::G_FILE_NEW);
    filemenu->appendGroup(Constants::G_FILE_OPEN);
    filemenu->appendGroup(Constants::G_FILE_PROJECT);
    filemenu->appendGroup(Constants::G_FILE_SAVE);
    filemenu->appendGroup(Constants::G_FILE_CLOSE);
    filemenu->appendGroup(Constants::G_FILE_PRINT);
    filemenu->appendGroup(Constants::G_FILE_OTHER);
    connect(filemenu->menu(), SIGNAL(aboutToShow()), this, SLOT(aboutToShowRecentFiles()));


    // Edit Menu
    ActionContainer *medit = am->createMenu(Constants::M_EDIT);
    menubar->addMenu(medit, Constants::G_EDIT);
    medit->menu()->setTitle(tr("&Edit"));
    medit->appendGroup(Constants::G_EDIT_UNDOREDO);
    medit->appendGroup(Constants::G_EDIT_COPYPASTE);
    medit->appendGroup(Constants::G_EDIT_SELECTALL);
    medit->appendGroup(Constants::G_EDIT_ADVANCED);
    medit->appendGroup(Constants::G_EDIT_FIND);
    medit->appendGroup(Constants::G_EDIT_OTHER);

    // Tools Menu
    ActionContainer *ac = am->createMenu(Constants::M_TOOLS);
    menubar->addMenu(ac, Constants::G_TOOLS);
    ac->menu()->setTitle(tr("&Tools"));

    // Window Menu
    ActionContainer *mwindow = am->createMenu(Constants::M_WINDOW);
    menubar->addMenu(mwindow, Constants::G_WINDOW);
    mwindow->menu()->setTitle(tr("&Window"));
    mwindow->appendGroup(Constants::G_WINDOW_SIZE);
    mwindow->appendGroup(Constants::G_WINDOW_VIEWS);
    mwindow->appendGroup(Constants::G_WINDOW_PANES);
    mwindow->appendGroup(Constants::G_WINDOW_SPLIT);
    mwindow->appendGroup(Constants::G_WINDOW_NAVIGATE);
    mwindow->appendGroup(Constants::G_WINDOW_OTHER);

    // Help Menu
    ac = am->createMenu(Constants::M_HELP);
    menubar->addMenu(ac, Constants::G_HELP);
    ac->menu()->setTitle(tr("&Help"));
    ac->appendGroup(Constants::G_HELP_HELP);
    ac->appendGroup(Constants::G_HELP_ABOUT);
}

static Command *createSeparator(ActionManager *am, QObject *parent,
                                const QString &name,
                                const Context &context)
{
    QAction *tmpaction = new QAction(parent);
    tmpaction->setSeparator(true);
    Command *cmd = am->registerAction(tmpaction, name, context);
    return cmd;
}

void MainWindow::registerDefaultActions()
{
    ActionManagerPrivate *am = m_actionManager;
    ActionContainer *mfile = am->actionContainer(Constants::M_FILE);
    ActionContainer *medit = am->actionContainer(Constants::M_EDIT);
    ActionContainer *mtools = am->actionContainer(Constants::M_TOOLS);
    ActionContainer *mwindow = am->actionContainer(Constants::M_WINDOW);
    ActionContainer *mhelp = am->actionContainer(Constants::M_HELP);

    Context globalContext(Constants::C_GLOBAL);

    // File menu separators
    Command *cmd = createSeparator(am, this, QLatin1String("QtCreator.File.Sep.Save"), globalContext);
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    cmd =  createSeparator(am, this, QLatin1String("QtCreator.File.Sep.Print"), globalContext);
    QIcon icon = QIcon::fromTheme(QLatin1String("edit-cut"), QIcon(Constants::ICON_CUT));
    mfile->addAction(cmd, Constants::G_FILE_PRINT);

    cmd =  createSeparator(am, this, QLatin1String("QtCreator.File.Sep.Close"), globalContext);
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);

    cmd = createSeparator(am, this, QLatin1String("QtCreator.File.Sep.Other"), globalContext);
    mfile->addAction(cmd, Constants::G_FILE_OTHER);

    // Edit menu separators
    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.CopyPaste"), globalContext);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);

    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.SelectAll"), globalContext);
    medit->addAction(cmd, Constants::G_EDIT_SELECTALL);

    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.Find"), globalContext);
    medit->addAction(cmd, Constants::G_EDIT_FIND);

    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.Advanced"), globalContext);
    medit->addAction(cmd, Constants::G_EDIT_ADVANCED);

    // Tools menu separators
    cmd = createSeparator(am, this, QLatin1String("QtCreator.Tools.Sep.Options"), globalContext);
    mtools->addAction(cmd, Constants::G_DEFAULT_THREE);

    // Return to editor shortcut: Note this requires Qt to fix up
    // handling of shortcut overrides in menus, item views, combos....
    m_focusToEditor = new QShortcut(this);
    cmd = am->registerShortcut(m_focusToEditor, Constants::S_RETURNTOEDITOR, globalContext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_Escape));
    connect(m_focusToEditor, SIGNAL(activated()), this, SLOT(setFocusToEditor()));

    // New File Action
    icon = QIcon::fromTheme(QLatin1String("document-new"), QIcon(Constants::ICON_NEWFILE));
    m_newAction = new QAction(icon, tr("&New File or Project..."), this);
    cmd = am->registerAction(m_newAction, Constants::NEW, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::New);
    mfile->addAction(cmd, Constants::G_FILE_NEW);
    connect(m_newAction, SIGNAL(triggered()), this, SLOT(newFile()));

    // Open Action
    icon = QIcon::fromTheme(QLatin1String("document-open"), QIcon(Constants::ICON_OPENFILE));
    m_openAction = new QAction(icon, tr("&Open File or Project..."), this);
    cmd = am->registerAction(m_openAction, Constants::OPEN, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Open);
    mfile->addAction(cmd, Constants::G_FILE_OPEN);
    connect(m_openAction, SIGNAL(triggered()), this, SLOT(openFile()));

    // Open With Action
    m_openWithAction = new QAction(tr("Open File &With..."), this);
    cmd = am->registerAction(m_openWithAction, Constants::OPEN_WITH, globalContext);
    mfile->addAction(cmd, Constants::G_FILE_OPEN);
    connect(m_openWithAction, SIGNAL(triggered()), this, SLOT(openFileWith()));

    // File->Recent Files Menu
    ActionContainer *ac = am->createMenu(Constants::M_FILE_RECENTFILES);
    mfile->addMenu(ac, Constants::G_FILE_OPEN);
    ac->menu()->setTitle(tr("Recent &Files"));
    ac->setOnAllDisabledBehavior(ActionContainer::Show);

    // Save Action
    icon = QIcon::fromTheme(QLatin1String("document-save"), QIcon(Constants::ICON_SAVEFILE));
    QAction *tmpaction = new QAction(icon, tr("&Save"), this);
    tmpaction->setEnabled(false);
    cmd = am->registerAction(tmpaction, Constants::SAVE, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Save);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultText(tr("&Save"));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    // Save As Action
    icon = QIcon::fromTheme(QLatin1String("document-save-as"));
    tmpaction = new QAction(icon, tr("Save &As..."), this);
    tmpaction->setEnabled(false);
    cmd = am->registerAction(tmpaction, Constants::SAVEAS, globalContext);
#ifdef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+S")));
#endif
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultText(tr("Save &As..."));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    // SaveAll Action
    m_saveAllAction = new QAction(tr("Save A&ll"), this);
    cmd = am->registerAction(m_saveAllAction, Constants::SAVEALL, globalContext);
#ifndef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+S")));
#endif
    mfile->addAction(cmd, Constants::G_FILE_SAVE);
    connect(m_saveAllAction, SIGNAL(triggered()), this, SLOT(saveAll()));

    // Print Action
    icon = QIcon::fromTheme(QLatin1String("document-print"));
    tmpaction = new QAction(icon, tr("&Print..."), this);
    tmpaction->setEnabled(false);
    cmd = am->registerAction(tmpaction, Constants::PRINT, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Print);
    mfile->addAction(cmd, Constants::G_FILE_PRINT);

    // Exit Action
    icon = QIcon::fromTheme(QLatin1String("application-exit"));
    m_exitAction = new QAction(icon, tr("E&xit"), this);
    cmd = am->registerAction(m_exitAction, Constants::EXIT, globalContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Q")));
    mfile->addAction(cmd, Constants::G_FILE_OTHER);
    connect(m_exitAction, SIGNAL(triggered()), this, SLOT(exit()));

    // Undo Action
    icon = QIcon::fromTheme(QLatin1String("edit-undo"), QIcon(Constants::ICON_UNDO));
    tmpaction = new QAction(icon, tr("&Undo"), this);
    cmd = am->registerAction(tmpaction, Constants::UNDO, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Undo);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultText(tr("&Undo"));
    medit->addAction(cmd, Constants::G_EDIT_UNDOREDO);
    tmpaction->setEnabled(false);

    // Redo Action
    icon = QIcon::fromTheme(QLatin1String("edit-redo"), QIcon(Constants::ICON_REDO));
    tmpaction = new QAction(icon, tr("&Redo"), this);
    cmd = am->registerAction(tmpaction, Constants::REDO, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Redo);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultText(tr("&Redo"));
    medit->addAction(cmd, Constants::G_EDIT_UNDOREDO);
    tmpaction->setEnabled(false);

    // Cut Action
    icon = QIcon::fromTheme(QLatin1String("edit-cut"), QIcon(Constants::ICON_CUT));
    tmpaction = new QAction(icon, tr("Cu&t"), this);
    cmd = am->registerAction(tmpaction, Constants::CUT, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Cut);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Copy Action
    icon = QIcon::fromTheme(QLatin1String("edit-copy"), QIcon(Constants::ICON_COPY));
    tmpaction = new QAction(icon, tr("&Copy"), this);
    cmd = am->registerAction(tmpaction, Constants::COPY, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Copy);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Paste Action
    icon = QIcon::fromTheme(QLatin1String("edit-paste"), QIcon(Constants::ICON_PASTE));
    tmpaction = new QAction(icon, tr("&Paste"), this);
    cmd = am->registerAction(tmpaction, Constants::PASTE, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Paste);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Select All
    icon = QIcon::fromTheme(QLatin1String("edit-select-all"));
    tmpaction = new QAction(icon, tr("&Select All"), this);
    cmd = am->registerAction(tmpaction, Constants::SELECTALL, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::SelectAll);
    medit->addAction(cmd, Constants::G_EDIT_SELECTALL);
    tmpaction->setEnabled(false);

    // Goto Action
    icon = QIcon::fromTheme(QLatin1String("go-jump"));
    tmpaction = new QAction(icon, tr("&Go To Line..."), this);
    cmd = am->registerAction(tmpaction, Constants::GOTO, globalContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+L")));
    medit->addAction(cmd, Constants::G_EDIT_OTHER);
    tmpaction->setEnabled(false);

    // Options Action
    m_optionsAction = new QAction(tr("&Options..."), this);
    cmd = am->registerAction(m_optionsAction, Constants::OPTIONS, globalContext);
#ifdef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+,"));
    cmd->action()->setMenuRole(QAction::PreferencesRole);
#endif
    mtools->addAction(cmd, Constants::G_DEFAULT_THREE);
    connect(m_optionsAction, SIGNAL(triggered()), this, SLOT(showOptionsDialog()));

#ifdef Q_WS_MAC
    // Minimize Action
    m_minimizeAction = new QAction(tr("Minimize"), this);
    cmd = am->registerAction(m_minimizeAction, Constants::MINIMIZE_WINDOW, globalContext);
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+M"));
    mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
    connect(m_minimizeAction, SIGNAL(triggered()), this, SLOT(showMinimized()));

    // Zoom Action
    m_zoomAction = new QAction(tr("Zoom"), this);
    cmd = am->registerAction(m_zoomAction, Constants::ZOOM_WINDOW, globalContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
    connect(m_zoomAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

    // Window separator
    cmd = createSeparator(am, this, QLatin1String("QtCreator.Window.Sep.Size"), globalContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
#endif

    // Show Sidebar Action
    m_toggleSideBarAction = new QAction(QIcon(Constants::ICON_TOGGLE_SIDEBAR),
                                        tr("Show Sidebar"), this);
    m_toggleSideBarAction->setCheckable(true);
    cmd = am->registerAction(m_toggleSideBarAction, Constants::TOGGLE_SIDEBAR, globalContext);
    cmd->setAttribute(Command::CA_UpdateText);
#ifdef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+0"));
#else
    cmd->setDefaultKeySequence(QKeySequence("Alt+0"));
#endif
    connect(m_toggleSideBarAction, SIGNAL(triggered(bool)), this, SLOT(setSidebarVisible(bool)));
    m_toggleSideBarButton->setDefaultAction(cmd->action());
    mwindow->addAction(cmd, Constants::G_WINDOW_VIEWS);
    m_toggleSideBarAction->setEnabled(false);

#ifndef Q_WS_MAC
    // Full Screen Action
    m_toggleFullScreenAction = new QAction(tr("Full Screen"), this);
    m_toggleFullScreenAction->setCheckable(true);
    cmd = am->registerAction(m_toggleFullScreenAction, Constants::TOGGLE_FULLSCREEN, globalContext);
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+Shift+F11"));
    mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
    connect(m_toggleFullScreenAction, SIGNAL(triggered(bool)), this, SLOT(setFullScreen(bool)));
#endif

    // Window->Views
    ActionContainer *mviews = am->createMenu(Constants::M_WINDOW_VIEWS);
    mwindow->addMenu(mviews, Constants::G_WINDOW_VIEWS);
    mviews->menu()->setTitle(tr("&Views"));

    // About IDE Action
    icon = QIcon::fromTheme(QLatin1String("help-about"));
#ifdef Q_WS_MAC
    tmpaction = new QAction(icon, tr("About &Qt Creator"), this); // it's convention not to add dots to the about menu
#else
    tmpaction = new QAction(icon, tr("About &Qt Creator..."), this);
#endif
    cmd = am->registerAction(tmpaction, Constants::ABOUT_QTCREATOR, globalContext);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    tmpaction->setEnabled(true);
#ifdef Q_WS_MAC
    cmd->action()->setMenuRole(QAction::ApplicationSpecificRole);
#endif
    connect(tmpaction, SIGNAL(triggered()), this,  SLOT(aboutQtCreator()));

    //About Plugins Action
    tmpaction = new QAction(tr("About &Plugins..."), this);
    cmd = am->registerAction(tmpaction, Constants::ABOUT_PLUGINS, globalContext);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    tmpaction->setEnabled(true);
#ifdef Q_WS_MAC
    cmd->action()->setMenuRole(QAction::ApplicationSpecificRole);
#endif
    connect(tmpaction, SIGNAL(triggered()), this,  SLOT(aboutPlugins()));
    // About Qt Action
//    tmpaction = new QAction(tr("About &Qt..."), this);
//    cmd = am->registerAction(tmpaction, Constants:: ABOUT_QT, globalContext);
//    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
//    tmpaction->setEnabled(true);
//    connect(tmpaction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    // About sep
#ifndef Q_WS_MAC // doesn't have the "About" actions in the Help menu
    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    cmd = am->registerAction(tmpaction, "QtCreator.Help.Sep.About", globalContext);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
#endif
}

void MainWindow::newFile()
{
    showNewItemDialog(tr("New", "Title of dialog"), IWizard::allWizards(), QString());
}

void MainWindow::openFile()
{
    openFiles(editorManager()->getOpenFileNames(), ICore::SwitchMode);
}

static QList<IFileFactory*> getNonEditorFileFactories()
{
    const QList<IFileFactory*> allFileFactories =
        ExtensionSystem::PluginManager::instance()->getObjects<IFileFactory>();
    QList<IFileFactory*> nonEditorFileFactories;
    foreach (IFileFactory *factory, allFileFactories) {
        if (!qobject_cast<IEditorFactory *>(factory))
            nonEditorFileFactories.append(factory);
    }
    return nonEditorFileFactories;
}

static IFileFactory *findFileFactory(const QList<IFileFactory*> &fileFactories,
                                     const MimeDatabase *db,
                                     const QFileInfo &fi)
{
    if (const MimeType mt = db->findByFile(fi)) {
        const QString type = mt.type();
        foreach (IFileFactory *factory, fileFactories) {
            if (factory->mimeTypes().contains(type))
                return factory;
        }
    }
    return 0;
}

// opens either an editor or loads a project
void MainWindow::openFiles(const QStringList &fileNames, ICore::OpenFilesFlags flags)
{
    QList<IFileFactory*> nonEditorFileFactories = getNonEditorFileFactories();

    foreach (const QString &fileName, fileNames) {
        const QFileInfo fi(fileName);
        const QString absoluteFilePath = fi.absoluteFilePath();
        if (IFileFactory *fileFactory = findFileFactory(nonEditorFileFactories, mimeDatabase(), fi)) {
            Core::IFile *file = fileFactory->open(absoluteFilePath);
            if (!file && (flags & ICore::StopOnLoadFail))
                return;
            if (file && (flags & ICore::SwitchMode))
                Core::ModeManager::instance()->activateMode(Core::Constants::MODE_EDIT);
        } else {
            QFlags<EditorManager::OpenEditorFlag> emFlags;
            if (flags & ICore::SwitchMode)
                emFlags = EditorManager::ModeSwitch;
            if (flags & ICore::CanContainLineNumbers)
                emFlags |=  EditorManager::CanContainLineNumber;
            Core::IEditor *editor = editorManager()->openEditor(absoluteFilePath, QString(), emFlags);
            if (!editor && (flags & ICore::StopOnLoadFail))
                return;
        }
    }
}

void MainWindow::setFocusToEditor()
{
    bool focusWasMovedToEditor = false;

    // give focus to the editor if we have one
    if (IEditor *editor = m_editorManager->currentEditor()) {
        if (qApp->focusWidget() != editor->widget()) {
            editor->widget()->setFocus();
            focusWasMovedToEditor = editor->widget()->hasFocus();
        }
    }

    // check for some maximized pane which we want to unmaximize
    if (OutputPanePlaceHolder::getCurrent()
        && OutputPanePlaceHolder::getCurrent()->isVisible()
        && OutputPanePlaceHolder::getCurrent()->isMaximized()) {
        OutputPanePlaceHolder::getCurrent()->unmaximize();
        return;
    }

    if (focusWasMovedToEditor)
        return;


    // check for some visible bar which we want to hide
    bool stuffVisible =
            (FindToolBarPlaceHolder::getCurrent() &&
             FindToolBarPlaceHolder::getCurrent()->isVisible())
            || (OutputPanePlaceHolder::getCurrent() &&
                OutputPanePlaceHolder::getCurrent()->isVisible())
            || (RightPanePlaceHolder::current() &&
                RightPanePlaceHolder::current()->isVisible());
    if (stuffVisible) {
        if (FindToolBarPlaceHolder::getCurrent())
            FindToolBarPlaceHolder::getCurrent()->hide();
        OutputPaneManager::instance()->slotHide();
        RightPaneWidget::instance()->setShown(false);
        return;
    }

    // switch to edit mode if necessary
    m_coreImpl->modeManager()->activateMode(QLatin1String(Constants::MODE_EDIT));

}

void MainWindow::showNewItemDialog(const QString &title,
                                          const QList<IWizard *> &wizards,
                                          const QString &defaultLocation)
{
    // Scan for wizards matching the filter and pick one. Don't show
    // dialog if there is only one.
    IWizard *wizard = 0;
    switch (wizards.size()) {
    case 0:
        break;
    case 1:
        wizard = wizards.front();
        break;
    default: {
        NewDialog dlg(this);
        dlg.setWizards(wizards);
        dlg.setWindowTitle(title);
        wizard = dlg.showDialog();
    }
        break;
    }

    if (!wizard)
        return;

    QString path = defaultLocation;
    if (path.isEmpty()) {
        const FileManager *fm = m_coreImpl->fileManager();
        switch (wizard->kind()) {
        case IWizard::ProjectWizard:
            // Project wizards: Check for projects directory or
            // use last visited directory of file dialog. Never start
            // at current.
            path = fm->useProjectsDirectory() ?
                       fm->projectsDirectory() :
                       fm->fileDialogLastVisitedDirectory();
            break;
        default:
            path = fm->fileDialogInitialDirectory();
            break;
        }
    }
    wizard->runWizard(path, this);
}

bool MainWindow::showOptionsDialog(const QString &category,
                                   const QString &page,
                                   QWidget *parent)
{
    emit m_coreImpl->optionsDialogRequested();
    if (!parent)
        parent = this;
    SettingsDialog *dialog = SettingsDialog::getSettingsDialog(parent, category, page);
    return dialog->execDialog();
}

void MainWindow::saveAll()
{
    m_fileManager->saveModifiedFilesSilently(m_fileManager->modifiedFiles());
    emit m_coreImpl->saveSettingsRequested();
}

void MainWindow::exit()
{
    // this function is most likely called from a user action
    // that is from an event handler of an object
    // since on close we are going to delete everything
    // so to prevent the deleting of that object we
    // just append it
    QTimer::singleShot(0, this,  SLOT(close()));
}

void MainWindow::openFileWith()
{
    QStringList fileNames = editorManager()->getOpenFileNames();
    foreach (const QString &fileName, fileNames) {
        bool isExternal;
        const QString editorId = editorManager()->getOpenWithEditorId(fileName, &isExternal);
        if (editorId.isEmpty())
            continue;
        if (isExternal) {
            editorManager()->openExternalEditor(fileName, editorId);
        } else {
            editorManager()->openEditor(fileName, editorId, Core::EditorManager::ModeSwitch);
        }
    }
}

ActionManager *MainWindow::actionManager() const
{
    return m_actionManager;
}

FileManager *MainWindow::fileManager() const
{
    return m_fileManager;
}

UniqueIDManager *MainWindow::uniqueIDManager() const
{
    return m_uniqueIDManager;
}

MessageManager *MainWindow::messageManager() const
{
    return m_messageManager;
}

VcsManager *MainWindow::vcsManager() const
{
    return m_vcsManager;
}

QSettings *MainWindow::settings(QSettings::Scope scope) const
{
    if (scope == QSettings::UserScope)
        return m_settings;
    else
        return m_globalSettings;
}

EditorManager *MainWindow::editorManager() const
{
    return m_editorManager;
}

ProgressManager *MainWindow::progressManager() const
{
    return m_progressManager;
}

ScriptManager *MainWindow::scriptManager() const
{
     return m_scriptManager;
}

VariableManager *MainWindow::variableManager() const
{
     return m_variableManager.data();
}

ModeManager *MainWindow::modeManager() const
{
    return m_modeManager;
}

MimeDatabase *MainWindow::mimeDatabase() const
{
    return m_mimeDatabase;
}

HelpManager *MainWindow::helpManager() const
{
    return m_helpManager;
}

IContext *MainWindow::contextObject(QWidget *widget)
{
    return m_contextWidgets.value(widget);
}

void MainWindow::addContextObject(IContext *context)
{
    if (!context)
        return;
    QWidget *widget = context->widget();
    if (m_contextWidgets.contains(widget))
        return;

    m_contextWidgets.insert(widget, context);
}

void MainWindow::removeContextObject(IContext *context)
{
    if (!context)
        return;

    QWidget *widget = context->widget();
    if (!m_contextWidgets.contains(widget))
        return;

    m_contextWidgets.remove(widget);
    if (m_activeContext == context)
        updateContextObject(0);
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    if (e->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            if (debugMainWindow)
                qDebug() << "main window activated";
            emit windowActivated();
        }
    } else if (e->type() == QEvent::WindowStateChange) {
#ifdef Q_WS_MAC
        bool minimized = isMinimized();
        if (debugMainWindow)
            qDebug() << "main window state changed to minimized=" << minimized;
        m_minimizeAction->setEnabled(!minimized);
        m_zoomAction->setEnabled(!minimized);
#else
        bool isFullScreen = (windowState() & Qt::WindowFullScreen) != 0;
        m_toggleFullScreenAction->setChecked(isFullScreen);
#endif
    }
}

void MainWindow::updateFocusWidget(QWidget *old, QWidget *now)
{
    Q_UNUSED(old)

    // Prevent changing the context object just because the menu or a menu item is activated
    if (qobject_cast<QMenuBar*>(now) || qobject_cast<QMenu*>(now))
        return;

    IContext *newContext = 0;
    if (focusWidget()) {
        IContext *context = 0;
        QWidget *p = focusWidget();
        while (p) {
            context = m_contextWidgets.value(p);
            if (context) {
                newContext = context;
                break;
            }
            p = p->parentWidget();
        }
    }
    updateContextObject(newContext);
}

void MainWindow::updateContextObject(IContext *context)
{
    if (context == m_activeContext)
        return;
    IContext *oldContext = m_activeContext;
    m_activeContext = context;
    if (!context || oldContext != m_activeContext) {
        emit m_coreImpl->contextAboutToChange(context);
        updateContext();
        if (debugMainWindow)
            qDebug() << "new context object =" << context << (context ? context->widget() : 0)
                     << (context ? context->widget()->metaObject()->className() : 0);
    }
}

void MainWindow::resetContext()
{
    updateContextObject(0);
}

void MainWindow::aboutToShutdown()
{
    disconnect(QApplication::instance(), SIGNAL(focusChanged(QWidget*,QWidget*)),
               this, SLOT(updateFocusWidget(QWidget*,QWidget*)));
    m_activeContext = 0;
    hide();
}

static const char *settingsGroup = "MainWindow";
static const char *colorKey = "Color";
static const char *windowGeometryKey = "WindowGeometry";
static const char *windowStateKey = "WindowState";

// TODO compat for <= 2.1, remove later
static const char *geometryKey = "Geometry";
static const char *maxKey = "Maximized";
static const char *fullScreenKey = "FullScreen";

void MainWindow::readSettings()
{
    m_settings->beginGroup(QLatin1String(settingsGroup));

    if (m_overrideColor.isValid()) {
        Utils::StyleHelper::setBaseColor(m_overrideColor);
        // Get adapted base color.
        m_overrideColor = Utils::StyleHelper::baseColor();
    } else {
        Utils::StyleHelper::setBaseColor(
                m_settings->value(QLatin1String(colorKey),
                                  QColor(Utils::StyleHelper::DEFAULT_BASE_COLOR)).value<QColor>());
    }

    // TODO compat for <= 2.1, remove later
    if (m_settings->contains(QLatin1String(geometryKey))) {
        const QVariant geom = m_settings->value(QLatin1String(geometryKey));
        if (geom.isValid()) {
            setGeometry(geom.toRect());
        } else {
            resize(1024, 700);
        }
        if (m_settings->value(QLatin1String(maxKey), false).toBool())
            setWindowState(Qt::WindowMaximized);
        setFullScreen(m_settings->value(QLatin1String(fullScreenKey), false).toBool());

        m_settings->remove(QLatin1String(geometryKey));
        m_settings->remove(QLatin1String(maxKey));
        m_settings->remove(QLatin1String(fullScreenKey));
    } else {
        if (!restoreGeometry(m_settings->value(QLatin1String(windowGeometryKey)).toByteArray())) {
            resize(1024, 700);
        }
        restoreState(m_settings->value(QLatin1String(windowStateKey)).toByteArray());
    }

    m_settings->endGroup();

    m_editorManager->readSettings();
    m_navigationWidget->restoreSettings(m_settings);
    m_rightPaneWidget->readSettings(m_settings);
}

void MainWindow::writeSettings()
{
    m_settings->beginGroup(QLatin1String(settingsGroup));

    if (!(m_overrideColor.isValid() && Utils::StyleHelper::baseColor() == m_overrideColor))
        m_settings->setValue(QLatin1String(colorKey), Utils::StyleHelper::requestedBaseColor());

    m_settings->setValue(QLatin1String(windowGeometryKey), saveGeometry());
    m_settings->setValue(QLatin1String(windowStateKey), saveState());

    m_settings->endGroup();

    m_fileManager->saveSettings();
    m_actionManager->saveSettings(m_settings);
    m_editorManager->saveSettings();
    m_navigationWidget->saveSettings(m_settings);
}

void MainWindow::updateAdditionalContexts(const Context &remove, const Context &add)
{
    foreach (const int context, remove) {
        if (context == 0)
            continue;

        int index = m_additionalContexts.indexOf(context);
        if (index != -1)
            m_additionalContexts.removeAt(index);
    }

    foreach (const int context, add) {
        if (context == 0)
            continue;

        if (!m_additionalContexts.contains(context))
            m_additionalContexts.prepend(context);
    }

    updateContext();
}

bool MainWindow::hasContext(int context) const
{
    return m_actionManager->hasContext(context);
}

void MainWindow::updateContext()
{
    Context contexts;

    if (m_activeContext)
        contexts.add(m_activeContext->context());

    contexts.add(m_additionalContexts);

    Context uniquecontexts;
    for (int i = 0; i < contexts.size(); ++i) {
        const int c = contexts.at(i);
        if (!uniquecontexts.contains(c))
            uniquecontexts.add(c);
    }

    m_actionManager->setContext(uniquecontexts);
    emit m_coreImpl->contextChanged(m_activeContext, m_additionalContexts);
}

void MainWindow::aboutToShowRecentFiles()
{
    ActionContainer *aci =
        m_actionManager->actionContainer(Constants::M_FILE_RECENTFILES);
    aci->menu()->clear();

    bool hasRecentFiles = false;
    foreach (const FileManager::RecentFile &file, m_fileManager->recentFiles()) {
        hasRecentFiles = true;
        QAction *action = aci->menu()->addAction(
                    QDir::toNativeSeparators(Utils::withTildeHomePath(file.first)));
        action->setData(qVariantFromValue(file));
        connect(action, SIGNAL(triggered()), this, SLOT(openRecentFile()));
    }
    aci->menu()->setEnabled(hasRecentFiles);
}

void MainWindow::openRecentFile()
{
    if (const QAction *action = qobject_cast<const QAction*>(sender())) {
        const FileManager::RecentFile file = action->data().value<FileManager::RecentFile>();
        editorManager()->openEditor(file.first, file.second, Core::EditorManager::ModeSwitch);
    }
}

void MainWindow::aboutQtCreator()
{
    if (!m_versionDialog) {
        m_versionDialog = new VersionDialog(this);
        connect(m_versionDialog, SIGNAL(finished(int)),
                this, SLOT(destroyVersionDialog()));
    }
    m_versionDialog->show();
}

void MainWindow::destroyVersionDialog()
{
    if (m_versionDialog) {
        m_versionDialog->deleteLater();
        m_versionDialog = 0;
    }
}

void MainWindow::aboutPlugins()
{
    PluginDialog dialog(this);
    dialog.exec();
}

QPrinter *MainWindow::printer() const
{
    if (!m_printer)
        m_printer = new QPrinter(QPrinter::HighResolution);
    return m_printer;
}

void MainWindow::setFullScreen(bool on)
{
    if (bool(windowState() & Qt::WindowFullScreen) == on)
        return;

    if (on) {
        setWindowState(windowState() | Qt::WindowFullScreen);
        //statusBar()->hide();
        //menuBar()->hide();
    } else {
        setWindowState(windowState() & ~Qt::WindowFullScreen);
        //menuBar()->show();
        //statusBar()->show();
    }
}

// Display a warning with an additional button to open
// the debugger settings dialog if settingsId is nonempty.

bool MainWindow::showWarningWithOptions(const QString &title,
                                        const QString &text,
                                        const QString &details,
                                        const QString &settingsCategory,
                                        const QString &settingsId,
                                        QWidget *parent)
{
    if (parent == 0)
        parent = this;
    QMessageBox msgBox(QMessageBox::Warning, title, text,
                       QMessageBox::Ok, parent);
    if (details.isEmpty())
        msgBox.setDetailedText(details);
    QAbstractButton *settingsButton = 0;
    if (!settingsId.isEmpty() || !settingsCategory.isEmpty())
        settingsButton = msgBox.addButton(tr("Settings..."), QMessageBox::AcceptRole);
    msgBox.exec();
    if (settingsButton && msgBox.clickedButton() == settingsButton) {
        return showOptionsDialog(settingsCategory, settingsId);
    }
    return false;
}

void MainWindow::registerUserMimeTypes() const
{
    // This is to temporarily allow user specific MIME types (without recompilation).
    // Be careful with the file contents. Otherwise unpredictable behavior might arise.
    const QString &fileName = m_coreImpl->userResourcePath() + QLatin1String("/mimetypes.xml");
    if (QFile::exists(fileName)) {
        QString error;
        if (!m_coreImpl->mimeDatabase()->addMimeTypes(fileName, &error))
            qWarning() << error;
    }
}
