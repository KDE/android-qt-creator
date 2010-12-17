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

#ifndef FORMEDITORW_H
#define FORMEDITORW_H

#include "designerconstants.h"

#include <coreplugin/icontext.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE

class QDesignerIntegrationInterface;
class QDesignerFormEditorInterface;
class QDesignerFormWindowInterface;

class QAction;
class QActionGroup;
class QMenu;
class QSignalMapper;
class QSettings;
class QToolBar;

namespace qdesigner_internal {
    class QDesignerFormWindowManager;
}

QT_END_NAMESPACE

namespace Core {
class ActionManager;
class ActionContainer;
class ICore;
class IEditor;
class Command;
class DesignMode;
class EditorToolBar;
}

namespace Designer {
namespace Internal {

struct EditorData;
class EditorWidget;
class SettingsPage;
class DesignerContext;

/** FormEditorW is a singleton that stores the Designer CoreInterface and
  * performs centralized operations. The instance() method will return an
  * instance. However, it must be manually deleted when unloading the
  * plugin. Since fully initializing Designer at startup is expensive, the
  * class has an internal partial initialisation stage "RegisterPlugins"
  * which is there to register the Creator plugin objects
  * that must be present at startup (settings pages, actions).
  * The plugin uses this stage at first by calling ensureInitStage().
  * Requesting an editor via instance() will fully initialize the class.
  * This is based on the assumption that the Designer settings work with
  * no plugins loaded.
  *
  * The form editor shows a read-only XML editor in edit mode and Qt Designer
  * in Design mode. */
class FormEditorW : public QObject
{
    Q_OBJECT
public:
    enum InitializationStage {
        // Register Creator plugins (settings pages, actions)
        RegisterPlugins,
        // Fully initialized for handling editor requests
        FullyInitialized
    };

    virtual ~FormEditorW();

    // Create an instance and initialize up to stage s
    static void ensureInitStage(InitializationStage s);
    // Returns fully initialized instance
    static FormEditorW *instance();
    // Deletes an existing instance if there is one.
    static void deleteInstance();

    EditorData createEditor(QWidget *parent);

    inline QDesignerFormEditorInterface *designerEditor() const { return m_formeditor; }
    inline QWidget * const*designerSubWindows() const { return m_designerSubWindows; }

    EditorData activeEditor() const;
    QList<Core::IOptionsPage *> optionsPages() const;

private slots:
    void activateEditMode(int id);
    void activateEditMode(QAction*);
    void activeFormWindowChanged(QDesignerFormWindowInterface *);
    void currentEditorChanged(Core::IEditor *editor);
    void toolChanged(int);
    void print();
    void setPreviewMenuEnabled(bool e);
    void switchSourceForm();
    void updateShortcut(QObject *command);
    void closeFormEditorsForXmlEditors(QList<Core::IEditor*> editors);

private:
    FormEditorW();
    void fullInit();

    void saveSettings(QSettings *s);

    void initDesignerSubWindows();

    void setupActions();
    void setupViewActions();
    void addDockViewAction(Core::ActionManager *am,
                           Core::ActionContainer *viewMenu,
                           int index,
                           const Core::Context &context,
                           const QString &title, const QString &id);

    Core::ActionContainer *createPreviewStyleMenu(Core::ActionManager *am,
                                                   QActionGroup *actionGroup);

    void critical(const QString &errorMessage);
    void bindShortcut(Core::Command *command, QAction *action);
    QAction *createEditModeAction(QActionGroup *ag,
                                         const Core::Context &context,
                                         Core::ActionManager *am,
                                         Core::ActionContainer *medit,
                                         const QString &actionName,
                                         const QString &name,
                                         int toolNumber,
                                         const QString &iconName = QString(),
                                         const QString &keySequence = QString());
    Core::Command *addToolAction(QAction *a, Core::ActionManager *am,
                                 const Core::Context &context, const QString &name,
                                 Core::ActionContainer *c1, const QString &keySequence = QString());
    QToolBar *createEditorToolBar() const;

    static FormEditorW *m_self;

    QDesignerFormEditorInterface *m_formeditor;
    QDesignerIntegrationInterface *m_integration;
    qdesigner_internal::QDesignerFormWindowManager *m_fwm;
    Core::ICore *m_core;
    InitializationStage m_initStage;

    QWidget *m_designerSubWindows[Designer::Constants::DesignerSubWindowCount];

    QAction *m_lockAction;
    QAction *m_resetLayoutAction;

    QList<Core::IOptionsPage *> m_settingsPages;
    QActionGroup *m_actionGroupEditMode;
    QAction *m_actionPrint;
    QAction *m_actionPreview;
    QActionGroup *m_actionGroupPreviewInStyle;
    QMenu *m_previewInStyleMenu;
    QAction *m_actionAboutPlugins;
    QAction *m_modeActionSeparator;
    QAction *m_actionSwitchSource;
    QSignalMapper *m_shortcutMapper;

    DesignerContext *m_context;
    Core::Context m_contexts;

    QStringList m_toolActionIds;
    QWidget *m_modeWidget;
    EditorWidget *m_editorWidget;
    Core::DesignMode *m_designMode;

    QWidget *m_editorToolBar;
    Core::EditorToolBar *m_toolBar;

    QMap<Core::Command *, QAction *> m_commandToDesignerAction;
};

} // namespace Internal
} // namespace Designer

#endif // FORMEDITORW_H
