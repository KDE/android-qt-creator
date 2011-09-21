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

#ifndef CPPPLUGIN_H
#define CPPPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <coreplugin/editormanager/ieditorfactory.h>

#include <QtCore/QtPlugin>
#include <QtCore/QStringList>
#include <QtGui/QAction>

namespace TextEditor {
class TextEditorActionHandler;
class ITextEditor;
} // namespace TextEditor

namespace CppEditor {
namespace Internal {

class CPPEditorWidget;
class CppQuickFixCollector;
class CppQuickFixAssistProvider;

class CppPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    CppPlugin();
    ~CppPlugin();

    static CppPlugin *instance();

    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    // Connect editor to settings changed signals.
    void initializeEditor(CPPEditorWidget *editor);

    bool sortedOutline() const;

    CppQuickFixAssistProvider *quickFixProvider() const;

signals:
    void outlineSortingChanged(bool sort);
    void typeHierarchyRequested();

public slots:
    void setSortedOutline(bool sorted);

private slots:
    void switchDeclarationDefinition();
    void jumpToDefinition();
    void renameSymbolUnderCursor();
    void onTaskStarted(const QString &type);
    void onAllTasksFinished(const QString &type);
    void findUsages();
    void currentEditorChanged(Core::IEditor *editor);
    void openTypeHierarchy();

private:
    Core::IEditor *createEditor(QWidget *parent);
    void writeSettings();
    void readSettings();

    static CppPlugin *m_instance;

    TextEditor::TextEditorActionHandler *m_actionHandler;
    bool m_sortedOutline;
    QAction *m_renameSymbolUnderCursorAction;
    QAction *m_findUsagesAction;
    QAction *m_updateCodeModelAction;
    QAction *m_openTypeHierarchyAction;

    CppQuickFixAssistProvider *m_quickFixProvider;

    QPointer<TextEditor::ITextEditor> m_currentEditor;
};

class CppEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    CppEditorFactory(CppPlugin *owner);

    // IEditorFactory
    QStringList mimeTypes() const;
    Core::IEditor *createEditor(QWidget *parent);
    Core::Id id() const;
    QString displayName() const;
    Core::IFile *open(const QString &fileName);

private:
    CppPlugin *m_owner;
    QStringList m_mimeTypes;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPPLUGIN_H
