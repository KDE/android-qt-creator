/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
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

#include "texteditormacrohandler.h"
#include "macroevent.h"
#include "macro.h"

#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>

#include <QtGui/QWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QApplication>
#include <QtGui/QShortcut>

using namespace Macros;
using namespace Macros::Internal;

static const char KEYEVENTNAME[] = "TextEditorKey";
static quint8 TEXT = 0;
static quint8 TYPE = 1;
static quint8 MODIFIERS = 2;
static quint8 KEY = 3;
static quint8 AUTOREP = 4;
static quint8 COUNT = 5;


TextEditorMacroHandler::TextEditorMacroHandler():
    IMacroHandler()
{
    const Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(changeEditor(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorAboutToClose(Core::IEditor*)),
            this, SLOT(closeEditor(Core::IEditor*)));
}

void TextEditorMacroHandler::startRecording(Macros::Macro *macro)
{
    IMacroHandler::startRecording(macro);
    if (isRecording() && m_currentEditor && m_currentEditor->widget())
        m_currentEditor->widget()->installEventFilter(this);

    // Block completion
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    am->command(TextEditor::Constants::COMPLETE_THIS)->shortcut()->blockSignals(true);
}

void TextEditorMacroHandler::endRecordingMacro(Macros::Macro *macro)
{
    if (m_currentEditor && m_currentEditor->widget())
        m_currentEditor->widget()->removeEventFilter(this);
    IMacroHandler::endRecordingMacro(macro);

    // Unblock completion
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    am->command(TextEditor::Constants::COMPLETE_THIS)->shortcut()->blockSignals(false);
}

bool TextEditorMacroHandler::canExecuteEvent(const MacroEvent &macroEvent)
{
    return (macroEvent.id() == KEYEVENTNAME);
}

bool TextEditorMacroHandler::executeEvent(const MacroEvent &macroEvent)
{
    if (!m_currentEditor)
        return false;

    QKeyEvent keyEvent((QEvent::Type)macroEvent.value(TYPE).toInt(),
                       macroEvent.value(KEY).toInt(),
                       (Qt::KeyboardModifiers)macroEvent.value(MODIFIERS).toInt(),
                       macroEvent.value(TEXT).toString(),
                       macroEvent.value(AUTOREP).toBool(),
                       macroEvent.value(COUNT).toInt());
    QApplication::instance()->sendEvent(m_currentEditor->widget(), &keyEvent);
    return true;
}

bool TextEditorMacroHandler::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched)

    if (!isRecording())
        return false;

    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = dynamic_cast<QKeyEvent *>(event);
        MacroEvent e;
        e.setId(KEYEVENTNAME);
        e.setValue(TEXT, keyEvent->text());
        e.setValue(TYPE, keyEvent->type());
        e.setValue(MODIFIERS, (int)keyEvent->modifiers());
        e.setValue(KEY, keyEvent->key());
        e.setValue(AUTOREP, keyEvent->isAutoRepeat());
        e.setValue(COUNT, keyEvent->count());
        addMacroEvent(e);
    }
    return false;
}

void TextEditorMacroHandler::changeEditor(Core::IEditor *editor)
{
    if (isRecording() && m_currentEditor && m_currentEditor->widget())
        m_currentEditor->widget()->removeEventFilter(this);

    m_currentEditor = qobject_cast<TextEditor::ITextEditor *>(editor);
    if (isRecording() && m_currentEditor && m_currentEditor->widget())
        m_currentEditor->widget()->installEventFilter(this);
}

void TextEditorMacroHandler::closeEditor(Core::IEditor *editor)
{
    Q_UNUSED(editor);
    if (isRecording() && m_currentEditor && m_currentEditor->widget())
        m_currentEditor->widget()->removeEventFilter(this);
    m_currentEditor = 0;
}
