/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "itexteditor.h"

#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QTextCodec>

using namespace TextEditor;

void ITextMark::paint(QPainter *painter, const QRect &rect) const
{
    m_icon.paint(painter, rect, Qt::AlignCenter);
}

void ITextMark::updateLineNumber(int lineNumber)
{
    Q_UNUSED(lineNumber)
}

void ITextMark::updateBlock(const QTextBlock &)
{}

void ITextMark::removedFromEditor()
{}

void ITextMark::documentClosing()
{}

void ITextMark::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

void ITextMark::setPriority(Priority priority)
{
    m_priority = priority;
}

ITextMark::Priority ITextMark::priority() const
{
    return m_priority;
}


double ITextMark::widthFactor() const
{
    return 1.0;
}



QMap<QString, QString> ITextEditor::openedTextEditorsContents()
{
    QMap<QString, QString> workingCopy;
    foreach (Core::IEditor *editor, Core::EditorManager::instance()->openedEditors()) {
        ITextEditor *textEditor = qobject_cast<ITextEditor *>(editor);
        if (!textEditor)
            continue;
        QString fileName = textEditor->file()->fileName();
        workingCopy[fileName] = textEditor->contents();
    }
    return workingCopy;
}

QMap<QString, QTextCodec *> TextEditor::ITextEditor::openedTextEditorsEncodings()
{
    QMap<QString, QTextCodec *> workingCopy;
    foreach (Core::IEditor *editor, Core::EditorManager::instance()->openedEditors()) {
        ITextEditor *textEditor = qobject_cast<ITextEditor *>(editor);
        if (!textEditor)
            continue;
        QString fileName = textEditor->file()->fileName();
        workingCopy[fileName] = textEditor->textCodec();
    }
    return workingCopy;
}
