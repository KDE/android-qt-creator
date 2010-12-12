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

#include "opendocumentsfilter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

using namespace Core;
using namespace Locator;
using namespace Locator::Internal;

OpenDocumentsFilter::OpenDocumentsFilter(EditorManager *editorManager) :
    m_editorManager(editorManager)
{
    connect(m_editorManager, SIGNAL(editorOpened(Core::IEditor*)),
            this, SLOT(refreshInternally()));
    connect(m_editorManager, SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this, SLOT(refreshInternally()));
    setShortcutString(QString(QLatin1Char('o')));
    setIncludedByDefault(true);
}

QList<FilterEntry> OpenDocumentsFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry)
{
    QList<FilterEntry> value;
    const QChar asterisk = QLatin1Char('*');
    QString pattern = QString(asterisk);
    pattern += entry;
    pattern += asterisk;
    const QRegExp regexp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return value;
    foreach (const OpenEditorsModel::Entry &editorEntry, m_editors) {
        if (future.isCanceled())
            break;
        QString fileName = editorEntry.fileName();
        QString displayName = editorEntry.displayName();
        if (regexp.exactMatch(displayName)) {
            if (fileName.isEmpty()) {
                if (editorEntry.editor)
                    value.append(FilterEntry(this, displayName, qVariantFromValue(editorEntry.editor)));
            } else {
                QFileInfo fi(fileName);
                FilterEntry fiEntry(this, fi.fileName(), fileName);
                fiEntry.extraInfo = QDir::toNativeSeparators(fi.path());
                fiEntry.resolveFileIcon = true;
                value.append(fiEntry);
            }
        }
    }
    return value;
}

void OpenDocumentsFilter::refreshInternally()
{
    m_editors.clear();
    foreach (IEditor *editor, m_editorManager->openedEditors()) {
        OpenEditorsModel::Entry entry;
        entry.editor = editor;
        m_editors.append(entry);
    }
    m_editors += m_editorManager->openedEditorsModel()->restoredEditors();
}

void OpenDocumentsFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    QMetaObject::invokeMethod(this, "refreshInternally", Qt::BlockingQueuedConnection);
}

void OpenDocumentsFilter::accept(FilterEntry selection) const
{
    IEditor *editor = selection.internalData.value<IEditor *>();
    if (editor) {
        m_editorManager->activateEditor(editor, Core::EditorManager::ModeSwitch);
        return;
    }
    m_editorManager->openEditor(selection.internalData.toString(), QString(), Core::EditorManager::ModeSwitch);
}
