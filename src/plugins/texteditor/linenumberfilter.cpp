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

#include "linenumberfilter.h"
#include "itexteditor.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>

#include <QtCore/QVariant>

using namespace Core;
using namespace Locator;
using namespace TextEditor;
using namespace TextEditor::Internal;

LineNumberFilter::LineNumberFilter(QObject *parent)
  : ILocatorFilter(parent)
{
    setShortcutString(QString(QLatin1Char('l')));
    setIncludedByDefault(true);
}

QList<FilterEntry> LineNumberFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &, const QString &entry)
{
    bool ok;
    QList<FilterEntry> value;
    int line = entry.toInt(&ok);
    if (line > 0 && currentTextEditor())
        value.append(FilterEntry(this, tr("Line %1").arg(line), QVariant(line)));
    return value;
}

void LineNumberFilter::accept(FilterEntry selection) const
{
    ITextEditor *editor = currentTextEditor();
    if (editor) {
        Core::EditorManager *editorManager = Core::EditorManager::instance();
        editorManager->addCurrentPositionToNavigationHistory();
        editor->gotoLine(selection.internalData.toInt());
        editor->widget()->setFocus();
        Core::ModeManager::instance()->activateModeType(Core::Constants::MODE_EDIT_TYPE);
    }
}

ITextEditor *LineNumberFilter::currentTextEditor() const
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    if (!editorManager->currentEditor())
        return 0;
    return qobject_cast<TextEditor::ITextEditor*>(editorManager->currentEditor());
}
